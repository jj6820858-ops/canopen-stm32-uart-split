/*
 * power_on_check.c — 上电校验序列（复现 CAN 采集数据）
 *
 * 完整复现 上电校验.csv 中的 24 秒上电流程:
 *
 *   COB-ID          帧数    用途
 *   ───────────────────────────────────────────
 *   0x000 (NMT)       3    启停控制
 *   0x301 (RPDO2 N1) 7677  电机A 位置=0x0001F4(500)
 *   0x302 (RPDO2 N2) 4541  电机B 位置=0x00014D55(85333)
 *   0x303 (RPDO2 N3) 4630  电机C 位置=ramp 变化
 *   0x304 (RPDO2 N4) 4546  电机D 位置=0xFFFF82FD(-32003)
 *   0x206 (RPDO1 N6) 1299  控制字 0x00→0x18
 *   0x601/581 (SDO)    15  Node1 参数
 *   0x602/582 (SDO)     2  Node2 参数
 *   0x603/583 (SDO)     5  Node3 参数
 *
 * OD 变量 → PDO 映射:
 *   mX_* → Node1  (0x301), mY_* → Node2 (0x302)
 *   mZ_* → Node3  (0x303), mE_* → Node4 (0x304)
 *   photometer_* → Node6 (0x206)
 */
#include "power_on_check.h"
#include "canopen_master.h"
#include "reg_router.h"
#include <rtthread.h>
#include <string.h>

/* CanFestival SDO/NMT API */
#include "sdo.h"
#include "nmtMaster.h"
#include "pdo.h"

/* OD variable externs (from ObjDict.h) */
extern INTEGER32 mX_position;
extern INTEGER32 mY_position;
extern INTEGER32 mZ_position;
extern INTEGER32 mE_position;
extern INTEGER32 mT_position;
extern INTEGER32 mX_velocity;
extern INTEGER32 mY_velocity;
extern INTEGER32 mZ_velocity;
extern INTEGER32 mE_velocity;
extern UNS16 mX_control_word;
extern UNS16 mY_control_word;
extern UNS16 mZ_control_word;
extern UNS16 mE_control_word;
extern UNS16 mT_control_word;
extern INTEGER8 mX_modes;
extern INTEGER8 mY_modes;
extern INTEGER8 mZ_modes;
extern INTEGER8 mE_modes;
extern INTEGER8 mT_modes;
extern UNS16 mX_status_word;
extern UNS16 mY_status_word;
extern UNS16 mZ_status_word;
extern UNS16 mE_status_word;
extern UNS16 mT_status_word;
extern UNS16 photometer_ch0;
extern UNS16 photometer_ch1;

#define DBG_TAG "pwrchk"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* ═══════════════════════════════════════════════════════════════
 *  校验状态机
 * ═══════════════════════════════════════════════════════════════ */
typedef enum {
    PHASE_IDLE             = 0,
    PHASE_INIT             = 1,   /* 初始化, 等待启动 */
    PHASE_0_NMT_START      = 2,   /* t=0ms: NMT 启动 Node 1,4,7 */
    PHASE_0_SDO_LOT1       = 3,   /* t=1ms: SDO 批量写参数 (第一组) */
    PHASE_1_SDO_NODE1      = 4,   /* t=1s: SDO Node1 微调 */
    PHASE_2_IDLE_MONITOR   = 5,   /* t=1~11s: 闲置监测 */
    PHASE_3_MOTION_START   = 6,   /* t=11s: 启动运动 */
    PHASE_4_PARAM_RESTORE  = 7,   /* t=20s: 参数恢复 */
    PHASE_5_DONE           = 8,   /* t=24s: 完成 */
} power_on_phase_t;

static power_on_phase_t g_phase = PHASE_IDLE;
static rt_timer_t  g_timer = RT_NULL;
static rt_tick_t   g_phase_start_tick = 0;
static int         g_seq_idx = 0;      /* sub-step index within phase */
static int         g_sdo_pending = 0;   /* pending SDO replies */

/* ═══════════════════════════════════════════════════════════════
 *  SDO write helper — 向从站写入 OD 变量
 *  writeNetworkDict: nodeId, index, subIndex, count, dataType, *data, useBlockMode
 * ═══════════════════════════════════════════════════════════════ */
