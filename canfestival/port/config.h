/*
 * CANfestival platform config for STM32F103 + RT-Thread
 * Based on AT91 example config.h
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Needed defines by Canfestival lib */
#define MAX_CAN_BUS_ID                  1
#define SDO_MAX_LENGTH_TRANSFER         32
#define SDO_MAX_SIMULTANEOUS_TRANSFERS  1
#define NMT_MAX_NODE_ID                 128
#define SDO_TIMEOUT_MS                  3000U
#define MAX_NB_TIMER                    8
#define EMCY_MAX_ERRORS                 8
#define SDO_BLOCK_SIZE                  4

/* Endianness */
#define CANOPEN_LITTLE_ENDIAN           1

/* Timer: 1ms per tick */
#define US_TO_TIMEVAL_FACTOR            1000

/* REPEAT macros for struct initializers */
#define REPEAT_SDO_MAX_SIMULTANEOUS_TRANSFERS_TIMES(repeat)   repeat
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
