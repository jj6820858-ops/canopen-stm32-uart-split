/*
 * power_on_check.c — 上电校验 (匹配 上电校验4.csv)
 *
 * 成品精确时序 (10ms/tick):
 *   t=0ms:      NMT → Node 1,4,7
 *   t=10ms:     SDO 链 (7个, ~0.35s完成)
 *   t=10ms:     N3 0→780          @20/tick (0.36s)
 *   t=380ms:    N3 780→-2660      @20/tick (1.7s)
 *   t=2.0s:     N3 bounce→settle  @+20→-2
 *   t=3.3s:     N1 start-315      @15/tick, N2 start-106 @10/tick
 *   t=3.7s:     N1 reverse +15    @15/tick (过0), N2 reverse +10 @10/tick
 *   t=5.5s:     N1 reverse -15    @15/tick, N2 micro -1  @1/tick
 *   t=6.0s:     N1 micro +1       @1/tick
 *   t=7.3s:     N4 → -32003       @30/tick
 *   t=11.0s:    STOP → 全零
 *   t=20.0s:    SDO restore N1=300
 *   t=23.0s:    DONE
 */
#include "power_on_check.h"
#include "../canopen/canopen_master.h"
#include <rtthread.h>
#include <string.h>
#include "sdo.h"
#include "nmtMaster.h"
#include "pdo.h"
#include "can_driver.h"           /* canSend() */

extern INTEGER32 mX_position, mY_position, mZ_position, mE_position;
extern INTEGER32 mX_velocity, mY_velocity, mZ_velocity, mE_velocity;
extern UNS16 photometer_ch0, photometer_ch1;
extern UNS32 heating_target, refrigeration_target;
extern INTEGER8 mT_modes;  /* 转盘功能使能 */
extern UNS32 TEMP_control_word;  /* 温控控制字 */

#define DBG_TAG "pwrchk"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* ── 运动参数 (10ms/tick, 精确匹配成品) ── */
#define VELOCITY       0x0003E800   /* N1/N2/N3 velocity */
#define N3_UP_STEP     16           /* N3 上升 step/tick */
#define N3_UP_D        780          /* N3 上升幅度 */
#define N3_DN_FAST_D   800          /* N3 快速下降幅度 (780→-20) */
#define N3_DN_SLOW_D   1640         /* N3 慢速续降幅度 (-20→-2660) */
#define N3_DN_STEP     16           /* N3 下降 step/tick (快速段) */
#define N3_BOUNCE_D    98           /* N3 反弹幅度 (-2660→-2562) */
#define N3_MICRO_STEP  2            /* N3 微调 step/tick */
#define N1_STEP        15           /* N1 step/tick */
#define N1_GO_D        660          /* N1 GO 偏移 */
#define N1_BACK_D      2200         /* N1 BACK 反弹量 (-660→1995) */
#define N1_RET_D       104          /* N1 RETURN 回退量 (1995→1891) */
#define N2_STEP        10           /* N2 step/tick */
#define N2_GO_D        1360         /* N2 GO 偏移 */
#define N2_BACK_D      1370         /* N2 BACK 反弹量 (-1360→10) */
#define N2_RET_D       11           /* N2 RETURN 回退量 (10→-1) */
#define N4_STEP        100          /* N4 step/tick */
#define E4_POS         ((INTEGER32)0xFFFF82FD)  /* Node4 -32003 */
#define E4_VEL         0x00019000

/* ── 时间触发 (ms) ── */
#define T_N12_START    3250
#define T_N4_START     7280
#define T_STOP         10500
#define T_SDO_RESTORE  11500
#define T_DONE         12500

/* ── N12 子阶段 ── */
#define N12_GO         0    /* 向目标移动 */
#define N12_BACK       1    /* 反弹过0 */
#define N12_RETURN     2    /* 再次反转 */
#define N12_MICRO      3    /* 微调 */

/* ── 状态机 ── */
typedef enum {
    P_IDLE = 0,
    P_0_NMT,
    P_1_N3_UP,
    P_2_N3_DN,
    P_3_N3_SETTLE,
    P_4_N12,
    P_5_N4,
    P_6_STOP,
    P_7_SDO_RESTORE,
    P_DONE,
} phase_t;

