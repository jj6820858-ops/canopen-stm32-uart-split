#ifndef __CANOPEN_MASTER_H__
#define __CANOPEN_MASTER_H__

#include <rtthread.h>
#include "canfestival/port/canfestival.h"
#include "canfestival/port/can_stm32.h"
#include "canfestival/port/timer_rtthread.h"
#include "canfestival/od_master/ObjDict.h"

/*
 * 从站节点 ID 在 ObjDict.c 中配置，由 objdictgen 生成。
 * SDO 通道 0x1280~0x1284 各自带 Node_ID 子索引。
 * PDO COB-ID 已在对象字典表中按从站写死，运行时不要覆盖。
 */

int canopen_master_init(void);

/* CANopen 主站数据 */
extern CO_Data Master_Data;

#endif /* __CANOPEN_MASTER_H__ */
