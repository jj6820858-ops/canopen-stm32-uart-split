/*
 * CANopen Master Object Dictionary — Multi-axis Motion + Instrument Control
 * Generated from Master.od (DS-402 + custom manufacturer objects).
 * Node ID = 0x01, DS-301 Master
 * 5 axes (X/Y/Z/E/T) + photometer + temperature + weight sensors
 */

#include "ObjDict.h"

/**************************************************************************/
/* Data type constants (re-def after undef to ensure consistency)         */
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
/* int8/int16/int32 are defined in objdictdef.h:
   int8=0x02, int16=0x03, int32=0x04 */

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
UNS8 CanOpenMaster_bDeviceNodeId = 0x00;

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

/* ===== Standard DS-301 Communication Objects (0x1000-0x1018) ===== */

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

/* ===== 0x00A0 : INTEGER8[0-10] Type Definition ===== */

/* index 0x00A0 : INTEGER8[0-10] */
    UNS8 CanOpenMaster_highestSubIndex_obj00A0 = 4;
    UNS8 CanOpenMaster_obj00A0_Number_of_Entries = 3;
    INTEGER8 CanOpenMaster_obj00A0_Type            = 2;   /* int8 type code */
    INTEGER8 CanOpenMaster_obj00A0_Minimum_Value   = 0;
    INTEGER8 CanOpenMaster_obj00A0_Maximum_Value   = 10;
    subindex CanOpenMaster_Index00A0[] = {
        { RO, uint8, sizeof(UNS8),      (void*)&CanOpenMaster_highestSubIndex_obj00A0 },
        { RO, uint8, sizeof(UNS8),      (void*)&CanOpenMaster_obj00A0_Number_of_Entries },
        { RO, int8,  sizeof(INTEGER8),  (void*)&CanOpenMaster_obj00A0_Type },
        { RO, int8,  sizeof(INTEGER8),  (void*)&CanOpenMaster_obj00A0_Minimum_Value },
        { RO, int8,  sizeof(INTEGER8),  (void*)&CanOpenMaster_obj00A0_Maximum_Value }
    };

/* ===== SDO Server Parameter (0x1200) ===== */

/* index 0x1200 : SDO Server Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1200 = 2;
    UNS32 CanOpenMaster_obj1200_COB_ID_Client_to_Server = 0x00000601; /* 0x600 + node */
    UNS32 CanOpenMaster_obj1200_COB_ID_Server_to_Client = 0x00000581; /* 0x580 + node */
    subindex CanOpenMaster_Index1200[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1200 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1200_COB_ID_Client_to_Server },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1200_COB_ID_Server_to_Client }
    };

/* ===== SDO Client Parameters (0x1280-0x1284) ===== */

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

/* index 0x1281 : Client SDO 2 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1281 = 3;
    UNS32 CanOpenMaster_obj1281_COB_ID_Client_to_Server_Transmit_SDO = 0x00000641; /* 0x641 + node */
    UNS32 CanOpenMaster_obj1281_COB_ID_Server_to_Client_Receive_SDO  = 0x000005C1; /* 0x5C1 + node */
    UNS8  CanOpenMaster_obj1281_Node_ID_of_the_SDO_Server = 0x02;
    subindex CanOpenMaster_Index1281[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1281 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1281_COB_ID_Client_to_Server_Transmit_SDO },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1281_COB_ID_Server_to_Client_Receive_SDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1281_Node_ID_of_the_SDO_Server }
    };

/* index 0x1282 : Client SDO 3 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1282 = 3;
    UNS32 CanOpenMaster_obj1282_COB_ID_Client_to_Server_Transmit_SDO = 0x00000642; /* 0x642 + node */
    UNS32 CanOpenMaster_obj1282_COB_ID_Server_to_Client_Receive_SDO  = 0x000005C2; /* 0x5C2 + node */
    UNS8  CanOpenMaster_obj1282_Node_ID_of_the_SDO_Server = 0x03;
    subindex CanOpenMaster_Index1282[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1282 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1282_COB_ID_Client_to_Server_Transmit_SDO },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1282_COB_ID_Server_to_Client_Receive_SDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1282_Node_ID_of_the_SDO_Server }
    };

/* index 0x1283 : Client SDO 4 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1283 = 3;
    UNS32 CanOpenMaster_obj1283_COB_ID_Client_to_Server_Transmit_SDO = 0x00000643; /* 0x643 + node */
    UNS32 CanOpenMaster_obj1283_COB_ID_Server_to_Client_Receive_SDO  = 0x000005C3; /* 0x5C3 + node */
    UNS8  CanOpenMaster_obj1283_Node_ID_of_the_SDO_Server = 0x04;
    subindex CanOpenMaster_Index1283[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1283 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1283_COB_ID_Client_to_Server_Transmit_SDO },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1283_COB_ID_Server_to_Client_Receive_SDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1283_Node_ID_of_the_SDO_Server }
    };

/* index 0x1284 : Client SDO 5 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1284 = 3;
    UNS32 CanOpenMaster_obj1284_COB_ID_Client_to_Server_Transmit_SDO = 0x00000644; /* 0x644 + node */
    UNS32 CanOpenMaster_obj1284_COB_ID_Server_to_Client_Receive_SDO  = 0x000005C4; /* 0x5C4 + node */
    UNS8  CanOpenMaster_obj1284_Node_ID_of_the_SDO_Server = 0x05;
    subindex CanOpenMaster_Index1284[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1284 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1284_COB_ID_Client_to_Server_Transmit_SDO },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1284_COB_ID_Server_to_Client_Receive_SDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1284_Node_ID_of_the_SDO_Server }
    };

/* ===== RPDO Communication Parameters (0x1400-0x1409) — 10 RPDOs ===== */

/* index 0x1400 : Receive PDO 1 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1400 = 5;
    UNS32 CanOpenMaster_obj1400_COB_ID_used_by_PDO = 0x00000181; /* 0x181 */
    UNS8  CanOpenMaster_obj1400_Transmission_Type   = 0x00;
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

/* index 0x1401 : Receive PDO 2 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1401 = 5;
    UNS32 CanOpenMaster_obj1401_COB_ID_used_by_PDO = 0x00000182; /* 0x182 */
    UNS8  CanOpenMaster_obj1401_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1401_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1401_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1401_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1401[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1401 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1401_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1401_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1401_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1401_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1401_Event_Timer }
    };

/* index 0x1402 : Receive PDO 3 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1402 = 5;
    UNS32 CanOpenMaster_obj1402_COB_ID_used_by_PDO = 0x00000183; /* 0x183 */
    UNS8  CanOpenMaster_obj1402_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1402_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1402_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1402_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1402[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1402 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1402_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1402_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1402_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1402_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1402_Event_Timer }
    };

/* index 0x1403 : Receive PDO 4 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1403 = 5;
    UNS32 CanOpenMaster_obj1403_COB_ID_used_by_PDO = 0x00000184; /* 0x184 */
    UNS8  CanOpenMaster_obj1403_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1403_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1403_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1403_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1403[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1403 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1403_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1403_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1403_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1403_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1403_Event_Timer }
    };