static phase_t    g_p  = P_IDLE;
static rt_timer_t g_tm = RT_NULL;
static rt_tick_t  g_t0 = 0;
static int        g_st = 0;
static int32_t    g_n3 = 0, g_n3_tgt = 0, g_n3_start = 0;
static int32_t    g_n1 = 0, g_n1_tgt = 0, g_n1_start = 0;
static int32_t    g_n2 = 0, g_n2_tgt = 0, g_n2_start = 0;
static int32_t    g_n4 = 0;
static int        g_n12_sub = N12_GO;

static rt_tick_t ms(void) { return (rt_tick_get()-g_t0)*1000/RT_TICK_PER_SECOND; }
static void enter(phase_t p) { g_p=p; g_t0=rt_tick_get(); g_st=0; LOG_I("P%d",(int)p); }

/* ── SDO 链 ── */
typedef struct {
    UNS8  node; UNS16 index; UNS8  sub; UNS32 value;
} sdo_cmd_t;

static const sdo_cmd_t sdo_chain_init[] = {
    {2, 0x6083, 0, 50},  {1, 0x6083, 0, 300}, {3, 0x6083, 0, 500},
    {3, 0x6084, 0, 500}, {1, 0x6084, 0, 100}, {1, 0x6083, 0, 100},
    {1, 0x6084, 0, 100},
};
static const sdo_cmd_t sdo_chain_restore[] = {
    {1, 0x6083, 0, 300}, {1, 0x6084, 0, 300}, {1, 0x6083, 0, 300},
};

static const sdo_cmd_t *g_sdo_chain      = NULL;
static int               g_sdo_chain_cnt  = 0;
static int               g_sdo_chain_idx  = 0;
static int               g_sdo_chain_errs = 0;

static void sdo_chain_next(CO_Data* d, UNS8 prev_node)
{
    UNS32 abortCode = 0;
    if (g_sdo_chain_idx > 0) {
        getWriteResultNetworkDict(d, prev_node, &abortCode);
        if (abortCode) {
            LOG_E("SDO[%d] node=0x%02X abort 0x%08lX",
                  g_sdo_chain_idx - 1, prev_node, (unsigned long)abortCode);
            g_sdo_chain_errs++;
        }
    }
    while (g_sdo_chain_idx < g_sdo_chain_cnt) {
        const sdo_cmd_t *c = &g_sdo_chain[g_sdo_chain_idx];
        int my_idx = g_sdo_chain_idx;
        g_sdo_chain_idx++;
        UNS32 v = c->value;
        UNS8 ret = writeNetworkDictCallBack(&Master_Data, c->node, c->index,
                                             c->sub, 4, 0, &v, sdo_chain_next, 0);
        if (ret == 0) {
            LOG_D("SDO[%d] n=0x%02X idx=0x%04X=%lu",
                  my_idx, c->node, (unsigned long)c->index, (unsigned long)c->value);
            return;
        }
        LOG_E("SDO[%d] send fail n=0x%02X ret=0x%02X", my_idx, c->node, ret);
        g_sdo_chain_errs++;
        getWriteResultNetworkDict(d, c->node, &abortCode);
    }
    LOG_I("SDO chain done: %d/%d ok", g_sdo_chain_cnt - g_sdo_chain_errs, g_sdo_chain_cnt);
}

static void sdo_chain_start(const sdo_cmd_t *chain, int count)
{
    g_sdo_chain = (sdo_cmd_t *)chain; g_sdo_chain_cnt = count;
    g_sdo_chain_idx = 0; g_sdo_chain_errs = 0;
    sdo_chain_next(&Master_Data, 0);
}

/* 直接发温控 PDO 帧 */
static void send_temp_pdo(void)
{
    Message m;
    memset(&m, 0, sizeof(m));
    m.rtr = 0;

    /* TPDO12 → COB 0x207: heating_target + refrigeration_target */
    m.cob_id = 0x207;
    m.len = 8;
    m.data[0] = (UNS8)(heating_target & 0xFF);
    m.data[1] = (UNS8)((heating_target >> 8) & 0xFF);
    m.data[2] = (UNS8)((heating_target >> 16) & 0xFF);
    m.data[3] = (UNS8)((heating_target >> 24) & 0xFF);
    m.data[4] = (UNS8)(refrigeration_target & 0xFF);
    m.data[5] = (UNS8)((refrigeration_target >> 8) & 0xFF);
    m.data[6] = (UNS8)((refrigeration_target >> 16) & 0xFF);
    m.data[7] = (UNS8)((refrigeration_target >> 24) & 0xFF);
    canSend(Master_Data.canHandle, &m);

    /* TPDO13 → COB 0x307: TEMP_control_word */
    m.cob_id = 0x307;
    m.len = 4;
    m.data[0] = (UNS8)(TEMP_control_word & 0xFF);
    m.data[1] = (UNS8)((TEMP_control_word >> 8) & 0xFF);
    m.data[2] = (UNS8)((TEMP_control_word >> 16) & 0xFF);
    m.data[3] = (UNS8)((TEMP_control_word >> 24) & 0xFF);
    canSend(Master_Data.canHandle, &m);
}

