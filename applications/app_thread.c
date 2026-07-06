/*
 * app_thread.c - 应用线程和任务统一管理
 *
 * 本文件集中负责:
 *   1. 板级应用电源使能;
 *   2. CANopen、Modbus、采样转发、上电校验等模块初始化;
 *   3. 创建需要长期运行的应用线程。
 */
#include "app_thread.h"

#include <board.h>
#include <rtthread.h>

#include "canopen_master.h"
#include "mb.h"
#include "power_on_check.h"
#include "reg_router.h"
#include "sampling.h"
#include "user_mb_app.h"

#define MODBUS_SLAVE_ADDR       CONFIG_MODBUS_SLAVE_ADDR
#define MODBUS_PORT             2
#define MODBUS_BAUDRATE         115200
#define MODBUS_POLL_MS          5

#define MODBUS_THREAD_STACK     2048
#define MODBUS_THREAD_PRIORITY  18
#define MODBUS_THREAD_TICK      10

/* PC0: 电源控制输出，高电平使能 */
#define PWR_CTRL_PORT           GPIOC
#define PWR_CTRL_PIN            GPIO_PIN_0

static rt_thread_t g_modbus_thread = RT_NULL;

static void pwr_ctrl_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin   = PWR_CTRL_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(PWR_CTRL_PORT, &gpio);
    HAL_GPIO_WritePin(PWR_CTRL_PORT, PWR_CTRL_PIN, GPIO_PIN_SET);
}

static void modbus_thread_entry(void *parameter)
{
    (void)parameter;

    while (1) {
        eMBPoll();
        rt_thread_mdelay(MODBUS_POLL_MS);
    }
}

static int modbus_thread_start(void)
{
    eMBErrorCode err;

    err = eMBInit(MB_RTU, MODBUS_SLAVE_ADDR, MODBUS_PORT,
                  MODBUS_BAUDRATE, MB_PAR_NONE);
    if (err != MB_ENOERR) {
        rt_kprintf("FreeModbus 初始化失败: %d\n", err);
        return -RT_ERROR;
    }

    err = eMBEnable();
    if (err != MB_ENOERR) {
        rt_kprintf("FreeModbus 使能失败: %d\n", err);
        return -RT_ERROR;
    }

    g_modbus_thread = rt_thread_create("mbpoll", modbus_thread_entry, RT_NULL,
                                       MODBUS_THREAD_STACK,
                                       MODBUS_THREAD_PRIORITY,
                                       MODBUS_THREAD_TICK);
    if (g_modbus_thread == RT_NULL) {
        rt_kprintf("Modbus 轮询线程创建失败\n");
        return -RT_ENOMEM;
    }

    return rt_thread_startup(g_modbus_thread);
}

int app_thread_init(void)
{
    int ret;

    pwr_ctrl_init();
    rt_kprintf("PC0: 推挽输出高电平，电源已使能\n");

    reg_router_init();
    canopen_master_init();
    power_on_check_init();
    sampling_init();

    ret = modbus_thread_start();
    if (ret != RT_EOK) {
        return ret;
    }

    rt_kprintf("\n系统就绪: FreeModbus RTU 地址=%d, UART2\n", MODBUS_SLAVE_ADDR);
    rt_kprintf("链路: Modbus -> reg_router -> OD -> PDO -> CAN 从站\n");
    rt_kprintf("上电校验命令: pwrchk start\n\n");

    return RT_EOK;
}