/* index 0x1404 : Receive PDO 5 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1404 = 5;
    UNS32 CanOpenMaster_obj1404_COB_ID_used_by_PDO = 0x00000185; /* 0x185 */
    UNS8  CanOpenMaster_obj1404_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1404_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1404_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1404_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1404[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1404 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1404_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1404_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1404_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1404_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1404_Event_Timer }
    };

/* index 0x1405 : Receive PDO 6 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1405 = 5;
    UNS32 CanOpenMaster_obj1405_COB_ID_used_by_PDO = 0x00000186; /* 0x186 */
    UNS8  CanOpenMaster_obj1405_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1405_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1405_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1405_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1405[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1405 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1405_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1405_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1405_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1405_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1405_Event_Timer }
    };

/* index 0x1406 : Receive PDO 7 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1406 = 5;
    UNS32 CanOpenMaster_obj1406_COB_ID_used_by_PDO = 0x00000187; /* 0x187 */
    UNS8  CanOpenMaster_obj1406_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1406_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1406_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1406_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1406[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1406 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1406_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1406_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1406_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1406_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1406_Event_Timer }
    };

/* index 0x1407 : Receive PDO 8 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1407 = 5;
    UNS32 CanOpenMaster_obj1407_COB_ID_used_by_PDO = 0x00000287; /* 0x287 */
    UNS8  CanOpenMaster_obj1407_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1407_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1407_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1407_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1407[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1407 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1407_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1407_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1407_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1407_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1407_Event_Timer }
    };

/* index 0x1408 : Receive PDO 9 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1408 = 5;
    UNS32 CanOpenMaster_obj1408_COB_ID_used_by_PDO = 0x00000188; /* 0x188 */
    UNS8  CanOpenMaster_obj1408_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1408_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1408_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1408_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1408[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1408 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1408_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1408_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1408_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1408_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1408_Event_Timer }
    };

/* index 0x1409 : Receive PDO 10 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1409 = 5;
    UNS32 CanOpenMaster_obj1409_COB_ID_used_by_PDO = 0x00000288; /* 0x288 */
    UNS8  CanOpenMaster_obj1409_Transmission_Type   = 0x00;
    UNS16 CanOpenMaster_obj1409_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1409_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1409_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1409[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1409 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1409_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1409_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1409_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1409_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1409_Event_Timer }
    };

/* ===== RPDO Mapping Parameters (0x1600-0x1609) — 10 mappings ===== */

/* index 0x1600 : Receive PDO 1 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1600 = 2;
    UNS32 CanOpenMaster_obj1600[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1600[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1600 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1600[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1600[1] }
    };

/* index 0x1601 : Receive PDO 2 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1601 = 2;
    UNS32 CanOpenMaster_obj1601[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1601[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1601 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1601[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1601[1] }
    };

/* index 0x1602 : Receive PDO 3 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1602 = 2;
    UNS32 CanOpenMaster_obj1602[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1602[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1602 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1602[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1602[1] }
    };

/* index 0x1603 : Receive PDO 4 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1603 = 2;
    UNS32 CanOpenMaster_obj1603[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1603[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1603 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1603[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1603[1] }
    };

/* index 0x1604 : Receive PDO 5 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1604 = 2;
    UNS32 CanOpenMaster_obj1604[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1604[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1604 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1604[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1604[1] }
    };

/* index 0x1605 : Receive PDO 6 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1605 = 2;
    UNS32 CanOpenMaster_obj1605[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1605[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1605 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1605[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1605[1] }
    };

/* index 0x1606 : Receive PDO 7 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1606 = 2;
    UNS32 CanOpenMaster_obj1606[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1606[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1606 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1606[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1606[1] }
    };

/* index 0x1607 : Receive PDO 8 Mapping (1 mapping entry) */
    UNS8 CanOpenMaster_highestSubIndex_obj1607 = 1;
    UNS32 CanOpenMaster_obj1607[] = { 0x00000000 };
    subindex CanOpenMaster_Index1607[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1607 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1607[0] }
    };

/* index 0x1608 : Receive PDO 9 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1608 = 2;
    UNS32 CanOpenMaster_obj1608[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1608[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1608 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1608[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1608[1] }
    };

/* index 0x1609 : Receive PDO 10 Mapping (1 mapping entry) */
    UNS8 CanOpenMaster_highestSubIndex_obj1609 = 1;
    UNS32 CanOpenMaster_obj1609[] = { 0x00000000 };
    subindex CanOpenMaster_Index1609[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1609 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1609[0] }
    };

/* ===== TPDO Communication Parameters (0x1800-0x180D) — 14 TPDOs ===== */

/* index 0x1800 : Transmit PDO 1 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1800 = 5;
    UNS32 CanOpenMaster_obj1800_COB_ID_used_by_PDO = 0x00000201; /* 0x201 (0x201 + node 0x00) */
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

/* index 0x1801 : Transmit PDO 2 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1801 = 5;
    UNS32 CanOpenMaster_obj1801_COB_ID_used_by_PDO = 0x00000301; /* 0x301 (0x301 + node 0x00) */
    UNS8  CanOpenMaster_obj1801_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1801_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1801_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1801_Event_Timer          = 21;       /* 21ms per Master.od */
    subindex CanOpenMaster_Index1801[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1801 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1801_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1801_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1801_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1801_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1801_Event_Timer }
    };

/* index 0x1802 : Transmit PDO 3 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1802 = 5;
    UNS32 CanOpenMaster_obj1802_COB_ID_used_by_PDO = 0x00000202; /* 0x202 (0x202 + node 0x00) */
    UNS8  CanOpenMaster_obj1802_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1802_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1802_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1802_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1802[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1802 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1802_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1802_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1802_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1802_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1802_Event_Timer }
    };

/* index 0x1803 : Transmit PDO 4 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1803 = 5;
    UNS32 CanOpenMaster_obj1803_COB_ID_used_by_PDO = 0x00000302; /* 0x302 (0x302 + node 0x00) */
    UNS8  CanOpenMaster_obj1803_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1803_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1803_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1803_Event_Timer          = 36;       /* 36ms per Master.od */
    subindex CanOpenMaster_Index1803[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1803 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1803_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1803_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1803_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1803_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1803_Event_Timer }
    };

/* index 0x1804 : Transmit PDO 5 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1804 = 5;
    UNS32 CanOpenMaster_obj1804_COB_ID_used_by_PDO = 0x00000203; /* 0x203 (0x203 + node 0x00) */
    UNS8  CanOpenMaster_obj1804_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1804_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1804_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1804_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1804[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1804 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1804_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1804_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1804_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1804_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1804_Event_Timer }
    };

/* index 0x1805 : Transmit PDO 6 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1805 = 5;
    UNS32 CanOpenMaster_obj1805_COB_ID_used_by_PDO = 0x00000303; /* 0x303 (0x303 + node 0x00) */
    UNS8  CanOpenMaster_obj1805_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1805_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1805_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1805_Event_Timer          = 33;       /* 33ms per Master.od */
    subindex CanOpenMaster_Index1805[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1805 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1805_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1805_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1805_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1805_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1805_Event_Timer }
    };

