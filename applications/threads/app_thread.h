/*
 * app_thread.h - 应用线程统一管理接口
 */
#ifndef __APP_THREAD_H__
#define __APP_THREAD_H__

#include <rtthread.h>

int app_thread_init(void);
int app_thread_can_test_start(void (*entry)(void *parameter), void *parameter);
void app_thread_can_test_stop(void);

#endif /* __APP_THREAD_H__ */
