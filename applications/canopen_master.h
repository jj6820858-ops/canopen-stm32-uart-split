#ifndef __CANOPEN_MASTER_H__
#define __CANOPEN_MASTER_H__

#include <rtthread.h>
#include "canfestival/port/canfestival.h"
#include "canfestival/port/can_stm32.h"
#include "canfestival/port/timer_rtthread.h"
#include "canfestival/od_master/ObjDict.h"

#ifndef CONFIG_CANOPEN_SLAVE_NODE_ID
#define CONFIG_CANOPEN_SLAVE_NODE_ID  0x01
#endif
#define SLAVE_NODE_ID  CONFIG_CANOPEN_SLAVE_NODE_ID

int canopen_master_init(void);

/* External: CANopen master data (for SDO operations) */
extern CO_Data CanOpenMaster_Data;

#endif /* __CANOPEN_MASTER_H__ */
