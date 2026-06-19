/*
 * CANopen Master Object Dictionary — Minimal DS-301 Master
 * Auto-generated pattern, written manually.
 * Node ID = 0x01, DS-301
 */

#include "ObjDict.h"

/**************************************************************************/
/* Data type constants (from can.h / def.h)                              */
/**************************************************************************/
#ifdef uint8
#undef uint8
#endif
#ifdef uint16
#undef uint16
#endif
#ifdef uint32
#undef uint32
#endif
#define uint8  0x05
#define uint16 0x06
#define uint32 0x07

/**************************************************************************/
/* Value range test                                                       */
/**************************************************************************/
#define valueRange_EMC 0x9F
UNS32 CanOpenMaster_valueRangeTest(UNS8 typeValue, void *value)
{
    (void)value;
    switch (typeValue) {
        case valueRange_EMC:
            if (*(UNS8*)value != (UNS8)0) return OD_VALUE_RANGE_EXCEEDED;
            break;
    }
    return 0;
}

/**************************************************************************/
/* Node ID                                                                */
/**************************************************************************/
UNS8 CanOpenMaster_bDeviceNodeId = 0x01;

/**************************************************************************/
/* Master flag                                                            */
/**************************************************************************/
const UNS8 CanOpenMaster_iam_a_slave = 0;

/**************************************************************************/
/* Heartbeat timers                                                       */
/**************************************************************************/
TIMER_HANDLE CanOpenMaster_heartBeatTimers[1] = { TIMER_NONE };

/**************************************************************************/
/*                        OBJECT DICTIONARY                               */
/**************************************************************************/

/* index 0x1000 : Device Type */
    UNS32 CanOpenMaster_obj1000 = 0x00000000;   /* 0 = no profile */
    subindex CanOpenMaster_Index1000[] = {
        { RO, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1000 }
    };

/* index 0x1001 : Error Register */
    UNS8 CanOpenMaster_obj1001 = 0x00;
    subindex CanOpenMaster_Index1001[] = {
        { RO, uint8, sizeof(UNS8), (void*)&CanOpenMaster_obj1001 }
    };

/* index 0x1003 : Pre-defined Error Field */
    UNS8 CanOpenMaster_highestSubIndex_obj1003 = 1;
    UNS32 CanOpenMaster_obj1003[] = { 0x00000000 };
    ODCallback_t CanOpenMaster_Index1003_callbacks[] = { NULL, NULL };
    subindex CanOpenMaster_Index1003[] = {
        { RW, valueRange_EMC, sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1003 },
        { RO, uint32,         sizeof(UNS32), (void*)&CanOpenMaster_obj1003[0] }
    };

/* index 0x1005 : SYNC COB-ID */
    UNS32 CanOpenMaster_obj1005 = 0x00000080;   /* COB-ID 0x80 */
    ODCallback_t CanOpenMaster_Index1005_callbacks[] = { NULL };
    subindex CanOpenMaster_Index1005[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1005 }
    };

/* index 0x1006 : Communication Cycle Period */
    UNS32 CanOpenMaster_obj1006 = 50000;        /* 50ms */
    ODCallback_t CanOpenMaster_Index1006_callbacks[] = { NULL };
    subindex CanOpenMaster_Index1006[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1006 }
    };

/* index 0x100C : Guard Time */
    UNS16 CanOpenMaster_obj100C = 0x0000;
    ODCallback_t CanOpenMaster_Index100C_callbacks[] = { NULL };
    subindex CanOpenMaster_Index100C[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj100C }
    };

/* index 0x100D : Life Time Factor */
    UNS8 CanOpenMaster_obj100D = 0x00;
    ODCallback_t CanOpenMaster_Index100D_callbacks[] = { NULL };
    subindex CanOpenMaster_Index100D[] = {
        { RW, uint8, sizeof(UNS8), (void*)&CanOpenMaster_obj100D }
    };

/* index 0x1014 : Emergency COB-ID */
    UNS32 CanOpenMaster_obj1014 = 0x00000080;   /* COB-ID 0x80 + node 0x01 */
    ODCallback_t CanOpenMaster_Index1014_callbacks[] = { NULL };
    subindex CanOpenMaster_Index1014[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1014 }
    };

/* index 0x1016 : Consumer Heartbeat Time */
    UNS8 CanOpenMaster_highestSubIndex_obj1016 = 1;
    UNS32 CanOpenMaster_obj1016[] = { 0x000007D0 };  /* node 0x00, 2000ms — unused */
    subindex CanOpenMaster_Index1016[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1016 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1016[0] }
    };