/* index 0x1806 : Transmit PDO 7 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1806 = 5;
    UNS32 CanOpenMaster_obj1806_COB_ID_used_by_PDO = 0x00000204; /* 0x204 (0x204 + node 0x00) */
    UNS8  CanOpenMaster_obj1806_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1806_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1806_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1806_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1806[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1806 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1806_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1806_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1806_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1806_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1806_Event_Timer }
    };

/* index 0x1807 : Transmit PDO 8 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1807 = 5;
    UNS32 CanOpenMaster_obj1807_COB_ID_used_by_PDO = 0x00000304; /* 0x304 (0x304 + node 0x00) */
    UNS8  CanOpenMaster_obj1807_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1807_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1807_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1807_Event_Timer          = 27;       /* 27ms per Master.od */
    subindex CanOpenMaster_Index1807[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1807 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1807_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1807_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1807_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1807_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1807_Event_Timer }
    };

/* index 0x1808 : Transmit PDO 9 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1808 = 5;
    UNS32 CanOpenMaster_obj1808_COB_ID_used_by_PDO = 0x00000205; /* 0x205 (0x205 + node 0x00) */
    UNS8  CanOpenMaster_obj1808_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1808_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1808_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1808_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1808[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1808 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1808_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1808_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1808_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1808_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1808_Event_Timer }
    };

/* index 0x1809 : Transmit PDO 10 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj1809 = 5;
    UNS32 CanOpenMaster_obj1809_COB_ID_used_by_PDO = 0x00000305; /* 0x305 (0x305 + node 0x00) */
    UNS8  CanOpenMaster_obj1809_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj1809_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj1809_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj1809_Event_Timer          = 0x0000;
    subindex CanOpenMaster_Index1809[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1809 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1809_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1809_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1809_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj1809_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj1809_Event_Timer }
    };

/* index 0x180A : Transmit PDO 11 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj180A = 5;
    UNS32 CanOpenMaster_obj180A_COB_ID_used_by_PDO = 0x00000206; /* 0x206 (0x206 + node 0x00) */
    UNS8  CanOpenMaster_obj180A_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj180A_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj180A_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj180A_Event_Timer          = 97;       /* 97ms per Master.od */
    subindex CanOpenMaster_Index180A[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj180A },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj180A_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180A_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180A_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180A_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180A_Event_Timer }
    };

/* index 0x180B : Transmit PDO 12 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj180B = 5;
    UNS32 CanOpenMaster_obj180B_COB_ID_used_by_PDO = 0x00000207; /* 0x207 (0x207 + node 0x00) */
    UNS8  CanOpenMaster_obj180B_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj180B_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj180B_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj180B_Event_Timer          = 1000;     /* 1000ms per Master.od */
    subindex CanOpenMaster_Index180B[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj180B },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj180B_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180B_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180B_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180B_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180B_Event_Timer }
    };

/* index 0x180C : Transmit PDO 13 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj180C = 5;
    UNS32 CanOpenMaster_obj180C_COB_ID_used_by_PDO = 0x00000307; /* 0x307 (0x307 + node 0x00) */
    UNS8  CanOpenMaster_obj180C_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj180C_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj180C_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj180C_Event_Timer          = 1000;     /* 1000ms per Master.od */
    subindex CanOpenMaster_Index180C[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj180C },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj180C_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180C_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180C_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180C_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180C_Event_Timer }
    };

/* index 0x180D : Transmit PDO 14 Parameter */
    UNS8 CanOpenMaster_highestSubIndex_obj180D = 5;
    UNS32 CanOpenMaster_obj180D_COB_ID_used_by_PDO = 0x00000306; /* 0x306 (0x306 + node 0x00) */
    UNS8  CanOpenMaster_obj180D_Transmission_Type   = 0xFF;
    UNS16 CanOpenMaster_obj180D_Inhibit_Time        = 0x0000;
    UNS8  CanOpenMaster_obj180D_Compatibility_Entry = 0x00;
    UNS16 CanOpenMaster_obj180D_Event_Timer          = 3000;     /* 3000ms per Master.od */
    subindex CanOpenMaster_Index180D[] = {
        { RO, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj180D },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj180D_COB_ID_used_by_PDO },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180D_Transmission_Type },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180D_Inhibit_Time },
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_obj180D_Compatibility_Entry },
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj180D_Event_Timer }
    };

/* ===== TPDO Mapping Parameters (0x1A00-0x1A0D) — 14 mappings ===== */

/* index 0x1A00 : Transmit PDO 1 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A00 = 2;
    UNS32 CanOpenMaster_obj1A00[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A00[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A00 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A00[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A00[1] }
    };

/* index 0x1A01 : Transmit PDO 2 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A01 = 2;
    UNS32 CanOpenMaster_obj1A01[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A01[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A01 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A01[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A01[1] }
    };

/* index 0x1A02 : Transmit PDO 3 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A02 = 2;
    UNS32 CanOpenMaster_obj1A02[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A02[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A02 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A02[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A02[1] }
    };

/* index 0x1A03 : Transmit PDO 4 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A03 = 2;
    UNS32 CanOpenMaster_obj1A03[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A03[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A03 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A03[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A03[1] }
    };

/* index 0x1A04 : Transmit PDO 5 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A04 = 2;
    UNS32 CanOpenMaster_obj1A04[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A04[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A04 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A04[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A04[1] }
    };

/* index 0x1A05 : Transmit PDO 6 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A05 = 2;
    UNS32 CanOpenMaster_obj1A05[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A05[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A05 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A05[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A05[1] }
    };

/* index 0x1A06 : Transmit PDO 7 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A06 = 2;
    UNS32 CanOpenMaster_obj1A06[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A06[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A06 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A06[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A06[1] }
    };

/* index 0x1A07 : Transmit PDO 8 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A07 = 2;
    UNS32 CanOpenMaster_obj1A07[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A07[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A07 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A07[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A07[1] }
    };

/* index 0x1A08 : Transmit PDO 9 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A08 = 2;
    UNS32 CanOpenMaster_obj1A08[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A08[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A08 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A08[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A08[1] }
    };

/* index 0x1A09 : Transmit PDO 10 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A09 = 2;
    UNS32 CanOpenMaster_obj1A09[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A09[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A09 },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A09[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A09[1] }
    };

/* index 0x1A0A : Transmit PDO 11 Mapping (1 mapping entry) */
    UNS8 CanOpenMaster_highestSubIndex_obj1A0A = 1;
    UNS32 CanOpenMaster_obj1A0A[] = { 0x00000000 };
    subindex CanOpenMaster_Index1A0A[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A0A },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A0A[0] }
    };

