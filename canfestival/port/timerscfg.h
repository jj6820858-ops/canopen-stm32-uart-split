#ifndef __TIMERSCFG_H__
#define __TIMERSCFG_H__

/* Timer value type (RT-Thread tick = 1ms) */
#define TIMEVAL             UNS32
#define TIMEVAL_MAX         0xFFFFFFFFUL
#define MS_TO_TIMEVAL(ms)   ((ms))
#define US_TO_TIMEVAL(us)   (((us) + 999) / 1000)

#endif /* __TIMERSCFG_H__ */
