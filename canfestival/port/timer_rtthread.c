/* 避免 GCC14 newlib signal.h 与 RT-Thread libc_signal.h 冲突 */
#define _SIGNAL_H_

#include <rtthread.h>
#include "timer_rtthread.h"
#include "timerscfg.h"
#include "timers.h"              /* TimeDispatch() */

static rt_timer_t g_timer = RT_NULL;

TIMEVAL getElapsedTime(void)
{
    return (TIMEVAL)rt_tick_get();
}

static void timer_dispatch(void *param)
{
    (void)param;
    TimeDispatch();
}

void setTimer(TIMEVAL interval)
{
    if (g_timer) {
        rt_timer_stop(g_timer);
        /* 限制到 RT_TICK_MAX/2 - 1，避免 rt_timer_start 断言 */
        rt_tick_t clamped = (interval < RT_TICK_MAX / 2)
                            ? interval : (RT_TICK_MAX / 2 - 1);
        rt_timer_control(g_timer, RT_TIMER_CTRL_SET_TIME,
                         (void *)&clamped);
        rt_timer_start(g_timer);
    }
}

TIMEVAL canDelTimer(TIMEVAL interval)
{
    (void)interval;
    return 0;
}

void TimerInit(void)
{
    /* 周期性软定时器，驱动 CANopen TimeDispatch */
    g_timer = rt_timer_create("cantimer", timer_dispatch,
                              RT_NULL, 10,
                              RT_TIMER_FLAG_PERIODIC |
                              RT_TIMER_FLAG_SOFT_TIMER);
}

void TimerCleanup(void)
{
    if (g_timer) {
        rt_timer_stop(g_timer);
        rt_timer_delete(g_timer);
        g_timer = RT_NULL;
    }
}

/* 当前移植层未使用互斥锁 */
void EnterMutex(void) { }
void LeaveMutex(void) { }

void StartTimerLoop(void (*callback)(CO_Data *, UNS32))
{
    (void)callback;
    TimerInit();
}

void StopTimerLoop(void (*callback)(CO_Data *, UNS32))
{
    (void)callback;
    TimerCleanup();
}
