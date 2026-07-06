/*
 * CANfestival 平台配置: STM32F103 + RT-Thread
 * 参考 AT91 示例配置裁剪。
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* CANfestival 库需要的配置项 */
#define MAX_CAN_BUS_ID                  1
#define SDO_MAX_LENGTH_TRANSFER         32
#define SDO_MAX_SIMULTANEOUS_TRANSFERS  4
#define NMT_MAX_NODE_ID                 128
#define SDO_TIMEOUT_MS                  3000U
#define MAX_NB_TIMER                    8
#define EMCY_MAX_ERRORS                 8
#define SDO_BLOCK_SIZE                  4

/* 字节序 */
#define CANOPEN_LITTLE_ENDIAN           1

/* 定时器: 每 tick 1ms */
#define US_TO_TIMEVAL_FACTOR            1000

/* 结构体初始化使用的重复宏 */
#define REPEAT_SDO_MAX_SIMULTANEOUS_TRANSFERS_TIMES(repeat) \
    repeat repeat repeat repeat
#define REPEAT_EMCY_MAX_ERRORS_TIMES(repeat) \
    repeat repeat repeat repeat repeat repeat repeat repeat

#define REPEAT_NMT_MAX_NODE_ID_TIMES(repeat) \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat \
    repeat repeat repeat repeat repeat repeat repeat repeat

#endif /* _CONFIG_H_ */
