/*
 * power_on_check.h — 上电校验序列
 *
 * 复现 CAN 数据中采集到的上电校验完整流程:
 *   Phase 0 (t=0ms): NMT 启动 + SDO 批量配参
 *   Phase 1 (t=1s):  SDO Node1 微调
 *   Phase 2 (t=1-11s): 闲置监测
 *   Phase 3 (t=11s): 运动启动
 *   Phase 4 (t=20s): 参数恢复
 *   Phase 5 (t=24s): 完成
 */
#ifndef __POWER_ON_CHECK_H__
#define __POWER_ON_CHECK_H__

#include <rtthread.h>

/* ── Module lifecycle ── */
int  power_on_check_init(void);
void power_on_check_start(void);
void power_on_check_stop(void);

/* ── Status query ── */
int  power_on_check_is_running(void);
int  power_on_check_get_phase(void);
const char *power_on_check_get_phase_name(void);

#endif /* __POWER_ON_CHECK_H__ */
