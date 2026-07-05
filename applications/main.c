/*
 * CANopen Master + FreeModbus RTU Slave Gateway
 * FreeModbus UART2: eMBPoll() drives Modbus → reg_router → OD → PDO → CAN
 *
 * CANopen config: ObjDict.c / ObjDict.h (objdictgen-generated).
 * Modbus → OD mapping: reg_router.c.
 * Power-on check sequence: power_on_check.c.
 * No runtime OD patching — OD is the single source of truth.
 */
#include <rtthread.h>
#include <board.h>
#include "canopen_master.h"
#include "reg_router.h"
#include "power_on_check.h"
#include "sampling.h"
#include "mb.h"              /* FreeModbus: eMBInit / eMBEnable / eMBPoll */
#include "user_mb_app.h"     /* FreeModbus: register callbacks */

#define MODBUS_SLAVE_ADDR   CONFIG_MODBUS_SLAVE_ADDR
#define MODBUS_PORT         2           /* UART2 */
#define MODBUS_BAUDRATE     115200
#define MB_POLL_MS          5

/* PC0 — 触发电源控制 (推挽输出, 高电平使能) */
#define PWR_CTRL_PORT       GPIOC
#define PWR_CTRL_PIN        GPIO_PIN_0

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

int main(void)
{
    pwr_ctrl_init();
    rt_kprintf("PC0 → push-pull HIGH (power supply ON)\n");

    reg_router_init();
    canopen_master_init();
    power_on_check_init();
    sampling_init();

    /* Start FreeModbus RTU Slave */
    eMBInit(MB_RTU, MODBUS_SLAVE_ADDR, MODBUS_PORT, MODBUS_BAUDRATE, MB_PAR_NONE);
    eMBEnable();

    rt_kprintf("\nSystem ready. FreeModbus RTU addr=%d on UART2\n", MODBUS_SLAVE_ADDR);
    rt_kprintf("Modbus → reg_router → OD → PDO → CAN slave\n");
    rt_kprintf("Type 'pwrchk start' for power-on check sequence\n\n");

    while (1) {
        eMBPoll();
        rt_thread_mdelay(MB_POLL_MS);
    }
}
