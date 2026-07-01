/*
 * FreeModbus Libary: user callback functions - 快检设备 protocol
 * Bridges Modbus requests to CANopen local OD via reg_router.
 *
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 * Modifications for 快检设备 protocol (2026)
 *
 * Data flow:
 *   READ:  Modbus ← reg_read() → CANopen local OD (getODentry)
 *          Falls back to usSRegHoldBuf[] if no route / OD error.
 *   WRITE: Modbus → usSRegHoldBuf[] (immediate)
 *          → reg_write_async() → CANopen local OD (setODentry)
 */
#include "user_mb_app.h"
#include <reg_router.h>
#include <string.h>

#define DBG_TAG "modbus"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/*------------------------Slave mode buffers (unused, keep for compilation)------*/
USHORT   usSDiscInStart                               = S_DISCRETE_INPUT_START;
UCHAR    ucSDiscInBuf[1]                               = {0};
USHORT   usSCoilStart                                  = S_COIL_START;
UCHAR    ucSCoilBuf[1]                                 = {0};
USHORT   usSRegInStart                                 = S_REG_INPUT_START;
USHORT   usSRegInBuf[1]                                = {0};
USHORT   usSRegHoldStart                               = S_REG_HOLDING_START;
USHORT   usSRegHoldBuf[S_REG_HOLDING_NREGS]            = {0};

/* Helper: convert usAddress (1-based) to 0-based offset for reg_router */
#define ADDR_TO_OFFSET(a)  ((a) - 1)

/* ════════════════════════════════════════════════════════════════════════════
 * Holding Register callback
 *
 * Maps Modbus holding register reads/writes to CANopen via reg_router.
 * Falls back to local usSRegHoldBuf when CANopen is unavailable (testing).
 * Addresses are 1-based, matching the 快检设备 protocol spec.
 *
 * Protocol register map:
 *   00001~00033  - Control & Status
 *   00100~00104  - Real-time Data
 *   00120~00198  - Parameter Settings
 * ════════════════════════════════════════════════════════════════════════════ */
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                             USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;

    LOG_D("HOLDING CB: %s addr=%d n=%d",
          (eMode == MB_REG_READ) ? "READ" : "WRITE",
          usAddress, usNRegs);

    /* Convert 1-based to 0-based */
    usAddress--;

    /* Validate range */
    if (usAddress + usNRegs > S_REG_HOLDING_NREGS) {
        LOG_E("OUT OF RANGE: addr=%d n=%d max=%d",
              usAddress + 1, usNRegs, S_REG_HOLDING_NREGS);
        return MB_ENOREG;
    }

    switch (eMode) {
    case MB_REG_READ: {
        /* Try CANopen local OD first (reg_router), fall back to local buffer */
        USHORT i;
        int ret = reg_read(usAddress + 1, usNRegs, pucRegBuffer);
        if (ret >= 0) {
            LOG_D("RD addr=%d cnt=%d (local OD OK %dB)", usAddress + 1, usNRegs, ret);
            break;
        }
        /* Fallback: read from local buffer */
        LOG_D("RD addr=%d cnt=%d (local buffer)", usAddress + 1, usNRegs);
        for (i = 0; i < usNRegs; i++) {
            pucRegBuffer[i * 2]     = (UCHAR)(usSRegHoldBuf[usAddress + i] >> 8);
            pucRegBuffer[i * 2 + 1] = (UCHAR)(usSRegHoldBuf[usAddress + i] & 0xFF);
        }
        /* Print values for debugging */
        for (i = 0; i < usNRegs && i < 8; i++) {
            LOG_D("  [%d] = 0x%04X", usAddress + 1 + i, usSRegHoldBuf[usAddress + i]);
        }
        break;
    }

    case MB_REG_WRITE: {
        /* Write to local buffer (immediate) */
        USHORT i;
        uint8_t *pSrc = pucRegBuffer;
        LOG_I("WR addr=%d n=%d", usAddress + 1, usNRegs);
        for (i = 0; i < usNRegs; i++) {
            usSRegHoldBuf[usAddress + i] = (pSrc[0] << 8) | pSrc[1];
            pSrc += 2;
        }
        /* Sync to CANopen local OD (non-blocking) */
        if (reg_write_async(usAddress + 1, usNRegs, pucRegBuffer, NULL) != 0) {
            LOG_W("CANopen local OD write failed");
        }
        break;
    }

    default:
        eStatus = MB_ENOREG;
        break;
    }

    return eStatus;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Input Register callback  (not used in this protocol)
 * ════════════════════════════════════════════════════════════════════════════ */
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    return MB_ENOREG;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Coils callback  (not used in this protocol)
 * ════════════════════════════════════════════════════════════════════════════ */
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress,
                           USHORT usNCoils, eMBRegisterMode eMode)
{
    return MB_ENOREG;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Discrete Inputs callback  (not used in this protocol)
 * ════════════════════════════════════════════════════════════════════════════ */
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNDiscrete)
{
    return MB_ENOREG;
}
