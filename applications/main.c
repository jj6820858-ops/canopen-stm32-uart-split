/*
 * CANopen Master Protocol Gateway
 * RT-Thread + STM32F103RCT6 + CANfestival
 *
 * Architecture:
 *   Host PC --Modbus RTU(Slave)-->> STM32(CANopen Master) --CANopen-->> Slave MCU
 */

#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "canopen_master.h"
#include "reg_router.h"
#include "../packages/freemodbus/modbus/include/mb.h"
#include "../packages/freemodbus/modbus/include/mbport.h"
#include "../packages/freemodbus/port/user_mb_app.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Modbus slave address (1~247) */
#define MODBUS_SLAVE_ADDR       CONFIG_MODBUS_SLAVE_ADDR
/* Modbus serial port number */
#define MODBUS_SERIAL_PORT      CONFIG_MODBUS_SERIAL_PORT
/* Modbus baud rate */
#define MODBUS_BAUD_RATE        CONFIG_MODBUS_BAUD_RATE
/* Modbus parity */
#define MODBUS_PARITY           CONFIG_MODBUS_PARITY

static int freemodbus_slave_init(void)
{
    eMBErrorCode err;

    err = eMBInit(MB_RTU, MODBUS_SLAVE_ADDR, MODBUS_SERIAL_PORT,
                  MODBUS_BAUD_RATE, MODBUS_PARITY);
    if (err != MB_ENOERR) {
        LOG_E("FreeModbus init failed! err=%d", err);
        return -1;
    }

    err = eMBEnable();
    if (err != MB_ENOERR) {
        LOG_E("FreeModbus enable failed! err=%d", err);
        return -1;
    }

    LOG_I("FreeModbus Slave ready (addr=%d, uart%d, %dbps)",
          MODBUS_SLAVE_ADDR, MODBUS_SERIAL_PORT, MODBUS_BAUD_RATE);
    return 0;
}

/* ── MSH Test Commands ─────────────────────────────────────────── */

/* Share the same holding register buffer with FreeModbus */
extern USHORT usSRegHoldBuf[];

/** mb_read <addr> [count]  - read holding register(s) */
void mb_read(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: mb_read <addr(1-based)> [count]\n");
        return;
    }
    int addr = atoi(argv[1]);
    int cnt = (argc >= 3) ? atoi(argv[2]) : 1;
    if (addr < 1 || addr + cnt > S_REG_HOLDING_NREGS + 1) {
        rt_kprintf("Invalid range (1~%d)\n", S_REG_HOLDING_NREGS);
        return;
    }
    for (int i = 0; i < cnt; i++) {
        rt_kprintf("  %04d: 0x%04X (%d)\n",
                   addr + i, usSRegHoldBuf[addr - 1 + i],
                   usSRegHoldBuf[addr - 1 + i]);
    }
    rt_kprintf("  (%d registers)\n", cnt);
}
MSH_CMD_EXPORT(mb_read, "read Modbus holding registers");

/** mb_write <addr> <value>  - write holding register */
void mb_write(int argc, char **argv)
{
    if (argc < 3) {
        rt_kprintf("Usage: mb_write <addr(1-based)> <value>\n");
        return;
    }
    int addr = atoi(argv[1]);
    int val = atoi(argv[2]);
    if (addr < 1 || addr > S_REG_HOLDING_NREGS) {
        rt_kprintf("Invalid addr (1~%d)\n", S_REG_HOLDING_NREGS);
        return;
    }
    usSRegHoldBuf[addr - 1] = (uint16_t)val;
    rt_kprintf("  %04d <- 0x%04X (%d)\n", addr, (uint16_t)val, (uint16_t)val);
}
MSH_CMD_EXPORT(mb_write, "write Modbus holding registers");

/** mb_sim <addr> [count] - simulate Modbus Master read */
void mb_sim(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: mb_sim <addr(1-based)> [count]\n");
        return;
    }
    int addr = atoi(argv[1]);
    int cnt = (argc >= 3) ? atoi(argv[2]) : 1;
    if (addr < 1 || addr + cnt > S_REG_HOLDING_NREGS + 1) {
        rt_kprintf("Invalid range (1~%d)\n", S_REG_HOLDING_NREGS);
        return;
    }
    UCHAR buf[256];
    int ret = reg_read(addr, cnt, buf);
    rt_kprintf("CANopen: %s\n", (ret >= 0) ? "OK" : "FAIL (use local)");
    for (int i = 0; i < cnt; i++) {
        rt_kprintf("  REG[%04d] = 0x%04X (%d)\n",
                   addr + i, usSRegHoldBuf[addr - 1 + i],
                   usSRegHoldBuf[addr - 1 + i]);
    }
}
MSH_CMD_EXPORT(mb_sim, "simulate Modbus read via CANopen");

