/*
 * can_test.c - CAN 总线发送调试命令
 *
 * 测试逻辑放在 tests 目录，线程创建统一委托给 app_thread.c。
 */
#include <rtthread.h>
#include <stdlib.h>

#include "../applications/threads/app_thread.h"
#include "../canfestival/include/can_driver.h"

static volatile int g_test_running = 0;

static void can_test_entry(void *parameter)
{
    rt_uint32_t interval_ms = (rt_uint32_t)(rt_ubase_t)parameter;
    rt_uint32_t count = 0;

    rt_kprintf("[CAN_TEST] start, interval=%dms, id=0x123\n", interval_ms);

    while (g_test_running) {
        Message msg;

        msg.cob_id = 0x123;
        msg.rtr = 0;
        msg.len = 8;
        for (int i = 0; i < 8; i++) {
            msg.data[i] = (UNS8)(count + i);
        }

        if (canSend((CAN_HANDLE)1, &msg) == 0) {
            if ((count % 100) == 0) {
                rt_kprintf("[CAN_TEST] 已发送 %lu 帧\n",
                           (unsigned long)(count + 1));
            }
        } else {
            rt_kprintf("[CAN_TEST] 发送失败, count=%lu\n",
                       (unsigned long)count);
        }

        count++;
        rt_thread_mdelay(interval_ms);
    }

    rt_kprintf("[CAN_TEST] stop, count=%lu\n", (unsigned long)count);
}

static int can_test_start(int argc, char **argv)
{
    rt_uint32_t interval = 200;
    int ret;

    if (g_test_running) {
        rt_kprintf("[CAN_TEST] 已在运行\n");
        return 0;
    }

    if (argc >= 2) {
        interval = (rt_uint32_t)atoi(argv[1]);
        if (interval < 5) interval = 5;
        if (interval > 10000) interval = 10000;
    }

    g_test_running = 1;
    ret = app_thread_can_test_start(can_test_entry,
                                    (void *)(rt_ubase_t)interval);
    if (ret != RT_EOK) {
        g_test_running = 0;
        rt_kprintf("[CAN_TEST] 线程启动失败: %d\n", ret);
    }

    return 0;
}
MSH_CMD_EXPORT(can_test_start, start CAN test);

static int can_test_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!g_test_running) {
        rt_kprintf("[CAN_TEST] 未运行\n");
        return 0;
    }

    g_test_running = 0;
    app_thread_can_test_stop();
    return 0;
}
MSH_CMD_EXPORT(can_test_stop, stop CAN test);
