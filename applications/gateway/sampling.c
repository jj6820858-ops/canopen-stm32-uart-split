/*
 * sampling.c — Modbus → CANopen 命令转发 (被动响应模式)
 *
 * 上位机一步一步发 Modbus 指令, MCU 逐个响应:
 *
 *   上位机                              MCU
 *   ─────────────────────────────────────────────
 *   06 写 0x000B = 0x8000 (停止)   →  mY_position=0, 停止 Y 转盘
 *   06 写 0x000B = 0x9000 (启动)   →  mY_position=目标位, 启动 Y 转盘
 *   06 写 0x000B = 0xC000 (回零)   →  mY_position=-19, Y 转盘回零
 *   06 写 0x0005 = 0x2000 (启泵)   →  mE_control_word=0x9000, 启动 E 柱塞泵
 *   06 写 0x0005 = 0x0000 (停泵)   →  mE_control_word=0, 停止 E 柱塞泵
 *   06 写 0x0003 = 0~5    (洗针)   →  mX_modes, X 针头旋转/洗针模式
 *   06 写 0x0004 = 0x8000 (触发)   →  执行动作命令
 *   06 写 0x0004 = 0x1000 (就绪)   →  设就绪状态
 *   03 读 0x0000(3)    (孔位)     →  返回 Node6 编码器位置
 *   03 读 0x000B       (电机状态)  →  返回 0x8000/0x9000/0xC000
 *   03 读 0x0063/0x0064(传感器)   →  返回 Node8 双通道
 *
 * ── 关键: MCU 不主动驱动流程, 只在收到 Modbus 写时执行对应 CAN 动作 ──
 */
#include "sampling.h"
#include "../protocol/reg_router.h"
#include "../canopen/canopen_master.h"
#include "pdo.h"              /* 触发 PDO 发送 */
#include <rtthread.h>
#include <string.h>

/* OD 变量 (ObjDict.h) */
extern INTEGER8  mX_modes;            /* Node1 X 轴: 针头旋转 */
extern INTEGER32 mX_position;
extern INTEGER32 mX_velocity;
extern UNS16    mX_control_word;
extern UNS16    mX_status_word;
extern INTEGER8  mY_modes;            /* Node2 Y 轴: 转盘 */
extern INTEGER32 mY_position;
extern UNS16    mY_control_word;
extern INTEGER32 mZ_position;         /* Node3 Z 轴: 上下 */
extern UNS16    mZ_control_word;
extern UNS16    mE_control_word;      /* Node4 E 轴: 柱塞泵 */
extern UNS16    photometer_ch0;       /* RPDO1 Node6 */
extern UNS16    photometer_ch1;
extern UNS32    current_heating;      /* Node8 ch1 → Modbus 0x0063 */
extern UNS32    current_refrigeration;/* Node8 ch2 → Modbus 0x0064 */
extern UNS32    heating_target;        /* OD 0x201E → PDO 0x207 */
extern UNS32    refrigeration_target;  /* OD 0x201F → PDO 0x207 */
extern UNS32    TEMP_control_word;     /* OD 0x201D → PDO 0x307 */
extern unsigned short usSRegHoldBuf[];      /* FreeModbus 兜底缓存 */

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
#define R_HOLE0      150   /* 0x0096 孔位编码 [3reg] */
#define R_NEEDLE     61    /* 0x003D 针状态 */
#define R_ACTION     24    /* 0x0018 动作命令 */
#define R_PUMP       173   /* 0x00AD 泵状态 */
#define R_PERI_SPD   38    /* 0x0026 蠕动泵转速 */
#define R_PERI_TURN  91    /* 0x005B 蠕动泵圈数 (步数) */
#define R_MODE       12    /* 0x000C 模式 */
#define R_TURNT_FN   167   /* 0x00A7 转盘功能 */
#define R_MOTOR      74    /* 0x004A 电机控制 */
#define R_TURNT_POS0 118   /* 0x0076 转盘指定位号 [3reg] */
#define R_TURNT_POS1 119
#define R_TURNT_POS2 120
#define R_LIGHT      44    /* 0x002C 光强 */
#define R_DEV_STAT   5     /* 0x0005 设备状态 */
#define R_COOLING    176   /* 0x00B0 酶孔位制冷 */
#define R_HEAT       80    /* 0x0050 加热温度 */
#define R_COOL       31    /* 0x001F 制冷温度 */
#define R_PARAM      14    /* 0x000E 参数控制 */
#define R_WEIGH      57    /* 0x0039 称重控制 */
#define R_HEAT_TARGET 88   /* 0x0058 加热目标温度 */
#define R_COOL_TARGET 89   /* 0x0059 制冷目标温度 */