/* index 0x1A0B : Transmit PDO 12 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A0B = 2;
    UNS32 CanOpenMaster_obj1A0B[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A0B[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A0B },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A0B[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A0B[1] }
    };

/* index 0x1A0C : Transmit PDO 13 Mapping (1 mapping entry) */
    UNS8 CanOpenMaster_highestSubIndex_obj1A0C = 1;
    UNS32 CanOpenMaster_obj1A0C[] = { 0x00000000 };
    subindex CanOpenMaster_Index1A0C[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A0C },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A0C[0] }
    };

/* index 0x1A0D : Transmit PDO 14 Mapping */
    UNS8 CanOpenMaster_highestSubIndex_obj1A0D = 2;
    UNS32 CanOpenMaster_obj1A0D[] = { 0x00000000, 0x00000000 };
    subindex CanOpenMaster_Index1A0D[] = {
        { RW, uint8,  sizeof(UNS8),  (void*)&CanOpenMaster_highestSubIndex_obj1A0D },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A0D[0] },
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj1A0D[1] }
    };

/* ===== User-Defined Objects (0x2001-0x202B) — 43 entries ===== */
/* 5 axes (X/Y/Z/E/T/B) × modes + position + velocity + status_word + control_word + current_actual */
/* + photometer + temperature + weight sensors */

/* --- X-axis (0x2001-0x2005, 0x2028) --- */

/* index 0x2001 : mX_modes (INTEGER8) */
    UNS8 CanOpenMaster_obj2001 = 0;
    subindex CanOpenMaster_Index2001[] = {
        { RW, int8, sizeof(UNS8), (void*)&CanOpenMaster_obj2001 }
    };

/* index 0x2002 : mX_position (INTEGER32) */
    UNS32 CanOpenMaster_obj2002 = 0;
    subindex CanOpenMaster_Index2002[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2002 }
    };

/* index 0x2003 : mX_velocity (INTEGER32) */
    UNS32 CanOpenMaster_obj2003 = 0;
    subindex CanOpenMaster_Index2003[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2003 }
    };

/* index 0x2004 : mX_status_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2004 = 0;
    subindex CanOpenMaster_Index2004[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2004 }
    };

/* index 0x2005 : mX_control_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2005 = 0;
    subindex CanOpenMaster_Index2005[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2005 }
    };

/* --- Y-axis (0x2006-0x200A, 0x2029) --- */

/* index 0x2006 : mY_modes (INTEGER8) */
    UNS8 CanOpenMaster_obj2006 = 0;
    subindex CanOpenMaster_Index2006[] = {
        { RW, int8, sizeof(UNS8), (void*)&CanOpenMaster_obj2006 }
    };

/* index 0x2007 : mY_position (INTEGER32) */
    UNS32 CanOpenMaster_obj2007 = 0;
    subindex CanOpenMaster_Index2007[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2007 }
    };

/* index 0x2008 : mY_velocity (INTEGER32) */
    UNS32 CanOpenMaster_obj2008 = 0;
    subindex CanOpenMaster_Index2008[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2008 }
    };

/* index 0x2009 : mY_status_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2009 = 0;
    subindex CanOpenMaster_Index2009[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2009 }
    };

/* index 0x200A : mY_control_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj200A = 0;
    subindex CanOpenMaster_Index200A[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj200A }
    };

/* --- Z-axis (0x200B-0x200F, 0x202A) --- */

/* index 0x200B : mZ_modes (INTEGER8) */
    UNS8 CanOpenMaster_obj200B = 0;
    subindex CanOpenMaster_Index200B[] = {
        { RW, int8, sizeof(UNS8), (void*)&CanOpenMaster_obj200B }
    };

/* index 0x200C : mZ_position (INTEGER32) */
    UNS32 CanOpenMaster_obj200C = 0;
    subindex CanOpenMaster_Index200C[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj200C }
    };

/* index 0x200D : mZ_velocity (INTEGER32) */
    UNS32 CanOpenMaster_obj200D = 0;
    subindex CanOpenMaster_Index200D[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj200D }
    };

/* index 0x200E : mZ_status_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj200E = 0;
    subindex CanOpenMaster_Index200E[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj200E }
    };

/* index 0x200F : mZ_control_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj200F = 0;
    subindex CanOpenMaster_Index200F[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj200F }
    };

/* --- E-axis (0x2010-0x2014) --- */

/* index 0x2010 : mE_modes (INTEGER8) */
    UNS8 CanOpenMaster_obj2010 = 0;
    subindex CanOpenMaster_Index2010[] = {
        { RW, int8, sizeof(UNS8), (void*)&CanOpenMaster_obj2010 }
    };

/* index 0x2011 : mE_position (INTEGER32) */
    UNS32 CanOpenMaster_obj2011 = 0;
    subindex CanOpenMaster_Index2011[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2011 }
    };

/* index 0x2012 : mE_velocity (INTEGER32) */
    UNS32 CanOpenMaster_obj2012 = 0;
    subindex CanOpenMaster_Index2012[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2012 }
    };

/* index 0x2013 : mE_status_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2013 = 0;
    subindex CanOpenMaster_Index2013[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2013 }
    };

/* index 0x2014 : mE_control_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2014 = 0;
    subindex CanOpenMaster_Index2014[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2014 }
    };

/* --- T-axis (0x2015-0x2019) --- */

/* index 0x2015 : mT_modes (INTEGER8) */
    UNS8 CanOpenMaster_obj2015 = 0;
    subindex CanOpenMaster_Index2015[] = {
        { RW, int8, sizeof(UNS8), (void*)&CanOpenMaster_obj2015 }
    };

/* index 0x2016 : mT_position (INTEGER32) */
    UNS32 CanOpenMaster_obj2016 = 0;
    subindex CanOpenMaster_Index2016[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2016 }
    };

/* index 0x2017 : mT_velocity (INTEGER32) */
    UNS32 CanOpenMaster_obj2017 = 0;
    subindex CanOpenMaster_Index2017[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2017 }
    };

/* index 0x2018 : mT_status_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2018 = 0;
    subindex CanOpenMaster_Index2018[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2018 }
    };

/* index 0x2019 : mT_control_word (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2019 = 0;
    subindex CanOpenMaster_Index2019[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2019 }
    };

/* --- Photometer (0x201A-0x201C, 0x2022-0x2023) --- */

/* index 0x201A : photometer_ch0 (UNSIGNED16) */
    UNS16 CanOpenMaster_obj201A = 0;
    subindex CanOpenMaster_Index201A[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj201A }
    };

/* index 0x201B : photometer_ch1 (UNSIGNED16) */
    UNS16 CanOpenMaster_obj201B = 0;
    subindex CanOpenMaster_Index201B[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj201B }
    };

/* index 0x201C : photometer_led (UNSIGNED32) */
    UNS32 CanOpenMaster_obj201C = 0;
    subindex CanOpenMaster_Index201C[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj201C }
    };

/* index 0x2022 : photometer_rate (UNSIGNED16) */
    UNS16 CanOpenMaster_obj2022 = 0;
    subindex CanOpenMaster_Index2022[] = {
        { RW, uint16, sizeof(UNS16), (void*)&CanOpenMaster_obj2022 }
    };