/* 直接发转盘加热使能到 0x203 和 0x205 */
static void send_heat_enable(void)
{
    Message m;
    memset(&m, 0, sizeof(m));
    m.rtr = 0;

    /* TPDO5 → COB 0x203 (Node3): mT_status_word(2B) + mT_modes(1B) */
    m.cob_id = 0x203;
    m.len = 3;
    m.data[0] = 0; m.data[1] = 0;  /* mT_status_word = 0 */
    m.data[2] = (UNS8)mT_modes;     /* mT_modes */
    canSend(Master_Data.canHandle, &m);

    /* TPDO9 → COB 0x205 (Node5): mT_modes(1B) + mT_control_word(2B) */
    m.cob_id = 0x205;
    m.len = 3;
    m.data[0] = (UNS8)mT_modes;     /* mT_modes */
    m.data[1] = 0; m.data[2] = 0;   /* mT_control_word = 0 */
    canSend(Master_Data.canHandle, &m);
}

static void nmt(UNS8 n) { masterSendNMTstateChange(&Master_Data,n,NMT_Start_Node); }
static void rpdo0(void) {
    mX_position=mY_position=mZ_position=mE_position=0;
    mX_velocity=mY_velocity=mZ_velocity=mE_velocity=0;
    photometer_ch0=photometer_ch1=0;
}
static int32_t ramp(int32_t cur, int32_t tgt, int32_t step) {
    if(cur<tgt){cur+=step;if(cur>tgt)cur=tgt;}
    else if(cur>tgt){cur-=step;if(cur<tgt)cur=tgt;}
    return cur;
}

