#include "canopen_master.h"
#include "reg_router.h"
#include "sampling.h"

#define DBG_TAG "canopen"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Directly accessed by CANfestival via CO_Data struct pointers */
extern UNS16 Master_obj1017;
extern UNS8  Master_highestSubIndex_obj1016;
extern UNS32 Master_obj1016[];

void TestMaster_initialisation(CO_Data *d)
{
    int i;

    /* Enable master heartbeat: 1000ms on COB-ID 0x700 */
    Master_obj1017 = 1000;

    /* Monitor slaves 1~8 heartbeat (3s timeout each) */
    Master_highestSubIndex_obj1016 = 8;
    for (i = 0; i < 8; i++) {
        Master_obj1016[i] = ((UNS32)(i + 1) << 16) | 0x0BB8;  /* nodeId | 3000ms */
    }

    LOG_I("Master: heartbeat 1s, monitoring slaves 1~8 (3s timeout)");
    (void)d;
}

void TestMaster_preOperational(CO_Data *d)
{
    LOG_I("Master: preOperational → Operational");
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

void TestMaster_post_sync(CO_Data *d)
{
    (void)d;
    reg_od_sync_in();            /* OD vars → Modbus regs */
    sampling_sync_can_to_modbus();  /* CAN TPDO → hole encoding + sensors */
}

void TestMaster_post_TPDO(CO_Data *d)
{
    (void)d;
    reg_od_sync_in();   /* Sync OD vars → Modbus regs after TPDO send */
}

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

    LOG_I("CANopen master started (Node ID=0x00, OD-defined slaves)");
    return 0;
}
