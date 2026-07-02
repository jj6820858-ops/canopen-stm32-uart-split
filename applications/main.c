/*
 * CANopen Master + FreeModbus RTU Slave Gateway
 * FreeModbus UART2: eMBPoll() drives Modbus → reg_router → OD → PDO → CAN
 *
 * CANopen config: ObjDict.c / ObjDict.h (objdictgen-generated).
 * Modbus → OD mapping: reg_router.c.
 * No runtime OD patching — OD is the single source of truth.
 */
#include <rtthread.h>
#include "canopen_master.h"
#include "reg_router.h"
#include "mb.h"              /* FreeModbus: eMBInit / eMBEnable / eMBPoll */
#include "user_mb_app.h"     /* FreeModbus: register callbacks */

#define MODBUS_SLAVE_ADDR   CONFIG_MODBUS_SLAVE_ADDR
#define MODBUS_PORT         2           /* UART2 */
#define MODBUS_BAUDRATE     115200
#define MB_POLL_MS          20

int main(void)
{
    reg_router_init();
    canopen_master_init();

    /* Start FreeModbus RTU Slave */
    eMBInit(MB_RTU, MODBUS_SLAVE_ADDR, MODBUS_PORT, MODBUS_BAUDRATE, MB_PAR_NONE);
    eMBEnable();

    rt_kprintf("\nSystem ready. FreeModbus RTU addr=%d on UART2\n", MODBUS_SLAVE_ADDR);
    rt_kprintf("Modbus → reg_router → OD → PDO → CAN slave\n\n");

    while (1) {
        eMBPoll();
        rt_thread_mdelay(MB_POLL_MS);
    }
}
