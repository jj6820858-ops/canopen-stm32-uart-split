#ifndef __CANOPEN_MASTER_H__
#define __CANOPEN_MASTER_H__

#include <rtthread.h>
#include "canfestival/port/canfestival.h"
#include "canfestival/port/can_stm32.h"
#include "canfestival/port/timer_rtthread.h"
#include "canfestival/od_master/ObjDict.h"

/* Slave node IDs are configured in ObjDict.c (objdictgen-generated).
   SDO channels 0x1280~0x1284 each carry their own Node_ID subindex.
   PDO COB-IDs are hardcoded per-slave in the OD table.
   Do NOT override at runtime — OD is the single source of truth. */

int canopen_master_init(void);

/* External: CANopen master data */
extern CO_Data Master_Data;

#endif /* __CANOPEN_MASTER_H__ */
