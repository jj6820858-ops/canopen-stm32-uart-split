/* Prevent GCC14 newlib signal.h conflict with RT-Thread libc_signal.h */
#define _SIGNAL_H_

#include <rtthread.h>
#include "timer_rtthread.h"
#include "timerscfg.h"
#include "timers.h"              /* for TimeDispatch() */
#include "can_driver.h"          /* for canReceive() */
#include "states.h"              /* for canDispatch() */
#include "pdo.h"                 /* for sendPDOevent() */
/* Master_Data is declared in applications/canopen_master.h */
extern CO_Data Master_Data;

static rt_timer_t g_timer = RT_NULL;
static rt_thread_t g_timer_thread = RT_NULL;

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
        /* Clamp to RT_TICK_MAX/2 - 1 to avoid assertion in rt_timer_start */
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
    /* Periodic soft timer to drive CANopen TimeDispatch */
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

/* Empty mutex implementations (single-threaded) */
void EnterMutex(void) { }
void LeaveMutex(void) { }

static void timer_thread_entry(void *param)
{
    (void)param;
    CO_Data *d = &Master_Data;
    Message msg;

    while (1) {
        /* Process CAN receive frames */
        while (canReceive((CAN_HANDLE)1, &msg)) {
            canDispatch(d, &msg);
        }
        /* Fire event-driven TPDOs (type 0xFE/0xFF) on OD variable change */
        sendPDOevent(d);
        rt_thread_mdelay(5);
    }
}

void StartTimerLoop(void (*callback)(CO_Data *, UNS32))
{
    (void)callback;     /* no longer used — we use TimeDispatch directly */
    TimerInit();
    g_timer_thread = rt_thread_create("cantloop", timer_thread_entry,
                                       RT_NULL, 1024, 8, 10);
    if (g_timer_thread) {
        rt_thread_startup(g_timer_thread);
    }
}

void StopTimerLoop(void (*callback)(CO_Data *, UNS32))
{
    (void)callback;
    if (g_timer_thread) {
        rt_thread_delete(g_timer_thread);
        g_timer_thread = RT_NULL;
    }
    TimerCleanup();
}