/* index 0x2023 : photometer_gain (UNSIGNED8) */
    UNS8 CanOpenMaster_obj2023 = 0;
    subindex CanOpenMaster_Index2023[] = {
        { RW, uint8, sizeof(UNS8), (void*)&CanOpenMaster_obj2023 }
    };

/* --- Temperature Control (0x201D-0x2021, 0x2024) --- */

/* index 0x201D : TEMP_control_word (UNSIGNED32) */
    UNS32 CanOpenMaster_obj201D = 0;
    subindex CanOpenMaster_Index201D[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj201D }
    };

/* index 0x201E : heating_target (UNSIGNED32) */
    UNS32 CanOpenMaster_obj201E = 0;
    subindex CanOpenMaster_Index201E[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj201E }
    };

/* index 0x201F : refrigeration_target (UNSIGNED32) */
    UNS32 CanOpenMaster_obj201F = 0;
    subindex CanOpenMaster_Index201F[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj201F }
    };

/* index 0x2020 : current_heating (UNSIGNED32) */
    UNS32 CanOpenMaster_obj2020 = 0;
    subindex CanOpenMaster_Index2020[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj2020 }
    };

/* index 0x2021 : current_refrigeration (UNSIGNED32) */
    UNS32 CanOpenMaster_obj2021 = 0;
    subindex CanOpenMaster_Index2021[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj2021 }
    };

/* index 0x2024 : TEMP_status_word (UNSIGNED32) */
    UNS32 CanOpenMaster_obj2024 = 0;
    subindex CanOpenMaster_Index2024[] = {
        { RW, uint32, sizeof(UNS32), (void*)&CanOpenMaster_obj2024 }
    };

/* --- Weight Sensors (0x2025-0x2027) --- */

/* index 0x2025 : weight_clean_water (INTEGER32) */
    UNS32 CanOpenMaster_obj2025 = 0;
    subindex CanOpenMaster_Index2025[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2025 }
    };

/* index 0x2026 : weight_buff_liq (INTEGER32) */
    UNS32 CanOpenMaster_obj2026 = 0;
    subindex CanOpenMaster_Index2026[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2026 }
    };

/* index 0x2027 : weight_waste_liq (INTEGER32) */
    UNS32 CanOpenMaster_obj2027 = 0;
    subindex CanOpenMaster_Index2027[] = {
        { RW, int32, sizeof(UNS32), (void*)&CanOpenMaster_obj2027 }
    };

/* --- Current Feedback (0x2028-0x202B) --- */

/* index 0x2028 : mX_Current_actual (INTEGER16) */
    UNS16 CanOpenMaster_obj2028 = 0;
    subindex CanOpenMaster_Index2028[] = {
        { RW, int16, sizeof(UNS16), (void*)&CanOpenMaster_obj2028 }
    };

/* index 0x2029 : mY_Current_actual (INTEGER16) */
    UNS16 CanOpenMaster_obj2029 = 0;
    subindex CanOpenMaster_Index2029[] = {
        { RW, int16, sizeof(UNS16), (void*)&CanOpenMaster_obj2029 }
    };

/* index 0x202A : mZ_Current_actual (INTEGER16) */
    UNS16 CanOpenMaster_obj202A = 0;
    subindex CanOpenMaster_Index202A[] = {
        { RW, int16, sizeof(UNS16), (void*)&CanOpenMaster_obj202A }
    };

/* index 0x202B : mB_Current_actual (INTEGER16) */
    UNS16 CanOpenMaster_obj202B = 0;
    subindex CanOpenMaster_Index202B[] = {
        { RW, int16, sizeof(UNS16), (void*)&CanOpenMaster_obj202B }
    };

/**************************************************************************/
/*                          INDEX TABLE                                   */
/* Order: DS-301 → 0x00A0 → SDO_SVR → SDO_CLT → PDO_RCV → PDO_RCV_MAP   */
/*        → PDO_TRS → PDO_TRS_MAP → User objects (0x2001-0x202B)          */
/**************************************************************************/

