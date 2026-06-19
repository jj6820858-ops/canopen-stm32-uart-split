/*
 * Simplified CAN test (like 1211 project)
 * RT-Thread + STM32F103RCT6
 */

#include <rtthread.h>

/* CAN init (defined in canfestival/port/can_stm32.c) */
extern void can_hardware_init(void);

int main(void)
{
    rt_kprintf("CAN1 20KHz Normal (PA15=0)\n");

    /* Init CAN hardware + auto-start test thread */
    can_hardware_init();

    return 0;
}
