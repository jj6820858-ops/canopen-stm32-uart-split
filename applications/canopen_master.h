#ifndef __CANOPEN_MASTER_H__
#define __CANOPEN_MASTER_H__

#include <rtthread.h>
#include "canfestival/port/canfestival.h"
#include "canfestival/port/can_stm32.h"
#include "canfestival/port/timer_rtthread.h"
#include "canfestival/od_master/ObjDict.h"

#define SLAVE_NODE_ID  0x02

int canopen_master_init(void);

/* External: CANopen master data (for SDO operations) */
extern CO_Data CanOpenMaster_Data;

#endif /* __CANOPEN_MASTER_H__ */
