#ifndef __CAN_STM32_H__
#define __CAN_STM32_H__

#include "applicfg.h"
#include "can_driver.h"

/* CAN hardware initialization (call once before CANopen start) */
void can_hardware_init(void);

#endif /* __CAN_STM32_H__ */