const indextable CanOpenMaster_objdict[] = {
    /* [0-10] Standard DS-301 */
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
    /* [11] 0x00A0 type definition */
    { (subindex*)CanOpenMaster_Index00A0, sizeof(CanOpenMaster_Index00A0)/sizeof(CanOpenMaster_Index00A0[0]), 0x00A0 },
    /* [12] SDO_SVR first */
    { (subindex*)CanOpenMaster_Index1200, sizeof(CanOpenMaster_Index1200)/sizeof(CanOpenMaster_Index1200[0]), 0x1200 },
    /* [13-17] SDO_CLT */
    { (subindex*)CanOpenMaster_Index1280, sizeof(CanOpenMaster_Index1280)/sizeof(CanOpenMaster_Index1280[0]), 0x1280 },
    { (subindex*)CanOpenMaster_Index1281, sizeof(CanOpenMaster_Index1281)/sizeof(CanOpenMaster_Index1281[0]), 0x1281 },
    { (subindex*)CanOpenMaster_Index1282, sizeof(CanOpenMaster_Index1282)/sizeof(CanOpenMaster_Index1282[0]), 0x1282 },
    { (subindex*)CanOpenMaster_Index1283, sizeof(CanOpenMaster_Index1283)/sizeof(CanOpenMaster_Index1283[0]), 0x1283 },
    { (subindex*)CanOpenMaster_Index1284, sizeof(CanOpenMaster_Index1284)/sizeof(CanOpenMaster_Index1284[0]), 0x1284 },
    /* [18-27] PDO_RCV */
    { (subindex*)CanOpenMaster_Index1400, sizeof(CanOpenMaster_Index1400)/sizeof(CanOpenMaster_Index1400[0]), 0x1400 },
    { (subindex*)CanOpenMaster_Index1401, sizeof(CanOpenMaster_Index1401)/sizeof(CanOpenMaster_Index1401[0]), 0x1401 },
    { (subindex*)CanOpenMaster_Index1402, sizeof(CanOpenMaster_Index1402)/sizeof(CanOpenMaster_Index1402[0]), 0x1402 },
    { (subindex*)CanOpenMaster_Index1403, sizeof(CanOpenMaster_Index1403)/sizeof(CanOpenMaster_Index1403[0]), 0x1403 },
    { (subindex*)CanOpenMaster_Index1404, sizeof(CanOpenMaster_Index1404)/sizeof(CanOpenMaster_Index1404[0]), 0x1404 },
    { (subindex*)CanOpenMaster_Index1405, sizeof(CanOpenMaster_Index1405)/sizeof(CanOpenMaster_Index1405[0]), 0x1405 },
    { (subindex*)CanOpenMaster_Index1406, sizeof(CanOpenMaster_Index1406)/sizeof(CanOpenMaster_Index1406[0]), 0x1406 },
    { (subindex*)CanOpenMaster_Index1407, sizeof(CanOpenMaster_Index1407)/sizeof(CanOpenMaster_Index1407[0]), 0x1407 },
    { (subindex*)CanOpenMaster_Index1408, sizeof(CanOpenMaster_Index1408)/sizeof(CanOpenMaster_Index1408[0]), 0x1408 },
    { (subindex*)CanOpenMaster_Index1409, sizeof(CanOpenMaster_Index1409)/sizeof(CanOpenMaster_Index1409[0]), 0x1409 },
    /* [28-37] PDO_RCV_MAP */
    { (subindex*)CanOpenMaster_Index1600, sizeof(CanOpenMaster_Index1600)/sizeof(CanOpenMaster_Index1600[0]), 0x1600 },
    { (subindex*)CanOpenMaster_Index1601, sizeof(CanOpenMaster_Index1601)/sizeof(CanOpenMaster_Index1601[0]), 0x1601 },
    { (subindex*)CanOpenMaster_Index1602, sizeof(CanOpenMaster_Index1602)/sizeof(CanOpenMaster_Index1602[0]), 0x1602 },
    { (subindex*)CanOpenMaster_Index1603, sizeof(CanOpenMaster_Index1603)/sizeof(CanOpenMaster_Index1603[0]), 0x1603 },
    { (subindex*)CanOpenMaster_Index1604, sizeof(CanOpenMaster_Index1604)/sizeof(CanOpenMaster_Index1604[0]), 0x1604 },
    { (subindex*)CanOpenMaster_Index1605, sizeof(CanOpenMaster_Index1605)/sizeof(CanOpenMaster_Index1605[0]), 0x1605 },
    { (subindex*)CanOpenMaster_Index1606, sizeof(CanOpenMaster_Index1606)/sizeof(CanOpenMaster_Index1606[0]), 0x1606 },
    { (subindex*)CanOpenMaster_Index1607, sizeof(CanOpenMaster_Index1607)/sizeof(CanOpenMaster_Index1607[0]), 0x1607 },
    { (subindex*)CanOpenMaster_Index1608, sizeof(CanOpenMaster_Index1608)/sizeof(CanOpenMaster_Index1608[0]), 0x1608 },
    { (subindex*)CanOpenMaster_Index1609, sizeof(CanOpenMaster_Index1609)/sizeof(CanOpenMaster_Index1609[0]), 0x1609 },
    /* [38-51] PDO_TRS */
    { (subindex*)CanOpenMaster_Index1800, sizeof(CanOpenMaster_Index1800)/sizeof(CanOpenMaster_Index1800[0]), 0x1800 },
    { (subindex*)CanOpenMaster_Index1801, sizeof(CanOpenMaster_Index1801)/sizeof(CanOpenMaster_Index1801[0]), 0x1801 },
    { (subindex*)CanOpenMaster_Index1802, sizeof(CanOpenMaster_Index1802)/sizeof(CanOpenMaster_Index1802[0]), 0x1802 },
    { (subindex*)CanOpenMaster_Index1803, sizeof(CanOpenMaster_Index1803)/sizeof(CanOpenMaster_Index1803[0]), 0x1803 },
    { (subindex*)CanOpenMaster_Index1804, sizeof(CanOpenMaster_Index1804)/sizeof(CanOpenMaster_Index1804[0]), 0x1804 },
    { (subindex*)CanOpenMaster_Index1805, sizeof(CanOpenMaster_Index1805)/sizeof(CanOpenMaster_Index1805[0]), 0x1805 },
    { (subindex*)CanOpenMaster_Index1806, sizeof(CanOpenMaster_Index1806)/sizeof(CanOpenMaster_Index1806[0]), 0x1806 },
    { (subindex*)CanOpenMaster_Index1807, sizeof(CanOpenMaster_Index1807)/sizeof(CanOpenMaster_Index1807[0]), 0x1807 },
    { (subindex*)CanOpenMaster_Index1808, sizeof(CanOpenMaster_Index1808)/sizeof(CanOpenMaster_Index1808[0]), 0x1808 },
    { (subindex*)CanOpenMaster_Index1809, sizeof(CanOpenMaster_Index1809)/sizeof(CanOpenMaster_Index1809[0]), 0x1809 },
    { (subindex*)CanOpenMaster_Index180A, sizeof(CanOpenMaster_Index180A)/sizeof(CanOpenMaster_Index180A[0]), 0x180A },
    { (subindex*)CanOpenMaster_Index180B, sizeof(CanOpenMaster_Index180B)/sizeof(CanOpenMaster_Index180B[0]), 0x180B },
    { (subindex*)CanOpenMaster_Index180C, sizeof(CanOpenMaster_Index180C)/sizeof(CanOpenMaster_Index180C[0]), 0x180C },
    { (subindex*)CanOpenMaster_Index180D, sizeof(CanOpenMaster_Index180D)/sizeof(CanOpenMaster_Index180D[0]), 0x180D },
    /* [52-65] PDO_TRS_MAP */
    { (subindex*)CanOpenMaster_Index1A00, sizeof(CanOpenMaster_Index1A00)/sizeof(CanOpenMaster_Index1A00[0]), 0x1A00 },
    { (subindex*)CanOpenMaster_Index1A01, sizeof(CanOpenMaster_Index1A01)/sizeof(CanOpenMaster_Index1A01[0]), 0x1A01 },
    { (subindex*)CanOpenMaster_Index1A02, sizeof(CanOpenMaster_Index1A02)/sizeof(CanOpenMaster_Index1A02[0]), 0x1A02 },
    { (subindex*)CanOpenMaster_Index1A03, sizeof(CanOpenMaster_Index1A03)/sizeof(CanOpenMaster_Index1A03[0]), 0x1A03 },
    { (subindex*)CanOpenMaster_Index1A04, sizeof(CanOpenMaster_Index1A04)/sizeof(CanOpenMaster_Index1A04[0]), 0x1A04 },
    { (subindex*)CanOpenMaster_Index1A05, sizeof(CanOpenMaster_Index1A05)/sizeof(CanOpenMaster_Index1A05[0]), 0x1A05 },
    { (subindex*)CanOpenMaster_Index1A06, sizeof(CanOpenMaster_Index1A06)/sizeof(CanOpenMaster_Index1A06[0]), 0x1A06 },
    { (subindex*)CanOpenMaster_Index1A07, sizeof(CanOpenMaster_Index1A07)/sizeof(CanOpenMaster_Index1A07[0]), 0x1A07 },
    { (subindex*)CanOpenMaster_Index1A08, sizeof(CanOpenMaster_Index1A08)/sizeof(CanOpenMaster_Index1A08[0]), 0x1A08 },
    { (subindex*)CanOpenMaster_Index1A09, sizeof(CanOpenMaster_Index1A09)/sizeof(CanOpenMaster_Index1A09[0]), 0x1A09 },
    { (subindex*)CanOpenMaster_Index1A0A, sizeof(CanOpenMaster_Index1A0A)/sizeof(CanOpenMaster_Index1A0A[0]), 0x1A0A },
    { (subindex*)CanOpenMaster_Index1A0B, sizeof(CanOpenMaster_Index1A0B)/sizeof(CanOpenMaster_Index1A0B[0]), 0x1A0B },
    { (subindex*)CanOpenMaster_Index1A0C, sizeof(CanOpenMaster_Index1A0C)/sizeof(CanOpenMaster_Index1A0C[0]), 0x1A0C },
    { (subindex*)CanOpenMaster_Index1A0D, sizeof(CanOpenMaster_Index1A0D)/sizeof(CanOpenMaster_Index1A0D[0]), 0x1A0D },
    /* [66-108] User-defined objects (0x2001-0x202B in ascending order) */
    { (subindex*)CanOpenMaster_Index2001, sizeof(CanOpenMaster_Index2001)/sizeof(CanOpenMaster_Index2001[0]), 0x2001 },
    { (subindex*)CanOpenMaster_Index2002, sizeof(CanOpenMaster_Index2002)/sizeof(CanOpenMaster_Index2002[0]), 0x2002 },
    { (subindex*)CanOpenMaster_Index2003, sizeof(CanOpenMaster_Index2003)/sizeof(CanOpenMaster_Index2003[0]), 0x2003 },
    { (subindex*)CanOpenMaster_Index2004, sizeof(CanOpenMaster_Index2004)/sizeof(CanOpenMaster_Index2004[0]), 0x2004 },
    { (subindex*)CanOpenMaster_Index2005, sizeof(CanOpenMaster_Index2005)/sizeof(CanOpenMaster_Index2005[0]), 0x2005 },
    { (subindex*)CanOpenMaster_Index2006, sizeof(CanOpenMaster_Index2006)/sizeof(CanOpenMaster_Index2006[0]), 0x2006 },
    { (subindex*)CanOpenMaster_Index2007, sizeof(CanOpenMaster_Index2007)/sizeof(CanOpenMaster_Index2007[0]), 0x2007 },
    { (subindex*)CanOpenMaster_Index2008, sizeof(CanOpenMaster_Index2008)/sizeof(CanOpenMaster_Index2008[0]), 0x2008 },
    { (subindex*)CanOpenMaster_Index2009, sizeof(CanOpenMaster_Index2009)/sizeof(CanOpenMaster_Index2009[0]), 0x2009 },
    { (subindex*)CanOpenMaster_Index200A, sizeof(CanOpenMaster_Index200A)/sizeof(CanOpenMaster_Index200A[0]), 0x200A },
    { (subindex*)CanOpenMaster_Index200B, sizeof(CanOpenMaster_Index200B)/sizeof(CanOpenMaster_Index200B[0]), 0x200B },
    { (subindex*)CanOpenMaster_Index200C, sizeof(CanOpenMaster_Index200C)/sizeof(CanOpenMaster_Index200C[0]), 0x200C },
    { (subindex*)CanOpenMaster_Index200D, sizeof(CanOpenMaster_Index200D)/sizeof(CanOpenMaster_Index200D[0]), 0x200D },
    { (subindex*)CanOpenMaster_Index200E, sizeof(CanOpenMaster_Index200E)/sizeof(CanOpenMaster_Index200E[0]), 0x200E },
    { (subindex*)CanOpenMaster_Index200F, sizeof(CanOpenMaster_Index200F)/sizeof(CanOpenMaster_Index200F[0]), 0x200F },
    { (subindex*)CanOpenMaster_Index2010, sizeof(CanOpenMaster_Index2010)/sizeof(CanOpenMaster_Index2010[0]), 0x2010 },
    { (subindex*)CanOpenMaster_Index2011, sizeof(CanOpenMaster_Index2011)/sizeof(CanOpenMaster_Index2011[0]), 0x2011 },
    { (subindex*)CanOpenMaster_Index2012, sizeof(CanOpenMaster_Index2012)/sizeof(CanOpenMaster_Index2012[0]), 0x2012 },
    { (subindex*)CanOpenMaster_Index2013, sizeof(CanOpenMaster_Index2013)/sizeof(CanOpenMaster_Index2013[0]), 0x2013 },
    { (subindex*)CanOpenMaster_Index2014, sizeof(CanOpenMaster_Index2014)/sizeof(CanOpenMaster_Index2014[0]), 0x2014 },
    { (subindex*)CanOpenMaster_Index2015, sizeof(CanOpenMaster_Index2015)/sizeof(CanOpenMaster_Index2015[0]), 0x2015 },
    { (subindex*)CanOpenMaster_Index2016, sizeof(CanOpenMaster_Index2016)/sizeof(CanOpenMaster_Index2016[0]), 0x2016 },
    { (subindex*)CanOpenMaster_Index2017, sizeof(CanOpenMaster_Index2017)/sizeof(CanOpenMaster_Index2017[0]), 0x2017 },
    { (subindex*)CanOpenMaster_Index2018, sizeof(CanOpenMaster_Index2018)/sizeof(CanOpenMaster_Index2018[0]), 0x2018 },
    { (subindex*)CanOpenMaster_Index2019, sizeof(CanOpenMaster_Index2019)/sizeof(CanOpenMaster_Index2019[0]), 0x2019 },
    { (subindex*)CanOpenMaster_Index201A, sizeof(CanOpenMaster_Index201A)/sizeof(CanOpenMaster_Index201A[0]), 0x201A },
    { (subindex*)CanOpenMaster_Index201B, sizeof(CanOpenMaster_Index201B)/sizeof(CanOpenMaster_Index201B[0]), 0x201B },
    { (subindex*)CanOpenMaster_Index201C, sizeof(CanOpenMaster_Index201C)/sizeof(CanOpenMaster_Index201C[0]), 0x201C },
    { (subindex*)CanOpenMaster_Index201D, sizeof(CanOpenMaster_Index201D)/sizeof(CanOpenMaster_Index201D[0]), 0x201D },
    { (subindex*)CanOpenMaster_Index201E, sizeof(CanOpenMaster_Index201E)/sizeof(CanOpenMaster_Index201E[0]), 0x201E },
    { (subindex*)CanOpenMaster_Index201F, sizeof(CanOpenMaster_Index201F)/sizeof(CanOpenMaster_Index201F[0]), 0x201F },
    { (subindex*)CanOpenMaster_Index2020, sizeof(CanOpenMaster_Index2020)/sizeof(CanOpenMaster_Index2020[0]), 0x2020 },
    { (subindex*)CanOpenMaster_Index2021, sizeof(CanOpenMaster_Index2021)/sizeof(CanOpenMaster_Index2021[0]), 0x2021 },
    { (subindex*)CanOpenMaster_Index2022, sizeof(CanOpenMaster_Index2022)/sizeof(CanOpenMaster_Index2022[0]), 0x2022 },
    { (subindex*)CanOpenMaster_Index2023, sizeof(CanOpenMaster_Index2023)/sizeof(CanOpenMaster_Index2023[0]), 0x2023 },
    { (subindex*)CanOpenMaster_Index2024, sizeof(CanOpenMaster_Index2024)/sizeof(CanOpenMaster_Index2024[0]), 0x2024 },
    { (subindex*)CanOpenMaster_Index2025, sizeof(CanOpenMaster_Index2025)/sizeof(CanOpenMaster_Index2025[0]), 0x2025 },
    { (subindex*)CanOpenMaster_Index2026, sizeof(CanOpenMaster_Index2026)/sizeof(CanOpenMaster_Index2026[0]), 0x2026 },
    { (subindex*)CanOpenMaster_Index2027, sizeof(CanOpenMaster_Index2027)/sizeof(CanOpenMaster_Index2027[0]), 0x2027 },
    { (subindex*)CanOpenMaster_Index2028, sizeof(CanOpenMaster_Index2028)/sizeof(CanOpenMaster_Index2028[0]), 0x2028 },
    { (subindex*)CanOpenMaster_Index2029, sizeof(CanOpenMaster_Index2029)/sizeof(CanOpenMaster_Index2029[0]), 0x2029 },
    { (subindex*)CanOpenMaster_Index202A, sizeof(CanOpenMaster_Index202A)/sizeof(CanOpenMaster_Index202A[0]), 0x202A },
    { (subindex*)CanOpenMaster_Index202B, sizeof(CanOpenMaster_Index202B)/sizeof(CanOpenMaster_Index202B[0]), 0x202B },
};