/* ── 当前选中的孔位索引 (0~6), 由上位机写模式寄存器间接选择 ── */
static int g_cur_slot = 0;
static int g_hole_written = 0;  /* 上位机已写孔位，禁止CAN覆盖 */
static uint16_t g_operate_bitmap = 0;
static uint16_t g_turntable_pos[3] = {0};
static uint8_t g_turntable_pos_mask = 0;

static int proto_first_set_bit(uint16_t word)
{
    for (int bit = 0; bit < 16; bit++) {
        if (word & (uint16_t)(0x8000u >> bit)) {
            return bit;
        }
    }

    return -1;
}

static int slot_from_turntable_bitmap(void)
{
    for (int word = 0; word < 3; word++) {
        int bit = proto_first_set_bit(g_turntable_pos[word]);

        if (bit >= 0) {
            int global_bit = word * 16 + bit;

            if (global_bit < 5) {
                return global_bit % SLOT_COUNT;          /* 试剂位 */
            }
            if (global_bit < 23) {
                return (global_bit - 5) % SLOT_COUNT;    /* 样品位 */
            }
            if (global_bit < 41) {
                return (global_bit - 23) % SLOT_COUNT;   /* 比色皿位 */
            }
            return 0;
        }
    }

    return g_cur_slot;
}

static int turntable_pos_write(uint16_t addr, uint16_t value)
{
    if (addr < R_TURNT_POS0 || addr > R_TURNT_POS2) {
        return 0;
    }

    /*
     * 成品上位机用 FC10 连续写 0x000C~0x000E 作为转盘目标位图。
     * 单独写 0x000E 时仍保留给制冷控制，因此只有检测到连续目标位图写入
     * 时才拦截 0x000E，避免误关制冷。
     */
    if (addr == R_TURNT_POS2 && g_turntable_pos_mask == 0) {
        return 0;
    }

    if (addr == R_TURNT_POS0) {
        memset(g_turntable_pos, 0, sizeof(g_turntable_pos));
    }

    g_turntable_pos[addr - R_TURNT_POS0] = value;
    g_turntable_pos_mask |= (uint8_t)(1u << (addr - R_TURNT_POS0));
    g_cur_slot = slot_from_turntable_bitmap();

    if (g_turntable_pos_mask == 0x07) {
        g_turntable_pos_mask = 0;
    }

    LOG_D("Turntable target: %04X %04X %04X -> slot %d",
          g_turntable_pos[0], g_turntable_pos[1], g_turntable_pos[2],
          g_cur_slot);
    return 1;
}

static void turntable_apply_function(uint16_t value)
{
    uint8_t mode = (uint8_t)(value >> 8);

    mY_modes = (INTEGER8)mode;

    if (mode & 0x80) {
        TEMP_control_word |= 0x0001;       /* 加热使能 */
        if (heating_target == 0) {
            heating_target = 2500;         /* 默认 25.00°C */
        }
    }

    if (mode & 0x04) {
        photometer_ch0 = 0x0018;           /* 读光强/测量窗口 */
    }

    LOG_D("Turntable fn: word=0x%04X mode=0x%02X", value, mode);
}

/* ═══════════════════════════════════════════════════════════════
 *  Modbus 寄存器 ← CAN TPDO 同步
 *  由 CANopen post_TPDO / post_sync 回调调用
 * ═══════════════════════════════════════════════════════════════ */
void sampling_sync_can_to_modbus(void)
{
    /* 上位机已主动写入孔位时，跳过 CAN 自动同步 */
    if (g_hole_written) {
        /* 只同步传感器数据，跳过孔位 */
        goto sync_sensors;
    }

    /*
     * CAN TPDO1 Node6 (0x186) → 编码器位置 → 孔位编码
     *
     * 这里用 mY_position 的低 16 位模拟编码器值。
     * 实际硬件连接后, CAN 中断会更新独立的编码器 OD 变量,
     * 届时替换为真实的编码器变量。
     *
     * 编码格式: 6 字节 = hole_index(2B) + 0x00(1B) + micro_step(1B) + crc(2B)
     */
    int32_t enc = (int32_t)(mY_position & 0xFFFF);

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
        reg_write_local(R_HOLE0, 3, buf);
    }