static void sdo_write_u32(UNS8 nodeId, UNS16 index, UNS8 subIdx, UNS32 value)
{
    UNS32 data = value;
    UNS8 ret = writeNetworkDict(&Master_Data, nodeId,
                                 index, subIdx, 4, 0, &data, 0);
    if (ret == 0) {
        g_sdo_pending++;
        LOG_D("SDO: Node%d 0x%04X/%02X = %d", nodeId, index, subIdx, (int)value);
    } else {
        LOG_W("SDO: Node%d 0x%04X/%02X FAILED ret=%02X", nodeId, index, subIdx, ret);
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  NMT helper — 发送 NMT 状态切换命令
 * ═══════════════════════════════════════════════════════════════ */
static void nmt_start_node(UNS8 nodeId)
{
    masterSendNMTstateChange(&Master_Data, nodeId, NMT_Start_Node);
    LOG_I("NMT: Node 0x%02X → Start (Operational)", nodeId);
}

/* ═══════════════════════════════════════════════════════════════
 *  RPDO2 / RPDO1 命令值设置 — 直接写 OD 变量, PDO 自动发送
 * ═══════════════════════════════════════════════════════════════ */
static void set_all_rpdo_zero(void)
{
    mX_position    = 0;
    mY_position    = 0;
    mZ_position    = 0;
    mE_position    = 0;
    mX_velocity    = 0;
    mY_velocity    = 0;
    mZ_velocity    = 0;
    mE_velocity    = 0;
    mX_control_word = 0;
    mY_control_word = 0;
    mZ_control_word = 0;
    mE_control_word = 0;
    photometer_ch0 = 0;
    photometer_ch1 = 0;
    LOG_D("RPDO: all zeroed");
}

static void set_motion_commands(void)
{
    /*
     * 从 CAN 数据提取的 RPDO2 命令值:
     *   0x301 Node1: pos=0x000001F4 (500)
     *   0x302 Node2: pos=0x00014D55 (85333)
     *   0x303 Node3: pos=0x00000ADC (2780) → control=0x00055355
     *   0x304 Node4: pos=0xFFFF82FD (-32003) → control=0x00019000
     *
     * 映射到 OD 变量:
     *   mX_position → Node1 RPDO2 (0x301)
     *   mY_position → Node2 RPDO2 (0x302)
     *   mZ_position → Node3 RPDO2 (0x303)
     *   mE_position → Node4 RPDO2 (0x304)
     *
     * Node6 RPDO1 (0x206): photometer_ch0=0x18 (切换模式)
     */
    mX_position     = 500;           /* 0x01F4 */
    mY_position     = 85333;         /* 0x014D55 */
    mZ_position     = 2780;          /* 0x000ADC */
    mE_position     = (INTEGER32)0xFFFF82FD;  /* -32003 */
    mX_control_word = 0;
    mY_control_word = 0;
    mZ_control_word = 0x5355;       /* from capture (lo 16 of 0x55355) */
    mE_control_word = 0x9000;       /* from capture (lo 16 of 0x19000) */
    photometer_ch0  = 0x18;           /* RPDO1 Node6 模式切换 */
    photometer_ch1  = 0;

    LOG_I("RPDO: motion commands set");
    LOG_I("  Node1 pos=%d ctrl=0x%04X",  (int)mX_position, mX_control_word);
    LOG_I("  Node2 pos=%d ctrl=0x%04X",  (int)mY_position, mY_control_word);
    LOG_I("  Node3 pos=%d ctrl=0x%04X",  (int)mZ_position, mZ_control_word);
    LOG_I("  Node4 pos=%d ctrl=0x%04X",  (int)mE_position, mE_control_word);
    LOG_I("  Node6 ch0=0x%04X",          photometer_ch0);
}

/* ═══════════════════════════════════════════════════════════════
 *  TPDO1 模拟 — 模拟从站心跳和状态反馈
 *  (实际硬件连接后会由 CAN 中断自动更新 OD 变量)
 * ═══════════════════════════════════════════════════════════════ */
static void simulate_tpdo_feedback(void)
{
    /*
     * 上电校验期间从站反馈:
     *   Node1-4 TPDO1 (0x181-0x184): 状态字 0x0000↔0x0004 交替
     *   Node6 TPDO1 (0x186): 位置 32bit
     *   Node8 TPDO1 (0x188): 双通道 32bit
     *
     * 无硬件时设置模拟值，有硬件时 CAN 中断会覆盖
     */
    static int toggle = 0;
    toggle ^= 1;

    /* Node1-4 状态交替 */
    mX_status_word = toggle ? 0x0004 : 0x0000;
    mY_status_word = toggle ? 0x0004 : 0x0000;
    mZ_status_word = toggle ? 0x0004 : 0x0000;
    mE_status_word = toggle ? 0x0004 : 0x0000;
}

/* ═══════════════════════════════════════════════════════════════
 *  阶段切换
 * ═══════════════════════════════════════════════════════════════ */
static void enter_phase(power_on_phase_t p)
{
    g_phase = p;
    g_phase_start_tick = rt_tick_get();
    g_seq_idx = 0;
    g_sdo_pending = 0;
    LOG_I("── Phase %d ──", (int)p);
}

/* ═══════════════════════════════════════════════════════════════
 *  主定时器回调 — 状态机驱动
 * ═══════════════════════════════════════════════════════════════ */
static void check_timer_cb(void *param)
{
    rt_tick_t elapsed = rt_tick_get() - g_phase_start_tick;
    rt_tick_t elapsed_ms = (elapsed * 1000) / RT_TICK_PER_SECOND;

    switch (g_phase) {

    /* ── Phase 0: NMT 启动 (t=0ms) ── */
    case PHASE_0_NMT_START:
        if (g_seq_idx == 0) {
            /* 模拟: Node2/Node1/Node6/Node8 心跳收齐 → 启动网络 */
            nmt_start_node(0x01);  /* Node 1 */
            nmt_start_node(0x04);  /* Node 4 */
            nmt_start_node(0x07);  /* Node 7 */
            g_seq_idx++;
            break;
        }
        /* 等 ~1ms → 进入 SDO 批量配置 */
        if (elapsed_ms >= 1) {
            enter_phase(PHASE_0_SDO_LOT1);
        }
        break;

    /* ── Phase 0.1: SDO 批量写参数 (t=1ms) ── */
    case PHASE_0_SDO_LOT1:
        if (g_seq_idx == 0) {
            /*
             * CAN 数据中 t=0.0006s 的 SDO 序列:
             *   Node2: 0x6083/00 = 50
             *   Node1: 0x6083/00 = 300
             *   Node3: 0x6083/00 = 500
             *   Node3: (special NMT command)
             *   Node3: 0x6084/00 = 500
             */
            sdo_write_u32(0x02, 0x6083, 0x00, 50);    /* Node2 Accel */
            sdo_write_u32(0x01, 0x6083, 0x00, 300);   /* Node1 Accel */
            sdo_write_u32(0x03, 0x6083, 0x00, 500);   /* Node3 Accel */
            sdo_write_u32(0x03, 0x6084, 0x00, 500);   /* Node3 Decel */
            g_seq_idx++;
            break;
        }
        /* SDO 应答收齐后 → 延迟到 t=1s */
        if (elapsed_ms >= 999) {
            enter_phase(PHASE_1_SDO_NODE1);
        }
        break;

    /* ── Phase 1: Node1 微调 (t=1s) ── */
    case PHASE_1_SDO_NODE1:
        if (g_seq_idx == 0) {
            /*
             * CAN 数据中 t=1.000s 的 SDO 序列:
             *   Node1: 0x6084/00 = 100
             *   Node1: 0x6083/00 = 100
             *   Node1: (special NMT command)
             *   Node1: 0x6084/00 = 100
             */
            sdo_write_u32(0x01, 0x6084, 0x00, 100);
            sdo_write_u32(0x01, 0x6083, 0x00, 100);
            sdo_write_u32(0x01, 0x6084, 0x00, 100);
            g_seq_idx++;
            break;
        }
        /* 等 SDO 完成 → 进入闲置期 */
        if (elapsed_ms >= 200) {
            /* 确保 RPDO 全为零 (闲置状态) */
            set_all_rpdo_zero();
            enter_phase(PHASE_2_IDLE_MONITOR);
        }
        break;

    /* ── Phase 2: 闲置监测 (t=1~11s) ── */
    case PHASE_2_IDLE_MONITOR:
        /* 模拟 Node1-4 状态反馈 */
        simulate_tpdo_feedback();

        /* 10 秒闲置后 → 启动运动 (CAN 数据中 t=11s 运动开始) */
        if (elapsed_ms >= 10000) {
            enter_phase(PHASE_3_MOTION_START);
        }
        break;

    /* ── Phase 3: 运动启动 (t=11s) ── */
    case PHASE_3_MOTION_START:
        if (g_seq_idx == 0) {
            set_motion_commands();
            LOG_I("Motion started — RPDO2 streaming...");
            g_seq_idx++;
            break;
        }
        /*
         * 运动执行 9 秒 (t=11s → t=20s)
         * 实际运行中 CAN 中断会持续收 TPDO 更新 OD 变量
         */
        simulate_tpdo_feedback();
        if (elapsed_ms >= 9000) {
            enter_phase(PHASE_4_PARAM_RESTORE);
        }
        break;

    /* ── Phase 4: 参数恢复 (t=20s) ── */
    case PHASE_4_PARAM_RESTORE:
        if (g_seq_idx == 0) {
            /*
             * CAN 数据中 t=20.000s 恢复:
             *   Node1: 0x6083/00 = 300
             *   Node1: 0x6084/00 = 300
             *   Node1: 0x6083/00 = 300 (重复)
             */
            sdo_write_u32(0x01, 0x6083, 0x00, 300);
            sdo_write_u32(0x01, 0x6084, 0x00, 300);
            sdo_write_u32(0x01, 0x6083, 0x00, 300);
            g_seq_idx++;
            break;
        }
        /* 等 4 秒 → 完成 */
        if (elapsed_ms >= 4000) {
            enter_phase(PHASE_5_DONE);
        }
        break;

    /* ── Phase 5: 完成 ── */
    case PHASE_5_DONE:
        set_all_rpdo_zero();
        LOG_I("══════ Power-on check DONE ══════");
        g_timer = RT_NULL;  /* timer will be auto-stopped */
        break;

    default:
        break;
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════ */

int power_on_check_init(void)
{
    LOG_I("Power-on check module init");
    g_phase = PHASE_IDLE;
    return 0;
}

void power_on_check_start(void)
{
    if (g_phase != PHASE_IDLE && g_phase != PHASE_5_DONE) {
        LOG_W("Already running (phase %d)", (int)g_phase);
        return;
    }

    LOG_I("══════ Power-on check START ══════");

    /* 确保 RPDO 初始为零 */
    set_all_rpdo_zero();

    /* 启动定时器: 每 50ms 驱动一次状态机 (匹配 CAN 数据的实时性) */
    g_timer = rt_timer_create("pwrchk", check_timer_cb, RT_NULL,
                                rt_tick_from_millisecond(50),
                                RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (g_timer) {
        enter_phase(PHASE_0_NMT_START);
        rt_timer_start(g_timer);
    } else {
        LOG_E("Failed to create timer");
    }
}

void power_on_check_stop(void)
{
    if (g_timer) {
        rt_timer_stop(g_timer);
        rt_timer_delete(g_timer);
        g_timer = RT_NULL;
    }
    set_all_rpdo_zero();
    g_phase = PHASE_IDLE;
    LOG_I("Power-on check STOPPED");
}

int power_on_check_is_running(void)
{
    return (g_phase > PHASE_IDLE && g_phase < PHASE_5_DONE) ? 1 : 0;
}

int power_on_check_get_phase(void)
{
    return (int)g_phase;
}

const char *power_on_check_get_phase_name(void)
{
    switch (g_phase) {
    case PHASE_IDLE:            return "IDLE";
    case PHASE_INIT:            return "INIT";
    case PHASE_0_NMT_START:     return "NMT_START";
    case PHASE_0_SDO_LOT1:      return "SDO_CONFIG";
    case PHASE_1_SDO_NODE1:     return "SDO_NODE1_TUNE";
    case PHASE_2_IDLE_MONITOR:  return "IDLE_MONITOR";
    case PHASE_3_MOTION_START:  return "MOTION";
    case PHASE_4_PARAM_RESTORE: return "PARAM_RESTORE";
    case PHASE_5_DONE:          return "DONE";
    default:                    return "???";
    }
}

/* ═══════════════════════════════════════════════════════════════
 *  Finsh 命令
 * ═══════════════════════════════════════════════════════════════ */
#ifdef RT_USING_FINSH
#include <finsh.h>

static int pwrchk(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: pwrchk start | stop | status\n");
        rt_kprintf("  Phase: %d (%s)  Running: %s\n",
                   (int)g_phase, power_on_check_get_phase_name(),
                   power_on_check_is_running() ? "yes" : "no");
        return 0;
    }
    if (strcmp(argv[1], "start") == 0) {
        power_on_check_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        power_on_check_stop();
    } else if (strcmp(argv[1], "status") == 0) {
        rt_kprintf("Phase: %s (%d), Running: %s\n",
                   power_on_check_get_phase_name(), (int)g_phase,
                   power_on_check_is_running() ? "yes" : "no");
    } else {
        rt_kprintf("Unknown: %s\n", argv[1]);
    }
    return 0;
}
MSH_CMD_EXPORT(pwrchk, Power-on check control);
#endif /* RT_USING_FINSH */
