/*
 * sampling.c — Modbus → CANopen 命令转发 (被动响应模式)
 *
 * 上位机一步一步发 Modbus 指令, MCU 逐个响应:
 *
 *   上位机                              MCU
 *   ─────────────────────────────────────────────
 *   06 写 0x000B = 0x8000 (停止)   →  mT_position=0, 停止 RPDO2
 *   06 写 0x000B = 0x9000 (启动)   →  mT_position=目标位, 启动 RPDO2
 *   06 写 0x000B = 0xC000 (回零)   →  mT_position=-19, 回零
 *   06 写 0x0005 = 0x2000 (启泵)   →  mX_control_word=1, 启动泵
 *   06 写 0x0005 = 0x0000 (停泵)   →  mX_control_word=0, 停止泵
 *   06 写 0x0004 = 0x8000 (触发)   →  执行动作命令
 *   06 写 0x0004 = 0x1000 (就绪)   →  设就绪状态
 *   03 读 0x0000(3)    (孔位)     →  返回 Node6 编码器位置
 *   03 读 0x000B       (电机状态)  →  返回 0x8000/0x9000/0xC000
 *   03 读 0x0063/0x0064(传感器)   →  返回 Node8 双通道
 *
 * ── 关键: MCU 不主动驱动流程, 只在收到 Modbus 写时执行对应 CAN 动作 ──
 */
#include "sampling.h"
#include "reg_router.h"
#include "canopen_master.h"
#include <rtthread.h>
#include <string.h>

/* OD 变量 (ObjDict.h) */
extern INTEGER32 mT_position;         /* Node3 转盘位置 */
extern UNS16    mT_control_word;
extern UNS16    mT_status_word;
extern INTEGER8  mT_modes;
extern INTEGER32 mX_position;         /* Node1 电机A */
extern UNS16    mX_control_word;
extern UNS16    mX_status_word;
extern INTEGER32 mY_position;         /* Node2 */
extern UNS16    mY_control_word;
extern INTEGER32 mZ_position;         /* Node3 */
extern UNS16    mZ_control_word;
extern INTEGER32 mE_position;         /* Node4 升降轴 */
extern UNS16    mE_control_word;
extern UNS16    mE_status_word;
extern UNS16    photometer_ch0;       /* RPDO1 Node6 */
extern UNS16    photometer_ch1;
extern UNS32    current_heating;      /* Node8 ch1 → Modbus 0x0063 */
extern UNS32    current_refrigeration;/* Node8 ch2 → Modbus 0x0064 */
extern UNS32    heating_target;        /* OD 0x201E → PDO 0x207 */
extern UNS32    refrigeration_target;  /* OD 0x201F → PDO 0x207 */

#define DBG_TAG "sample"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* ═══════════════════════════════════════════════════════════════
 *  孔位定义
 * ═══════════════════════════════════════════════════════════════ */
const int32_t slot_positions[SLOT_COUNT] = {
    13980, 13847, 13447, 12914, 10647, 5180, 3847
};

/* ═══════════════════════════════════════════════════════════════
 *  寄存器地址 (0-based Modbus frame address)
 * ═══════════════════════════════════════════════════════════════ */
#define R_HOLE0      0     /* 0x0000 孔位编码 [3reg] */
#define R_NEEDLE     3     /* 0x0003 针状态 */
#define R_ACTION     4     /* 0x0004 动作命令 */
#define R_PUMP       5     /* 0x0005 泵状态 */
#define R_PERI_SPD   7     /* 0x0007 蠕动泵转速 */
#define R_PERI_TURN  8     /* 0x0008 蠕动泵圈数 (步数) */
#define R_MODE       9     /* 0x0009 模式 */
#define R_TURNT_FN   10    /* 0x000A 转盘功能 */
#define R_MOTOR      11    /* 0x000B 电机控制 */
#define R_LIGHT      15    /* 0x000F 光强 */
#define R_DEV_STAT   17    /* 0x0011 设备状态 */
#define R_COOLING    14    /* 0x000E 酶孔位制冷 (Modbus 00015) */
#define R_HEAT       99    /* 0x0063 加热温度 */
#define R_COOL       100   /* 0x0064 制冷温度 */
#define R_PARAM      119   /* 0x0077 参数控制 */
#define R_WEIGH      120   /* 0x0078 称重控制 */

/* ── 当前选中的孔位索引 (0~6), 由上位机写模式寄存器间接选择 ── */
static int g_cur_slot = 0;

/* ═══════════════════════════════════════════════════════════════
 *  Modbus 寄存器 ← CAN TPDO 同步
 *  由 CANopen post_TPDO / post_sync 回调调用
 * ═══════════════════════════════════════════════════════════════ */
