/*
 * CANopen Master Protocol Gateway
 * RT-Thread + STM32F103RCT6 + CANfestival
 *
 * Architecture:
 *   Host PC --serial(AA55 frame)-->> STM32(CANopen Master) --CANopen-->> Slave MCU
 */

#include <rtthread.h>
#include "canopen_master.h"
#include "serial_protocol.h"
#include "reg_router.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    LOG_I("====================================");
    LOG_I(" CANopen Master Protocol Gateway");
    LOG_I(" STM32F103RCT6, RT-Thread v4.0.3");
    LOG_I("====================================");

    /* 1. Init routing table (no dependencies) */
    reg_router_init();

    /* 2. Init CANopen master (CAN hardware + CANfestival + timer loop) */
    if (canopen_master_init() != 0) {
        LOG_E("CANopen master init failed! Halting.");
        while (1) {
            rt_thread_mdelay(1000);
        }
    }

    /* 3. Init serial protocol (UART2 AA55 frame handler) */
    serial_protocol_init();

    LOG_I("====================================");
    LOG_I(" System ready. Waiting for commands.");
    LOG_I(" UART1: PA9  PA10   @ 115200 (console)");
    LOG_I(" UART2: PA2  PA3    @ 115200 (AA55 prot)");
    LOG_I(" CAN1:  PA12 PA11   @ 50K             ");
    LOG_I("====================================");

    while (1) {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
