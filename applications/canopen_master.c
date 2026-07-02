#include "canopen_master.h"

#define DBG_TAG "canopen"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* SDO client OD variables (defined in ObjDict.c) */
extern UNS32 Master_obj1280_COB_ID_Client_to_Server_Transmit_SDO;
extern UNS32 Master_obj1280_COB_ID_Server_to_Client_Receive_SDO;
extern UNS8  Master_obj1280_Node_ID_of_the_SDO_Server;

void TestMaster_initialisation(CO_Data *d)
{
    LOG_D("Master: initialisation");

    /* PDO COB-ID override: RPDO1=0x181, TPDO1=0x201 (standard for node 1) */
    UNS32 rpdo_cobid = 0x0180 + SLAVE_NODE_ID;
    UNS32 tpdo_cobid = 0x0200 + SLAVE_NODE_ID;
    UNS32 size4 = sizeof(UNS32);

    writeLocalDict(d, 0x1400, 0x01, &rpdo_cobid, &size4, RW);
    writeLocalDict(d, 0x1800, 0x01, &tpdo_cobid, &size4, RW);

    /* Patch SDO client for slave node */
    Master_obj1280_COB_ID_Client_to_Server_Transmit_SDO = 0x600 + SLAVE_NODE_ID;
    Master_obj1280_COB_ID_Server_to_Client_Receive_SDO  = 0x580 + SLAVE_NODE_ID;
    Master_obj1280_Node_ID_of_the_SDO_Server            = SLAVE_NODE_ID;

    LOG_I("Master init: slave=0x%02X, RPDO1=0x%04lX, TPDO1=0x%04lX",
          SLAVE_NODE_ID, (unsigned long)rpdo_cobid, (unsigned long)tpdo_cobid);
}

void TestMaster_preOperational(CO_Data *d)
{
    LOG_I("Master: Operational (PDOs enabled)");
    setState(d, Operational);
}

void TestMaster_operational(CO_Data *d)
{
    (void)d;
    LOG_I("Master: operational");
}

void TestMaster_stopped(CO_Data *d)
{
    (void)d;
    LOG_I("Master: stopped");
}

void TestMaster_post_sync(CO_Data *d)       { (void)d; }
void TestMaster_post_TPDO(CO_Data *d)       { (void)d; }

void TestMaster_heartbeatError(CO_Data *d, UNS8 hbID)
{
    (void)d;
    LOG_E("Heartbeat lost: node 0x%02X", hbID);
}

void TestMaster_post_SlaveBootup(CO_Data *d, UNS8 nodeId)
{
    LOG_I("Slave 0x%02X booted", nodeId);
    masterSendNMTstateChange(d, nodeId, NMT_Start_Node);
}

static void InitNodes(CO_Data *d, UNS32 id)
{
    (void)id;
    setNodeId(&Master_Data, 0x00);
    setState(&Master_Data, Initialisation);
}

int canopen_master_init(void)
{
    LOG_I("Initialising CANopen master...");

    can_hardware_init();

    Master_Data.initialisation   = TestMaster_initialisation;
    Master_Data.preOperational   = TestMaster_preOperational;
    Master_Data.operational      = TestMaster_operational;
    Master_Data.stopped          = TestMaster_stopped;
    Master_Data.post_sync        = TestMaster_post_sync;
    Master_Data.post_TPDO        = TestMaster_post_TPDO;
    Master_Data.heartbeatError   = TestMaster_heartbeatError;
    Master_Data.post_SlaveBootup = TestMaster_post_SlaveBootup;

    StartTimerLoop(NULL);
    InitNodes(&Master_Data, 0);

    LOG_I("CANopen master started (Node ID=0x00, Slave=0x%02X)", SLAVE_NODE_ID);
    return 0;
}
