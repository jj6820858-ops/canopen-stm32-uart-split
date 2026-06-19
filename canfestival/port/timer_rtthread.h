#ifndef __TIMER_RTTHREAD_H__
#define __TIMER_RTTHREAD_H__

#include "applicfg.h"
#include "data.h"

void TimerInit(void);
void TimerCleanup(void);
void StartTimerLoop(void (*callback)(CO_Data *, UNS32));
void StopTimerLoop(void (*callback)(CO_Data *, UNS32));

#endif /* __TIMER_RTTHREAD_H__ */