static void timer_cb(void *p)
{
    rt_tick_t m = ms();
    switch (g_p) {

    /* ── P0: NMT ── */
    case P_0_NMT:
        if (g_st==0) { nmt(1); nmt(4); nmt(7); g_st=1; LOG_I("NMT 1,4,7"); }
        if (m>=1) enter(P_1_N3_UP);
        break;

    /* ── P1: SDO + N3 上升 0→780 ── */
    case P_1_N3_UP:
        if (g_st==0) {
            sdo_chain_start(sdo_chain_init,
                            sizeof(sdo_chain_init)/sizeof(sdo_chain_init[0]));
            LOG_I("SDO: N2=50 N1=300/100 N3=500/500");
            g_n3_start = mZ_position;
            g_n3 = g_n3_start;
            g_n3_tgt = g_n3_start + N3_UP_D;
            photometer_ch0 = 0x0018;
            TEMP_control_word = 0x0003;     /* bit0=加热ON, bit1=制冷ON */
            heating_target = 0x09C4;       /* 25.00°C (2500) */
            refrigeration_target = 0x03E8;  /* 10.00°C (1000) */
            mT_modes = 0x01;                /* 转盘加热使能 */
            send_temp_pdo();                /* → 0x207 + 0x307 */
            send_heat_enable();             /* → 0x203 + 0x205 */
            LOG_I("Heat=25C Cool=10C");
            g_n4 = mE_position;
            g_st=1;
        }
        g_n3 = ramp(g_n3, g_n3_tgt, N3_UP_STEP);
        mZ_position = g_n3;
        mZ_velocity = VELOCITY;
        if (g_n3 >= g_n3_tgt) enter(P_2_N3_DN);
        break;

    /* ── P2: N3 下降 两段: 快速800 + 慢速1000 ── */
    case P_2_N3_DN:
        if (g_st==0) {
            /* A 段: 快速下降 800 */
            g_n3_tgt = g_n3 - N3_DN_FAST_D;
            g_st = 1;
        }
        if (g_st==1) {
            g_n3 = ramp(g_n3, g_n3_tgt, N3_DN_STEP);
            mZ_position = g_n3;
            mZ_velocity = VELOCITY;
            if (g_n3 <= g_n3_tgt) {
                /* B 段: 继续慢速下降 */
                g_n3_tgt = g_n3 - N3_DN_SLOW_D;
                g_st = 2;
            }
        } else {
            g_n3 = ramp(g_n3, g_n3_tgt, N3_DN_STEP);  /* 20/tick 和成品一致 */
            mZ_position = g_n3;
            mZ_velocity = VELOCITY;
            if (g_n3 <= g_n3_tgt) enter(P_3_N3_SETTLE);
        }
        break;

    /* ── P3: N3 反弹+微调，等待 N1/N2 启动时间 ── */
    case P_3_N3_SETTLE:
        if (g_st==0) {
            g_n3_tgt = g_n3 + N3_BOUNCE_D;  /* 反弹 */
            g_st=1;
        }
        if (g_st==1) {
            /* 反弹阶段: +20/tick */
            g_n3 = ramp(g_n3, g_n3_tgt, N3_DN_STEP);
            mZ_position = g_n3;
            mZ_velocity = VELOCITY;
            if (g_n3 >= g_n3_tgt) g_st=2;
        } else {
            /* 微调: -2/tick 直到时间到 */
            g_n3 -= N3_MICRO_STEP;
            mZ_position = g_n3;
            mZ_velocity = 0x00053555;
        }
        /* 每秒重发一次温控 PDO */
        if ((m % 1000) == 0) {
            send_temp_pdo();
            send_heat_enable();
        }

        if (m >= T_N12_START) enter(P_4_N12);
        break;

    /* ── P4: N1/N2 运动 (含反弹) ── */
    case P_4_N12:
        if (g_st==0) {
            g_n1_start = mX_position;
            g_n2_start = mY_position;
            g_n1 = g_n1_start;
            g_n2 = g_n2_start;
            g_n1_tgt = g_n1_start - N1_GO_D;
            g_n2_tgt = g_n2_start - N2_GO_D;
            g_n12_sub = N12_GO;
            g_st=1;
            LOG_D("N12 start: n1=%ld→%ld n2=%ld→%ld",
                  (long)g_n1_start, (long)g_n1_tgt,
                  (long)g_n2_start, (long)g_n2_tgt);
        }

        switch (g_n12_sub) {
        case N12_GO:
            g_n1 = ramp(g_n1, g_n1_tgt, N1_STEP);
            g_n2 = ramp(g_n2, g_n2_tgt, N2_STEP);
            if (g_n1 <= g_n1_tgt && g_n2 <= g_n2_tgt) {
                g_n1_tgt = g_n1 + N1_BACK_D;
                g_n2_tgt = g_n2 + N2_BACK_D;
                g_n12_sub = N12_BACK;
                LOG_D("N12 back: n1→%ld n2→%ld",
                      (long)g_n1_tgt, (long)g_n2_tgt);
            }
            break;
        case N12_BACK:
            g_n1 = ramp(g_n1, g_n1_tgt, N1_STEP);
            g_n2 = ramp(g_n2, g_n2_tgt, N2_STEP);
            if (g_n1 >= g_n1_tgt && g_n2 >= g_n2_tgt) {
                g_n1_tgt = g_n1 - N1_RET_D;
                g_n2_tgt = g_n2 - N2_RET_D;
                g_n12_sub = N12_RETURN;
                LOG_D("N12 ret: n1→%ld n2→%ld",
                      (long)g_n1_tgt, (long)g_n2_tgt);
            }
            break;
        case N12_RETURN:
            g_n1 = ramp(g_n1, g_n1_tgt, N1_STEP);
            g_n2 = ramp(g_n2, g_n2_tgt, N2_STEP);
            if (g_n1 <= g_n1_tgt && g_n2 <= g_n2_tgt) {
                g_n12_sub = N12_MICRO;
                LOG_D("N12 micro");
            }
            break;
        case N12_MICRO:
            g_n1 += 1; g_n2 -= 1;
            break;
        }

        mX_position = g_n1; mX_velocity = VELOCITY;
        mY_position = g_n2; mY_velocity = VELOCITY;
        /* N3 保持微调 */
        g_n3 -= N3_MICRO_STEP;
        mZ_position = g_n3;
        mZ_velocity = 0x00053555;

        /* N12 MICRO 跑 1s 后进入 N4 */
        if (g_n12_sub == N12_MICRO && m >= 1000) enter(P_5_N4);
        break;

    /* ── P5: N4 下降 ── */
    case P_5_N4:
        if (g_st==0) {
            LOG_I("N4: %ld→%ld", (long)g_n4, (long)E4_POS);
            g_st=1;
        }
        g_n4 = ramp(g_n4, (int32_t)E4_POS, N4_STEP);
        mE_position = g_n4;
        mE_velocity = E4_VEL;
        /* N1/N2/N3 保持 */
        mX_position = g_n1;
        mY_position = g_n2;
        mZ_position = g_n3;
        /* N4 到达目标即 STOP */
        if (g_n4 <= (int32_t)E4_POS) enter(P_6_STOP);
        break;

    /* ── P6: STOP ── */
    case P_6_STOP:
        rpdo0(); LOG_I("STOP"); enter(P_7_SDO_RESTORE);
        break;

    /* ── P7: SDO 恢复 (每200ms发一个，避免同节点冲突) ── */
    case P_7_SDO_RESTORE:
        if (g_st==0) {
            LOG_I("SDO restore: N1=300/300");
            g_st = 1;
        }
        /* 每100ms发一个SDO，等上一个完成 */
        {
            int idx = (m / 100);
            if (idx < (int)(sizeof(sdo_chain_restore)/sizeof(sdo_chain_restore[0]))
                && (m % 100) == 0 && idx >= g_st - 1)
            {
                const sdo_cmd_t *c = &sdo_chain_restore[idx];
                UNS32 v = c->value;
                UNS8 ret = writeNetworkDict(&Master_Data, c->node, c->index,
                                            c->sub, 4, 0, &v, 0);
                if (ret == 0) {
                    LOG_D("SDO restore[%d] ok", idx);
                    g_st = idx + 2;  /* 推进到下一条 */
                } else {
                    LOG_E("SDO restore[%d] fail ret=0x%02X", idx, ret);
                }
            }
        }
        if (m >= 1500) enter(P_DONE);
        break;

    /* ── DONE ── */
    case P_DONE:
        if (g_st==0) { rpdo0(); LOG_I("DONE"); g_st=1; }
        if (g_tm) { rt_timer_stop(g_tm); rt_timer_delete(g_tm); g_tm=RT_NULL; }
        break;

    default: break;
    }
    sendPDOevent(&Master_Data);
}

