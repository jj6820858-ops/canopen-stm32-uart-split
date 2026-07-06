/*
 * FreeModbus 用户回调 - 快检设备协议
 *
 * Modbus 请求通过 reg_router 转接到 CANopen 本地对象字典。
 *
 * 数据流:
 *   读: Modbus -> reg_read() -> CANopen 本地对象字典
 *       路由失败时回退到 usSRegHoldBuf[]。
 *   写: Modbus -> usSRegHoldBuf[] -> reg_write_async() -> CANopen 本地对象字典
 */
#include "user_mb_app.h"
#include "../../../applications/reg_router.h"
#include <string.h>

#define DBG_TAG "modbus"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 从机模式缓冲区: 当前协议不用线圈/离散量，但保留以满足 FreeModbus 编译 */
USHORT   usSDiscInStart                               = S_DISCRETE_INPUT_START;
UCHAR    ucSDiscInBuf[1]                               = {0};
USHORT   usSCoilStart                                  = S_COIL_START;
UCHAR    ucSCoilBuf[1]                                 = {0};
USHORT   usSRegInStart                                 = S_REG_INPUT_START;
USHORT   usSRegInBuf[1]                                = {0};
USHORT   usSRegHoldStart                               = S_REG_HOLDING_START;
USHORT   usSRegHoldBuf[S_REG_HOLDING_NREGS]            = {0};

/* 将 FreeModbus 的 1 起始地址转换为 reg_router 使用的 0 起始地址 */
#define ADDR_TO_OFFSET(a)  ((a) - 1)

/*
 * 保持寄存器回调
 *
 * 地址按协议文档使用 1 起始，进入 reg_router 前会转换为 0 起始。
 * 当路由不可用时，读操作回退到本地 usSRegHoldBuf[]，便于离线测试。
 */
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                             USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;

    LOG_D("HOLDING CB: %s addr=%d n=%d",
          (eMode == MB_REG_READ) ? "READ" : "WRITE",
          usAddress, usNRegs);

    /* 1 起始转 0 起始 */
    usAddress--;

    /* 范围检查 */
    if (usAddress + usNRegs > S_REG_HOLDING_NREGS) {
        LOG_E("OUT OF RANGE: addr=%d n=%d max=%d",
              usAddress + 1, usNRegs, S_REG_HOLDING_NREGS);
        return MB_ENOREG;
    }

    switch (eMode) {
    case MB_REG_READ: {
        USHORT i;
        int ret = reg_read(usAddress, usNRegs, pucRegBuffer);
        if (ret >= 0) {
            LOG_D("RD addr=%d cnt=%d (OD sync %dB)", usAddress + 1, usNRegs, ret);
            break;
        }
        /* 路由失败时读取本地缓存 */
        LOG_D("RD addr=%d cnt=%d (local buffer)", usAddress + 1, usNRegs);
        for (i = 0; i < usNRegs; i++) {
            pucRegBuffer[i * 2]     = (UCHAR)(usSRegHoldBuf[usAddress + i] >> 8);
            pucRegBuffer[i * 2 + 1] = (UCHAR)(usSRegHoldBuf[usAddress + i] & 0xFF);
        }
        break;
    }

    case MB_REG_WRITE: {
        USHORT i;
        uint8_t *pSrc = pucRegBuffer;
        LOG_I("WR addr=%d n=%d", usAddress + 1, usNRegs);
        for (i = 0; i < usNRegs; i++) {
            usSRegHoldBuf[usAddress + i] = (pSrc[0] << 8) | pSrc[1];
            pSrc += 2;
        }
        /* 非阻塞同步到 CANopen 对象字典 */
        if (reg_write_async(usAddress, usNRegs, pucRegBuffer, NULL) != 0) {
            LOG_W("CANopen OD write failed");
        }
        break;
    }

    default:
        eStatus = MB_ENOREG;
        break;
    }

    return eStatus;
}

/* 输入寄存器回调: 本协议未使用 */
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    return MB_ENOREG;
}

/* 线圈回调: 本协议未使用 */
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress,
                           USHORT usNCoils, eMBRegisterMode eMode)
{
    return MB_ENOREG;
}

/* 离散输入回调: 本协议未使用 */
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNDiscrete)
{
    return MB_ENOREG;
}