void sampling_sync_can_to_modbus(void)
{
    /*
     * CAN TPDO1 Node6 (0x186) → 编码器位置 → 孔位编码
     *
     * 这里用 mT_position 的低 16 位模拟编码器值。
     * 实际硬件连接后, CAN 中断会更新独立的编码器 OD 变量,
     * 届时替换为真实的编码器变量。
     *
     * 编码格式: 6 字节 = hole_index(2B) + 0x00(1B) + micro_step(1B) + crc(2B)
     */
    int32_t enc = (int32_t)(mT_position & 0xFFFF);

    uint16_t hole_index;
    uint8_t  micro;

    if (enc < 500) {
        hole_index = 0xFC00;
        micro      = 0x00;
    } else {
        hole_index = 0xFC00 | ((enc >> 8) & 0xFF);
        micro      = (enc >> 4) & 0x0F;
        /* 微步掩码: 逐级置位 */
        if      (micro >= 8) micro = 0xFF;
        else if (micro >= 7) micro = 0xFE;
        else if (micro >= 6) micro = 0xFC;
        else if (micro >= 5) micro = 0xF8;
        else if (micro >= 4) micro = 0xF0;
        else if (micro >= 3) micro = 0xE0;
        else if (micro >= 2) micro = 0xC0;
        else if (micro >= 1) micro = 0x80;
        else                 micro = 0x00;
    }

    uint16_t hole[3];
    hole[0] = hole_index;
    hole[1] = ((uint16_t)micro << 8) | 0x0000;
    hole[2] = (hole_index ^ micro ^ 0x0073) & 0xFFFF;

    /* 写入 Modbus 寄存器 (通过 reg_write API) */
    {
        uint8_t buf[6];
        buf[0] = (uint8_t)(hole[0] >> 8);
        buf[1] = (uint8_t)(hole[0] & 0xFF);
        buf[2] = (uint8_t)(hole[1] >> 8);
        buf[3] = (uint8_t)(hole[1] & 0xFF);
        buf[4] = (uint8_t)(hole[2] >> 8);
        buf[5] = (uint8_t)(hole[2] & 0xFF);
        reg_write(R_HOLE0, 3, buf);
    }

    /* 传感器 → Modbus */
    {
        uint8_t buf[2];
        uint16_t v;
        v = (uint16_t)(current_heating & 0xFFFF);
        buf[0] = (uint8_t)(v >> 8);
        buf[1] = (uint8_t)(v & 0xFF);
        reg_write(R_HEAT, 1, buf);

        v = (uint16_t)(current_refrigeration & 0xFFFF);
        buf[0] = (uint8_t)(v >> 8);
        buf[1] = (uint8_t)(v & 0xFF);
        reg_write(R_COOL, 1, buf);
    }

    /* 设备状态 (固定值) */
    {
        uint8_t buf[4];
        buf[0] = 0x10; buf[1] = 0x00;  /* 0x1000 */
        buf[2] = 0x00; buf[3] = 0x00;  /* 0x0000 */
        reg_write(R_DEV_STAT, 2, buf);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Modbus 寄存器写入回调
 *
 *  上位机写寄存器 → reg_router.reg_write() → 此函数
 *  MCU 根据写入的寄存器地址和值, 执行对应的 CANopen 动作。
 *
 *  ⚠️ 此函数在 Modbus 处理上下文中调用, 不能阻塞!
 * ═══════════════════════════════════════════════════════════════ */
void sampling_on_reg_write(uint16_t addr, uint16_t value)
{
    switch (addr) {

    /* ── 0x000B: 电机控制 ── */
    case R_MOTOR:
        switch (value) {
        case 0x8000:  /* 停止 */
            mT_position     = 0;
            mT_control_word = 0;
            mT_modes        = 0;
            photometer_ch0  = 0;
            LOG_D("Motor: STOP");
            break;

        case 0x9000:  /* 正转 → 移动到目标孔位 */
            mT_position     = slot_positions[g_cur_slot];
            mT_control_word = 0x5355;
            mT_modes        = 0x04;   /* 旋转模式 */
            photometer_ch0  = 0x0018; /* 测量模式 */
            LOG_D("Motor: GO → slot %d (pos=%ld)", g_cur_slot,
                  (long)slot_positions[g_cur_slot]);
            break;

        case 0xC000:  /* 回零 */
            mT_position     = SLOT_IDLE_POS;
            mT_control_word = 0;
            mT_modes        = 0;
            photometer_ch0  = 0x0000; /* 退出测量 */
            LOG_D("Motor: HOME → idle");
            break;

        default:
            LOG_D("Motor: unknown cmd 0x%04X", value);
            break;
        }
        break;

    /* ── 0x0004: 动作命令 ── */
    case R_ACTION:
        if (value == 0x8000) {
            /* 触发执行 — 上位机先写了具体动作值, 再写 0x8000 触发 */
            LOG_D("Action: TRIGGER execute");
            /* 动作已经在上一次写 0x0004 时设置, 这里只确认触发 */
        } else if (value == 0x1000) {
            LOG_D("Action: READY");
        } else if (value == 0x0000) {
            LOG_D("Action: IDLE/DONE");
        }
        break;

    /* ── 0x0005: 泵状态 ── */
    case R_PUMP:
        if (value == 0x2000) {
            mX_control_word = 0x0001;  /* 启动泵 */
            mE_control_word = 0x9000;  /* 升降轴 */
            LOG_D("Pump: START");
        } else if (value == 0x0000) {
            mX_control_word = 0;
            mE_control_word = 0;
            LOG_D("Pump: STOP");
        }
        break;

    /* ── 0x0008: 步数参数 ── */
    case R_PERI_TURN:
        /*
         * 上位机写的步数 (如 0x0BB8=3000) → 应通过 SDO 写 Node3 的
         * Profile Velocity (0x6081) 或直接设 mT_velocity。
         * 当前简化: 记录步数, 实际运动参数由 SDO 配置。
         */
        LOG_D("Steps: %d", (int)value);
        break;

    /* ── 0x0009: 模式选择 ── */
    case R_MODE:
        /*
         * 上位机写的模式值 → 影响 RPDO2 控制字或选择孔位
         */
        LOG_D("Mode: 0x%04X", value);
        break;

    /* ── 0x000A: 转盘功能 ── */
    case R_TURNT_FN:
        mT_modes = (INTEGER8)(value & 0xFF);
        LOG_D("Turntable fn: 0x%02X", value);
        break;

    /* ── 0x0077: 参数控制 ── */
    case R_PARAM:
        if (value == 0x0004) {
            LOG_D("Param: WASH NEEDLE");
        } else if (value == 0x0006) {
            LOG_D("Param: MIX");
        }
        break;

    /* ── 0x000E: 酶孔位制冷 ── */
    case R_COOLING:
        if (value == 0x0001) {
            refrigeration_target = 0x6400;  /* 100×100 = 10.0°C, enable */
            LOG_I("Cooling: ON");
        } else if (value == 0x8000) {
            /* 上位机写入 0x8000 — 光强触发信号 */
            photometer_ch0 = 0x0018;
            LOG_I("Light: TRIGGER (via 0x000F=0x8000)");
        } else {
            refrigeration_target = 0;
            LOG_I("Cooling: OFF");
        }
        break;

    /* ── 0x0010: 光强读数 (只读) ── */
    case R_LIGHT:
        LOG_D("Light read: 0x%04X", value);
        break;

    /* ── 0x0063: 转盘加热目标温度 ── */
    case R_HEAT:
        heating_target = ((UNS32)value) * 100;  /* ×100 */
        LOG_I("Heat target: %d.%d°C", value, value ? 0 : 0);
        break;

    /* ── 0x0064: 制冷目标温度 ── */
    case R_COOL:
        refrigeration_target = ((UNS32)value) * 100;  /* ×100 */
        LOG_I("Cool target: %d.%d°C", value, value ? 0 : 0);
        break;

    /* ── 0x0078: 称重控制 ── */
    case R_WEIGH:
        LOG_I("Weigh ctrl: 0x%04X", value);
        /* 0x0006 = 标定+去皮+容量 */
        break;

    default:
        break;
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  初始化
 * ═══════════════════════════════════════════════════════════════ */
int sampling_init(void)
{
    g_cur_slot = 0;

    /* 初始状态: 电机空闲 */
    mT_position     = SLOT_IDLE_POS;
    mT_control_word = 0;
    mT_modes        = 0;
    mX_control_word = 0;
    mY_control_word = 0;
    mZ_control_word = 0;
    mE_control_word = 0;
    photometer_ch0  = 0;
    photometer_ch1  = 0;

    LOG_I("Sampling init — Modbus→CAN gateway ready");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 *  Finsh: Modbus 寄存器仿真测试 (不用上位机)
 * ═══════════════════════════════════════════════════════════════ */
#ifdef RT_USING_FINSH
#include <finsh.h>

/* mbreg <addr> <value> — 仿真上位机写 Modbus 寄存器 */
static int mbreg(int argc, char **argv)
{
    if (argc < 3) {
        rt_kprintf("Usage: mbreg <addr> <value>\n");
        rt_kprintf("  Cool ON:    mbreg 14 1\n");
        rt_kprintf("  Cool OFF:   mbreg 14 0\n");
        rt_kprintf("  Heat temp:  mbreg 191 25\n");
        rt_kprintf("  Cool temp:  mbreg 192 10\n");
        rt_kprintf("  Motor STOP: mbreg 11 0x8000\n");
        rt_kprintf("  Motor GO:   mbreg 11 0x9000\n");
        rt_kprintf("  Pump ON:    mbreg 5 0x2000\n");
        rt_kprintf("  Read reg:   mbreg <addr>\n");
        rt_kprintf("  (addr = 0-based decimal, 0x for hex)\n");
        return 0;
    }
    uint16_t addr = (uint16_t)strtoul(argv[1], NULL, 0);

    if (argc >= 3) {
        /* Write */
        uint16_t val = (uint16_t)strtoul(argv[2], NULL, 0);
        uint8_t data[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
        reg_write(addr, 1, data);
        rt_kprintf("  WRITE reg[%d] = 0x%04X (%d)\n", addr, val, val);
    } else {
        /* Read */
        uint8_t buf[4];
        reg_read(addr, 1, buf);
        uint16_t v = ((uint16_t)buf[0] << 8) | buf[1];
        rt_kprintf("  READ  reg[%d] = 0x%04X (%d)\n", addr, v, v);
    }
    return 0;
}
MSH_CMD_EXPORT(mbreg, Modbus register read/write test);
#endif