/* index 0x1017 : Producer Heartbeat Time */
    UNS16 CanOpenMaster_obj1017 = 0x0000;       /* 0 = disabled */
    ODCallback_t CanOpenMaster_Index1017_callbacks[] = { NULL };
    subindex CanOpenMaster_Index1017[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1017 }
    };

/* index 0x1018 : Identity */
    UNS8 CanOpenMaster_highestSubIndex_obj1018 = 4;
    UNS32 CanOpenMaster_obj1018_Vendor_ID     = 0x00000000;
    UNS32 CanOpenMaster_obj1018_Product_Code  = 0x00000000;
    UNS32 CanOpenMaster_obj1018_Revision_Number = 0x00000000;
    UNS32 CanOpenMaster_obj1018_Serial_Number   = 0x00000000;
    subindex CanOpenMaster_Index1018[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1018 },
        { RO, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1018_Vendor_ID },
        { RO, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1018_Product_Code },
        { RO, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1018_Revision_Number },
        { RO, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1018_Serial_Number }
    };

/* index 0x1280 : Client SDO 1 Parameter (for SDO to slaves) */
    UNS8 CanOpenMaster_highestSubIndex_obj1280 = 3;
    UNS32 CanOpenMaster_obj1280_COB_ID_Client_to_Server_Transmit_SDO = 0x00000640; /* 0x640 + node */
    UNS32 CanOpenMaster_obj1280_COB_ID_Server_to_Client_Receive_SDO  = 0x000005C0; /* 0x5C0 + node */
    UNS8  CanOpenMaster_obj1280_Node_ID_of_the_SDO_Server = 0x40;  /* default: node 0x40 */
    subindex CanOpenMaster_Index1280[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1280 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1280_COB_ID_Client_to_Server_Transmit_SDO },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1280_COB_ID_Server_to_Client_Receive_SDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1280_Node_ID_of_the_SDO_Server }
    };

/* index 0x1400 : Receive PDO 1 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1400 = 5;
    UNS32 CanOpenMaster_obj1400_COB_ID_used_by_PDO = 0x80000182; /* 0x182, disabled bit 31 */
    UNS8  CanOpenMaster_obj1400_Transmission_Type   = 0xFF;      /* async */
    UNS16 CanOpenMaster_obj1400_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1400_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1400_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1400[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1400 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1400_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1400_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1400_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1400_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1400_Event_Timer }
    };

/* index 0x1600 : Receive PDO 1 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1600 = 1;
    UNS32 CanOpenMaster_obj1600[] = { 0x00000000 }; /* empty mapping */
    subindex CanOpenMaster_Index1600[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1600 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1600[0] }
    };

/* index 0x1800 : Transmit PDO 1 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1800 = 5;
    UNS32 CanOpenMaster_obj1800_COB_ID_used_by_PDO = 0x40000202; /* 0x202, disabled bit 30 */
    UNS8  CanOpenMaster_obj1800_Transmission_Type   = 0xFF;      /* async */
    UNS16 CanOpenMaster_obj1800_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1800_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1800_Event_Timer          = 0x0000;
    ODCallback_t CanOpenMaster_Index1800_callbacks[] = {
        NULL, NULL, NULL, NULL, NULL, NULL,
    };
    subindex CanOpenMaster_Index1800[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1800 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1800_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1800_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1800_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1800_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1800_Event_Timer }
    };

/* index 0x1A00 : Transmit PDO 1 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A00 = 1;
    UNS32 CanOpenMaster_obj1A00[] = { 0x00000000 };  /* empty mapping */
    subindex CanOpenMaster_Index1A00[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A00 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A00[0] }
    };

/**************************************************************************/
/*                          INDEX TABLE                                   */
/**************************************************************************/