sync_sensors:
    /* 传感器 → Modbus */
    {
        uint8_t buf[2];
        uint16_t v;
        v = (uint16_t)(current_heating & 0xFFFF);
        buf[0] = (uint8_t)(v >> 8);
        buf[1] = (uint8_t)(v & 0xFF);
        reg_write_local(R_HEAT, 1, buf);

        v = (uint16_t)(current_refrigeration & 0xFFFF);
        buf[0] = (uint8_t)(v >> 8);
        buf[1] = (uint8_t)(v & 0xFF);
        reg_write_local(R_COOL, 1, buf);
    }

    /* 设备状态 (固定值) — 同时设 OD 变量防止 CANopen sync 覆盖 */
    mX_status_word = 0x1C00;  /* OD 变量 → reg_od_sync_in 会同步到 regs[17] */
    {
        uint8_t buf[4];
        buf[0] = 0x1C; buf[1] = 0x00;  /* 0x1C00 */
        buf[2] = 0x00; buf[3] = 0x01;  /* 0x0001 */
        reg_write_local(R_DEV_STAT, 2, buf);
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
    if (addr != R_TURNT_POS0 && addr != R_TURNT_POS1 && addr != R_TURNT_POS2) {
        g_turntable_pos_mask = 0;
    }

    if (turntable_pos_write(addr, value)) {
        goto send_pdo;
    }

    switch (addr) {

    /* ── 0x000B: 电机控制 ── */
    case R_MOTOR:
        switch (value) {
        case 0x8000:  /* 停止 */
            mY_position     = 0;
            mY_control_word = 0;
            mY_modes        = 0;
            photometer_ch0  = 0;
            LOG_D("Motor: STOP");
            break;

        case 0x9000:  /* 正转 → 移动到目标孔位 */
            mY_position     = slot_positions[g_cur_slot];
            mY_control_word = 0x5355;
            mY_modes        = 0x04;   /* 旋转模式 */
            photometer_ch0  = 0x0018; /* 测量模式 */
            LOG_D("Motor: GO → slot %d (pos=%ld)", g_cur_slot,
                  (long)slot_positions[g_cur_slot]);
            break;

        case 0xA000:  /* 成品上位机用于运动后的保持/确认 */
            mY_control_word = 0;
            LOG_D("Motor: HOLD");
            break;

        case 0xC000:  /* 回零 */
            mY_position     = SLOT_IDLE_POS;
            mY_control_word = 0;
            mY_modes        = 0;
            photometer_ch0  = 0x0000; /* 退出测量 */
            LOG_D("Motor: HOME → idle");
            break;

        default:
            LOG_D("Motor: unknown cmd 0x%04X", value);
            break;
        }
        break;

    /* ── 0x0003: 针头清洗/动作触发 ── */
    case R_NEEDLE:
        mX_modes = (INTEGER8)(value & 0xFF);
        LOG_I("Needle/X: mode=%d", (int)mX_modes);
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
            mE_control_word = 0x9000;  /* 启动 E 柱塞泵 */
            LOG_D("Pump: START");
        } else if (value == 0x0000) {
            mE_control_word = 0;
            LOG_D("Pump: STOP");
        }
        break;

    /* ── 0x0008: 步数参数 ── */
    case R_PERI_TURN:
        /*
         * 上位机写的步数 (如 0x0BB8=3000) → 应通过对应蠕动泵节点的
         * Profile Velocity (0x6081)。蠕动泵未绑定到 X/Y/Z/E 主轴。
         * 当前简化: 记录步数, 实际运动参数由 SDO 配置。
         */
        LOG_D("Steps: %d", (int)value);
        break;

    /* ── 0x0009: 模式选择 ── */
    case R_MODE:
        /*
         * 上位机写的操作位号，后续动作命令会引用它。
         */
        g_operate_bitmap = value;
        LOG_D("Operate bitmap: 0x%04X", g_operate_bitmap);
        break;

    /* ── 0x000A: 转盘功能/电机控制 ── */
    case R_TURNT_FN:
        turntable_apply_function(value);
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
            TEMP_control_word |= 0x0002;    /* 使能制冷 */
            if (refrigeration_target == 0) {
                refrigeration_target = 1000; /* 默认 10.00°C */
            }
            LOG_I("Cooling: ON");
        } else {
            TEMP_control_word &= ~0x0002;
            refrigeration_target = 0;
            LOG_I("Cooling: OFF");
        }
        break;

    /* ── 0x0010: 光强读数 (只读) ── */
    case R_LIGHT:
        if (value == 0x8000) {
            photometer_ch0 = 0x0018;
            LOG_I("Light: TRIGGER");
        } else {
            LOG_D("Light write/readback: 0x%04X", value);
        }
        break;

    /* ── 0x00BF / 协议 0192: 转盘加热目标温度，值已按 x100 放大 ── */
    case R_HEAT_TARGET:
        heating_target = (UNS32)value;
        LOG_I("Heat target: %d.%02d°C", value / 100, value % 100);
        break;

    /* ── 0x00C0 / 协议 0193: 制冷目标温度，值已按 x100 放大 ── */
    case R_COOL_TARGET:
        refrigeration_target = (UNS32)value;
        LOG_I("Cool target: %d.%02d°C", value / 100, value % 100);
        break;

    /* ── 0x0078: 称重控制 ── */
    case R_WEIGH:
        LOG_I("Weigh ctrl: 0x%04X", value);
        /* 0x0006 = 标定+去皮+容量 */
        break;

    default:
        /* 上位机写孔位寄存器 0x0001~0x0003，禁止 CAN 覆盖 */
        if (addr <= 2 && g_hole_written == 0) {
            g_hole_written = 1;
            LOG_I("Hole locked by上位机");
        }
        break;
    }
