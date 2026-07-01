#include "canopen_master.h"

#define DBG_TAG "canopen"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static int init_step = 0;

/* ── Forward declarations ──────────────────────── */
static void ConfigureSlaveNode(CO_Data *d, UNS8 nodeId);
static void CheckSDOAndContinue(CO_Data *d, UNS8 nodeId);

/* ── SDO completion callback ───────────────────── */
static void CheckSDOAndContinue(CO_Data *d, UNS8 nodeId)
{
    UNS32 abortCode;

    if (getWriteResultNetworkDict(d, nodeId, &abortCode) != SDO_FINISHED) {
        LOG_E("SDO fail: node 0x%02X, step %d, abort=0x%04lX",
              nodeId, init_step, (unsigned long)abortCode);
    }
    closeSDOtransfer(d, nodeId, SDO_CLIENT);
    ConfigureSlaveNode(d, nodeId);
}

/* ── Chain-configure slave via SDO ─────────────── */
static void ConfigureSlaveNode(CO_Data *d, UNS8 nodeId)
{
    (void)d;
    switch (++init_step) {
    case 1: {   /* Disable slave TPDO1 */
        UNS32 cobid = 0x80000180 + nodeId;
        writeNetworkDictCallBack(d, nodeId, 0x1800, 0x01, 4, 0,
                                 &cobid, CheckSDOAndContinue, 0);
    } break;
    case 2: {   /* Set slave TPDO1 to SYNC transmit */
        UNS8 type = 0x01;
        writeNetworkDictCallBack(d, nodeId, 0x1800, 0x02, 1, 0,
                                 &type, CheckSDOAndContinue, 0);
    } break;
    case 3: {   /* Re-enable slave TPDO1 */
        UNS32 cobid = 0x00000180 + nodeId;
        writeNetworkDictCallBack(d, nodeId, 0x1800, 0x01, 4, 0,
                                 &cobid, CheckSDOAndContinue, 0);
    } break;
    case 4: {   /* Disable slave RPDO1 */
        UNS32 cobid = 0x80000200 + nodeId;
        writeNetworkDictCallBack(d, nodeId, 0x1400, 0x01, 4, 0,
                                 &cobid, CheckSDOAndContinue, 0);
    } break;
    case 5: {   /* Set slave RPDO1 receive type = async */
        UNS8 type = 0xFF;
        writeNetworkDictCallBack(d, nodeId, 0x1400, 0x02, 1, 0,
                                 &type, CheckSDOAndContinue, 0);
    } break;
    case 6: {   /* Re-enable slave RPDO1 */
        UNS32 cobid = 0x00000200 + nodeId;
        writeNetworkDictCallBack(d, nodeId, 0x1400, 0x01, 4, 0,
                                 &cobid, CheckSDOAndContinue, 0);
    } break;
    case 7: {   /* Set heartbeat producer time = 1000ms */
        UNS16 hb_time = 1000;
        writeNetworkDictCallBack(d, nodeId, 0x1017, 0x00, 2, 0,
                                 &hb_time, CheckSDOAndContinue, 0);
    } break;
    case 8: {   /* All done — start slave */
        LOG_I("Slave 0x%02X configured, sending NMT Start", nodeId);
        setState(d, Operational);
        masterSendNMTstateChange(d, nodeId, NMT_Start_Node);
        init_step = 0;
    } break;
    }
}

/* ── State transition callbacks ────────────────── */
/* Direct access to SDO client OD variables (defined in ObjDict.c) */
extern UNS32 CanOpenMaster_obj1280_COB_ID_Client_to_Server_Transmit_SDO;
extern UNS32 CanOpenMaster_obj1280_COB_ID_Server_to_Client_Receive_SDO;
extern UNS8  CanOpenMaster_obj1280_Node_ID_of_the_SDO_Server;

void TestMaster_initialisation(CO_Data *d)
{
    LOG_D("Master: initialisation");

    /* Bind RPDO1 to receive slave TPDO1 (COB-ID: 0x180 + slaveId) */
    UNS32 rpdo_cobid = 0x0180 + SLAVE_NODE_ID;
    UNS32 tpdo_cobid = 0x0200 + SLAVE_NODE_ID;
    UNS32 size4 = sizeof(UNS32);

    writeLocalDict(d, 0x1400, 0x01, &rpdo_cobid, &size4, RW);
    writeLocalDict(d, 0x1800, 0x01, &tpdo_cobid, &size4, RW);

    /* Directly patch SDO client OD for slave node (writeLocalDict doesn't
     * persist because CANfestival resets OD entries during state transition).
     * This makes GetSDOClientFromNodeId() find the correct slave node. */
    CanOpenMaster_obj1280_COB_ID_Client_to_Server_Transmit_SDO = 0x600 + SLAVE_NODE_ID;
    CanOpenMaster_obj1280_COB_ID_Server_to_Client_Receive_SDO  = 0x580 + SLAVE_NODE_ID;
    CanOpenMaster_obj1280_Node_ID_of_the_SDO_Server            = SLAVE_NODE_ID;

    LOG_I("SDO client patched: node=0x%02X, TX=0x%04lX, RX=0x%04lX",
          SLAVE_NODE_ID,
          (unsigned long)CanOpenMaster_obj1280_COB_ID_Client_to_Server_Transmit_SDO,
          (unsigned long)CanOpenMaster_obj1280_COB_ID_Server_to_Client_Receive_SDO);
}

void TestMaster_preOperational(CO_Data *d)
{
    LOG_I("Master: preOperational, configuring slaves");
    init_step = 0;
    ConfigureSlaveNode(d, SLAVE_NODE_ID);
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

void TestMaster_post_sync(CO_Data *d)
{
    (void)d;
}

void TestMaster_post_TPDO(CO_Data *d)
{
    (void)d;
}

void TestMaster_heartbeatError(CO_Data *d, UNS8 hbID)
{
    (void)d;
    LOG_E("Heartbeat lost: node 0x%02X", hbID);
}

void TestMaster_post_SlaveBootup(CO_Data *d, UNS8 nodeId)
{
    LOG_I("Slave 0x%02X booted, starting", nodeId);
    masterSendNMTstateChange(d, nodeId, NMT_Start_Node);
}

/* ── Init ─────────────────────────────────────── */
static void InitNodes(CO_Data *d, UNS32 id)
{
    (void)id;
    setNodeId(&CanOpenMaster_Data, 0x00);
    setState(&CanOpenMaster_Data, Initialisation);
}

int canopen_master_init(void)
{
    LOG_I("Initialising CANopen master...");

    /* 1. Hardware CAN init */
    can_hardware_init();

    /* 2. Set CANopen state callbacks */
    CanOpenMaster_Data.initialisation   = TestMaster_initialisation;
    CanOpenMaster_Data.preOperational   = TestMaster_preOperational;
    CanOpenMaster_Data.operational      = TestMaster_operational;
    CanOpenMaster_Data.stopped          = TestMaster_stopped;
    CanOpenMaster_Data.post_sync        = TestMaster_post_sync;
    CanOpenMaster_Data.post_TPDO        = TestMaster_post_TPDO;
    CanOpenMaster_Data.heartbeatError   = TestMaster_heartbeatError;
    CanOpenMaster_Data.post_SlaveBootup = TestMaster_post_SlaveBootup;

    /* 3. Start CANopen timer loop (periodic TimeDispatch) */
    StartTimerLoop(NULL);

    /* 4. Init CANopen node once */
    InitNodes(&CanOpenMaster_Data, 0);

    LOG_I("CANopen master started (Node ID=0x00, Slave=0x%02X)", SLAVE_NODE_ID);
    return 0;
}
