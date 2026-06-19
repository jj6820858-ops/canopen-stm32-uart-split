/*
 * FreeModbus Libary: user callback functions - 快检设备 protocol
 * Bridges Modbus requests to CANopen via reg_router.
 *
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 * Modifications for 快检设备 protocol (2026)
 */
#include "user_mb_app.h"
#include <reg_router.h>
#include <string.h>

#define DBG_TAG "modbus"
#define DBG_LVL DBG_LOG
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

    /* DEBUG: log every callback entry */
    LOG_I("HOLDING CB: %s addr=%d n=%d",
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
        /* Try CANopen first (reg_router), fall back to local buffer */
        USHORT i;
        int ret = reg_read(usAddress + 1, usNRegs, pucRegBuffer);
        if (ret >= 0) {
            LOG_I("RD addr=%d cnt=%d (CANopen OK %dB)", usAddress + 1, usNRegs, ret);
            break;
        }
        /* Fallback: read from local buffer */
        LOG_I("RD addr=%d cnt=%d (local buffer)", usAddress + 1, usNRegs);
        for (i = 0; i < usNRegs; i++) {
            pucRegBuffer[i * 2]     = (UCHAR)(usSRegHoldBuf[usAddress + i] >> 8);
            pucRegBuffer[i * 2 + 1] = (UCHAR)(usSRegHoldBuf[usAddress + i] & 0xFF);
        }
        /* Print values for debugging */
        for (i = 0; i < usNRegs && i < 8; i++) {
            LOG_I("  [%d] = 0x%04X", usAddress + 1 + i, usSRegHoldBuf[usAddress + i]);
        }
        break;
    }

    case MB_REG_WRITE: {
        /* Write to local buffer (immediate, non-blocking) */
        USHORT i;
        uint16_t val = (pucRegBuffer[0] << 8) | pucRegBuffer[1];
        LOG_I("WR addr=%d val=0x%04X (%d)", usAddress + 1, val, val);
        usSRegHoldBuf[usAddress] = val;
        /* CANopen sync handled by background task - don't block here */
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