send_pdo:
    /* OD 变量已改，立即触发 PDO 发送 */
    sendPDOevent(&Master_Data);
}

/* ═══════════════════════════════════════════════════════════════
 *  初始化
 * ═══════════════════════════════════════════════════════════════ */
int sampling_init(void)
{
    g_cur_slot = 0;
    g_hole_written = 0;

    /* 初始状态: 电机空闲 */
    mY_position     = SLOT_IDLE_POS;
    mY_control_word = 0;
    mY_modes        = 0;
    mX_modes        = 0;
    mX_control_word = 0;
    mZ_control_word = 0;
    mE_control_word = 0;
    photometer_ch0  = 0;
    photometer_ch1  = 0;

    /* 孔位状态初始化 —— 上位机通过读 0x0001 判设备就绪 */
    {
        uint8_t hole_init[6] = {0xFC, 0x00, 0xFF, 0x00, 0x73, 0x00};
        reg_write_local(R_HOLE0, 3, hole_init);
    }

    /* 设备状态初始化 —— 上位机通过读 0x0012 判断仪器状态
     * 必须在 Modbus 使能前设好默认值, 否则上位机读到全 0 会走错初始化流程
     *
     * 四重保险: OD变量 + regs[] + usSRegHoldBuf[] 全部设 */
    mX_status_word = 0x1C00;  /* OD 变量 (index 0x2004) */
    {
        uint8_t dev_stat[4] = {0x1C, 0x00, 0x00, 0x01};  /* 0x1C00 0001 */
        reg_write_local(R_DEV_STAT, 2, dev_stat);
    }
    usSRegHoldBuf[R_DEV_STAT] = 0x1C00;  /* 兜底: Modbus 本地缓存 */
    usSRegHoldBuf[R_DEV_STAT + 1] = 0x0001;

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
    if (argc < 2) {
        rt_kprintf("Usage: mbreg <addr> <value>\n");
        rt_kprintf("  Cool ON:    mbreg 176 1\n");
        rt_kprintf("  Cool OFF:   mbreg 176 0\n");
        rt_kprintf("  Heat temp:  mbreg 88 25\n");
        rt_kprintf("  Cool temp:  mbreg 89 10\n");
        rt_kprintf("  Motor STOP: mbreg 74 0x8000\n");
        rt_kprintf("  Motor GO:   mbreg 74 0x9000\n");
        rt_kprintf("  Pump ON:    mbreg 173 0x2000\n");
        rt_kprintf("  Read reg:   mbreg <addr>\n");
        rt_kprintf("  (addr = 0-based decimal, 0x for hex)\n");
        return 0;
    }
    uint16_t addr = (uint16_t)strtoul(argv[1], NULL, 0);

    if (argc >= 3) {
        /* 写入 */
        uint16_t val = (uint16_t)strtoul(argv[2], NULL, 0);
        uint8_t data[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
        reg_write(addr, 1, data);
        rt_kprintf("  WRITE reg[%d] = 0x%04X (%d)\n", addr, val, val);
    } else {
        /* 读取 */
        uint8_t buf[4];
        reg_read(addr, 1, buf);
        uint16_t v = ((uint16_t)buf[0] << 8) | buf[1];
        rt_kprintf("  READ  reg[%d] = 0x%04X (%d)\n", addr, v, v);
    }
    return 0;
}
MSH_CMD_EXPORT(mbreg, Modbus register read/write test);
#endif
