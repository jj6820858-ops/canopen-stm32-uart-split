#ifndef OBJDICT_H
#define OBJDICT_H

#include "data.h"

/* Prototypes of functions provided by object dictionary */
UNS32 CanOpenMaster_valueRangeTest(UNS8 typeValue, void *value);
const indextable *CanOpenMaster_scanIndexOD(UNS16 wIndex,
    UNS32 *errorCode, ODCallback_t **callbacks);

/* Master node data struct */
extern CO_Data CanOpenMaster_Data;

#endif