/* 对外接口 */
#define PWRCHK_VER "20260706_2045"  /* 版本: 上位机孔位锁定 */
int power_on_check_init(void) { LOG_I("ver %s", PWRCHK_VER); g_p=P_IDLE; return 0; }
void power_on_check_start(void) {
    if (g_p!=P_IDLE&&g_p!=P_DONE) return;
    LOG_I("START"); rpdo0(); g_n3=g_n1=g_n2=g_n4=0;
    g_tm=rt_timer_create("pwrchk",timer_cb,RT_NULL,rt_tick_from_millisecond(10),
                         RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
    if(g_tm){enter(P_0_NMT);rt_timer_start(g_tm);}
}
void power_on_check_stop(void) {
    if(g_tm){rt_timer_stop(g_tm);rt_timer_delete(g_tm);g_tm=RT_NULL;}
    rpdo0(); g_p=P_IDLE;
}
int power_on_check_is_running(void){return g_p>P_IDLE&&g_p<P_DONE;}
int power_on_check_get_phase(void){return(int)g_p;}
const char *power_on_check_get_phase_name(void) {
    static const char *n[]={"IDLE","NMT","N3_UP","N3_DN","N3_SETTLE",
        "N12","N4","STOP","SDO_RESTORE","DONE"};
    return n[(int)g_p<=P_DONE?(int)g_p:0];
}
#ifdef RT_USING_FINSH
#include <finsh.h>
static int pwrchk(int argc,char**argv){
    if(argc<2){rt_kprintf("pwrchk start|stop|status\n");return 0;}
    if(!strcmp(argv[1],"start")) power_on_check_start();
    else if(!strcmp(argv[1],"stop")) power_on_check_stop();
    else if(!strcmp(argv[1],"status"))
        rt_kprintf("%s(%d) n1=%ld(%ld) n2=%ld(%ld) n3=%ld n4=%ld\n",
                   power_on_check_get_phase_name(),(int)g_p,
                   (long)g_n1,(long)g_n1_tgt,(long)g_n2,(long)g_n2_tgt,
                   (long)g_n3,(long)g_n4);
    return 0;
}
MSH_CMD_EXPORT(pwrchk, power-on check);
#endif