/** mb_dump  - dump entire register map */
void mb_dump(int argc, char **argv)
{
    rt_kprintf("Holding Register Map (1..%d):\n", S_REG_HOLDING_NREGS);
    for (int i = 0; i < S_REG_HOLDING_NREGS; i += 8) {
        rt_kprintf("  %03d:", i + 1);
        for (int j = 0; j < 8 && i + j < S_REG_HOLDING_NREGS; j++) {
            rt_kprintf(" %04X", usSRegHoldBuf[i + j]);
        }
        rt_kprintf("\n");
    }
}
MSH_CMD_EXPORT(mb_dump, "dump all Modbus holding registers");

/** mb_test [addr]  - write-read-verify loop on a register */
void mb_test(int argc, char **argv)
{
    int addr = (argc >= 2) ? atoi(argv[1]) : 4;
    if (addr < 1 || addr > S_REG_HOLDING_NREGS) {
        rt_kprintf("Invalid addr (1~%d)\n", S_REG_HOLDING_NREGS);
        return;
    }
    uint16_t orig = usSRegHoldBuf[addr - 1];
    uint16_t test_val = (orig == 0xAAAA) ? 0x5555 : 0xAAAA;
    int pass = 0, fail = 0;

    rt_kprintf("Modbus Loopback Test (addr=%d)\n", addr);
    rt_kprintf("  Original:  0x%04X (%d)\n", orig, orig);

    /* Test write */
    usSRegHoldBuf[addr - 1] = test_val;
    uint16_t read = usSRegHoldBuf[addr - 1];
    if (read == test_val) {
        rt_kprintf("  WRITE 0x%04X:  PASS (read=0x%04X)\n", test_val, read);
        pass++;
    } else {
        rt_kprintf("  WRITE 0x%04X:  FAIL (read=0x%04X)\n", test_val, read);
        fail++;
    }

    /* Test readback with CANopen */
    UCHAR buf[4];
    int ret = reg_read(addr, 1, buf);
    if (ret >= 0) {
        uint16_t can_val = (buf[0] << 8) | buf[1];
        rt_kprintf("  CANopen read: PASS (val=0x%04X, %d bytes)\n", can_val, ret);
    } else {
        rt_kprintf("  CANopen read: skipped (no slave)\n");
    }

    /* Restore */
    usSRegHoldBuf[addr - 1] = orig;
    rt_kprintf("  Restored: 0x%04X\n", orig);
    rt_kprintf("  Result: %d/%d passed\n", pass, pass + fail);
}
MSH_CMD_EXPORT(mb_test, "write-read-verify test on a register");

int main(void)
{
    LOG_I("====================================");
    LOG_I(" CANopen Master + Modbus Slave GW");
    LOG_I(" STM32F103RCT6, RT-Thread v4.0.3");
    LOG_I("====================================");

    /* 1. Init routing table (no dependencies) */
    reg_router_init();

    /* 2. Init CANopen master (CAN hardware + CANfestival + timer loop) */
    if (canopen_master_init() != 0) {
        LOG_E("CANopen master init failed! Halting.");
        while (1) {
            rt_thread_mdelay(1000);
        }
    }

    /* 3. Init FreeModbus Slave (UART2 Modbus RTU) */
    if (freemodbus_slave_init() != 0) {
        LOG_E("FreeModbus slave init failed! Halting.");
        while (1) {
            rt_thread_mdelay(1000);
        }
    }

    LOG_I("====================================");
    LOG_I(" System ready.");
    LOG_I(" UART1: PA9  PA10   @ 115200 (console)");
    LOG_I(" UART2: PA2  PA3    @ %d bps (Modbus Slave RTU)", MODBUS_BAUD_RATE);
    LOG_I(" CAN1:  PA12 PA11   @ 50K             ");
    LOG_I("====================================");

    while (1) {
        /* Poll FreeModbus for incoming requests */
        eMBPoll();
        rt_thread_mdelay(10);
    }

    return RT_EOK;
}