/**************************************************************************/
/*                         SCAN INDEX                                     */
/**************************************************************************/

const indextable *CanOpenMaster_scanIndexOD(UNS16 wIndex,
    UNS32 *errorCode, ODCallback_t **callbacks)
{
    *callbacks = NULL;
    switch (wIndex) {
        /* Standard DS-301 */
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
        /* 0x00A0 */
        case 0x00A0: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[11];
        /* SDO_SVR */
        case 0x1200: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[12];
        /* SDO_CLT */
        case 0x1280: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[13];
        case 0x1281: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[14];
        case 0x1282: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[15];
        case 0x1283: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[16];
        case 0x1284: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[17];
        /* PDO_RCV */
        case 0x1400: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[18];
        case 0x1401: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[19];
        case 0x1402: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[20];
        case 0x1403: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[21];
        case 0x1404: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[22];
        case 0x1405: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[23];
        case 0x1406: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[24];
        case 0x1407: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[25];
        case 0x1408: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[26];
        case 0x1409: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[27];
        /* PDO_RCV_MAP */
        case 0x1600: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[28];
        case 0x1601: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[29];
        case 0x1602: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[30];
        case 0x1603: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[31];
        case 0x1604: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[32];
        case 0x1605: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[33];
        case 0x1606: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[34];
        case 0x1607: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[35];
        case 0x1608: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[36];
        case 0x1609: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[37];
        /* PDO_TRS */
        case 0x1800: *errorCode = OD_SUCCESSFUL; *callbacks = CanOpenMaster_Index1800_callbacks; return &CanOpenMaster_objdict[38];
        case 0x1801: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[39];
        case 0x1802: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[40];
        case 0x1803: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[41];
        case 0x1804: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[42];
        case 0x1805: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[43];
        case 0x1806: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[44];
        case 0x1807: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[45];
        case 0x1808: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[46];
        case 0x1809: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[47];
        case 0x180A: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[48];
        case 0x180B: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[49];
        case 0x180C: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[50];
        case 0x180D: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[51];
        /* PDO_TRS_MAP */
        case 0x1A00: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[52];
        case 0x1A01: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[53];
        case 0x1A02: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[54];
        case 0x1A03: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[55];
        case 0x1A04: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[56];
        case 0x1A05: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[57];
        case 0x1A06: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[58];
        case 0x1A07: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[59];
        case 0x1A08: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[60];
        case 0x1A09: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[61];
        case 0x1A0A: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[62];
        case 0x1A0B: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[63];
        case 0x1A0C: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[64];
        case 0x1A0D: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[65];
        /* User-defined objects (0x2001-0x202B) */
        case 0x2001: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[66];
        case 0x2002: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[67];
        case 0x2003: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[68];
        case 0x2004: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[69];
        case 0x2005: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[70];
        case 0x2006: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[71];
        case 0x2007: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[72];
        case 0x2008: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[73];
        case 0x2009: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[74];
        case 0x200A: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[75];
        case 0x200B: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[76];
        case 0x200C: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[77];
        case 0x200D: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[78];
        case 0x200E: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[79];
        case 0x200F: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[80];
        case 0x2010: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[81];
        case 0x2011: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[82];
        case 0x2012: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[83];
        case 0x2013: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[84];
        case 0x2014: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[85];
        case 0x2015: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[86];
        case 0x2016: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[87];
        case 0x2017: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[88];
        case 0x2018: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[89];
        case 0x2019: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[90];
        case 0x201A: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[91];
        case 0x201B: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[92];
        case 0x201C: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[93];
        case 0x201D: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[94];
        case 0x201E: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[95];
        case 0x201F: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[96];
        case 0x2020: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[97];
        case 0x2021: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[98];
        case 0x2022: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[99];
        case 0x2023: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[100];
        case 0x2024: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[101];
        case 0x2025: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[102];
        case 0x2026: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[103];
        case 0x2027: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[104];
        case 0x2028: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[105];
        case 0x2029: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[106];
        case 0x202A: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[107];
        case 0x202B: *errorCode = OD_SUCCESSFUL; return &CanOpenMaster_objdict[108];
        default:
            *errorCode = OD_NO_SUCH_OBJECT;
            return NULL;
    }
}