const indextable CanOpenMaster_objdict[] = {
    { (subindex*)CanOpenMaster_Index1000, sizeof(CanOpenMaster_Index1000)/sizeof(CanOpenMaster_Index1000[0]), 0x1000 },
    { (subindex*)CanOpenMaster_Index1001, sizeof(CanOpenMaster_Index1001)/sizeof(CanOpenMaster_Index1001[0]), 0x1001 },
    { (subindex*)CanOpenMaster_Index1003, sizeof(CanOpenMaster_Index1003)/sizeof(CanOpenMaster_Index1003[0]), 0x1003 },
    { (subindex*)CanOpenMaster_Index1005, sizeof(CanOpenMaster_Index1005)/sizeof(CanOpenMaster_Index1005[0]), 0x1005 },
    { (subindex*)CanOpenMaster_Index1006, sizeof(CanOpenMaster_Index1006)/sizeof(CanOpenMaster_Index1006[0]), 0x1006 },
    { (subindex*)CanOpenMaster_Index100C, sizeof(CanOpenMaster_Index100C)/sizeof(CanOpenMaster_Index100C[0]), 0x100C },
    { (subindex*)CanOpenMaster_Index100D, sizeof(CanOpenMaster_Index100D)/sizeof(CanOpenMaster_Index100D[0]), 0x100D },
    { (subindex*)CanOpenMaster_Index1014, sizeof(CanOpenMaster_Index1014)/sizeof(CanOpenMaster_Index1014[0]), 0x1014 },
    { (subindex*)CanOpenMaster_Index1016, sizeof(CanOpenMaster_Index1016)/sizeof(CanOpenMaster_Index1016[0]), 0x1016 },
    { (subindex*)CanOpenMaster_Index1017, sizeof(CanOpenMaster_Index1017)/sizeof(CanOpenMaster_Index1017[0]), 0x1017 },
    { (subindex*)CanOpenMaster_Index1018, sizeof(CanOpenMaster_Index1018)/sizeof(CanOpenMaster_Index1018[0]), 0x1018 },
    { (subindex*)CanOpenMaster_Index1280, sizeof(CanOpenMaster_Index1280)/sizeof(CanOpenMaster_Index1280[0]), 0x1280 },
    { (subindex*)CanOpenMaster_Index1400, sizeof(CanOpenMaster_Index1400)/sizeof(CanOpenMaster_Index1400[0]), 0x1400 },
    { (subindex*)CanOpenMaster_Index1600, sizeof(CanOpenMaster_Index1600)/sizeof(CanOpenMaster_Index1600[0]), 0x1600 },
    { (subindex*)CanOpenMaster_Index1800, sizeof(CanOpenMaster_Index1800)/sizeof(CanOpenMaster_Index1800[0]), 0x1800 },
    { (subindex*)CanOpenMaster_Index1A00, sizeof(CanOpenMaster_Index1A00)/sizeof(CanOpenMaster_Index1A00[0]), 0x1A00 },
};

/**************************************************************************/
/*                         SCAN INDEX                                     */
/**************************************************************************/

const indextable *CanOpenMaster_scanIndexOD(UNS16 wIndex,
    UNS32 *errorCode, ODCallback_t **callbacks)
{
    *callbacks = NULL;
    switch (wIndex) {
        case 0x1000: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[0];
        case 0x1001: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[1];
        case 0x1003: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1003_callbacks; return &CanOpenMaster_objdict[2];
        case 0x1005: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1005_callbacks; return &CanOpenMaster_objdict[3];
        case 0x1006: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1006_callbacks; return &CanOpenMaster_objdict[4];
        case 0x100C: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index100C_callbacks; return &CanOpenMaster_objdict[5];
        case 0x100D: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index100D_callbacks; return &CanOpenMaster_objdict[6];
        case 0x1014: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1014_callbacks; return &CanOpenMaster_objdict[7];
        case 0x1016: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[8];
        case 0x1017: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1017_callbacks; return &CanOpenMaster_objdict[9];
        case 0x1018: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[10];
        case 0x1280: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[11];
        case 0x1400: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[12];
        case 0x1600: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[13];
        case 0x1800: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1800_callbacks; return &CanOpenMaster_objdict[14];
        case 0x1A00: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[15];
        default:
            *errorCode = OD_NO_SUCH_OBJECT;
            return NULL;
    }
}

/**************************************************************************/
/*                       PDO STATUS                                       */
/**************************************************************************/

s_PDO_status CanOpenMaster_PDO_status[1] = { s_PDO_status_Initializer };

/**************************************************************************/
/*                       QUICK INDEX                                      */
/**************************************************************************/

quick_index CanOpenMaster_firstIndex = {
    2,  /* SDO_SVR: 0x1003 */
    11, /* SDO_CLT: 0x1280 */
    12, /* PDO_RCV: 0x1400 */
    13, /* PDO_RCV_MAP: 0x1600 */
    14, /* PDO_TRS: 0x1800 */
    15  /* PDO_TRS_MAP: 0x1A00 */
};

quick_index CanOpenMaster_lastIndex = {
    2,  /* SDO_SVR */
    11, /* SDO_CLT */
    12, /* PDO_RCV */
    13, /* PDO_RCV_MAP */
    14, /* PDO_TRS */
    15  /* PDO_TRS_MAP */
};

UNS16 CanOpenMaster_ObjdictSize = sizeof(CanOpenMaster_objdict) /
                                  sizeof(CanOpenMaster_objdict[0]);

/**************************************************************************/
/*                      CO_Data INITIALIZATION                            */
/**************************************************************************/

CO_Data CanOpenMaster_Data = CANOPEN_NODE_DATA_INITIALIZER(CanOpenMaster);