/**************************************************************************/
/*                       PDO STATUS                                       */
/**************************************************************************/

s_PDO_status CanOpenMaster_PDO_status[14] = {
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer,
    s_PDO_status_Initializer
};

/**************************************************************************/
/*                       QUICK INDEX                                      */
/**************************************************************************/

quick_index CanOpenMaster_firstIndex = {
    12,  /* SDO_SVR: 0x1200 */
    13,  /* SDO_CLT: 0x1280 */
    18,  /* PDO_RCV: 0x1400 */
    28,  /* PDO_RCV_MAP: 0x1600 */
    38,  /* PDO_TRS: 0x1800 */
    52   /* PDO_TRS_MAP: 0x1A00 */
};

quick_index CanOpenMaster_lastIndex = {
    12,  /* SDO_SVR: 0x1200 (only one) */
    17,  /* SDO_CLT: 0x1284 */
    27,  /* PDO_RCV: 0x1409 */
    37,  /* PDO_RCV_MAP: 0x1609 */
    51,  /* PDO_TRS: 0x180D */
    65   /* PDO_TRS_MAP: 0x1A0D */
};

UNS16 CanOpenMaster_ObjdictSize = sizeof(CanOpenMaster_objdict) /
                                  sizeof(CanOpenMaster_objdict[0]);

/**************************************************************************/
/*                      CO_Data INITIALIZATION                            */
/**************************************************************************/

CO_Data CanOpenMaster_Data = CANOPEN_NODE_DATA_INITIALIZER(CanOpenMaster);
