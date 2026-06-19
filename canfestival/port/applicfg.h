/*
 * Platform configuration for STM32F103 + RT-Thread
 * No RT-Thread headers included to avoid GCC14 signal conflict
 */

#ifndef __APPLICFG_H__
#define __APPLICFG_H__

#include <string.h>

/* ── Integer types (CANfestival requires these) ── */
#define INTEGER8    signed char
#define INTEGER16   signed short
#define INTEGER32   signed long
#define INTEGER40   long long
#define INTEGER48   long long
#define INTEGER56   long long
#define INTEGER64   long long
#define UNS8        unsigned char
#define UNS16       unsigned short
#define UNS24       unsigned int
#define UNS32       unsigned long
#define UNS40       unsigned long long
#define UNS48       unsigned long long
#define UNS56       unsigned long long
#define UNS64       unsigned long long

/* ── CAN bus count ── */
#define MAX_CAN_BUS_ID  1

/* ── Disable dynamic loading ── */
#define NOT_USE_DYNAMIC_LOADING

/* ── Enable master callbacks ── */
#define CO_MASTER_CALLBACK_TABLE

/* ── CAN types ── */
typedef void* CAN_HANDLE;
typedef void* CAN_PORT;

/* ── Message macros ── */
#define MSG(...)
#define MSG_ERR(num, str, val)
#define MSG_WAR(num, str, val)

/* ── Thread safety (empty for single-threaded) ── */
/* EnterMutex/LeaveMutex are declared as functions in timers_driver.h.
   We provide empty implementations in timer_rtthread.c instead of macros. */

/* ── Disable dynamic loading ── */
/* DLL_CALL: on embedded, function names map directly.
   Exception: canOpen conflicts with states.c's canOpen(board, CO_Data*),
   so the driver function keeps the _driver suffix. */
#ifdef DLL_CALL
#undef DLL_CALL
#endif
#define DLL_CALL(funcname) funcname

/* Special: check canOpen conflict — driver uses canOpen, states.c also has canOpen */
/* This is handled by having the driver implement canOpen_driver directly,
   while canSend/canReceive/canClose/canChangeBaudRate use the identity mapping. */

/* ── Task handle type ── */
typedef void* TASK_HANDLE;

/* ── Debug printing ── */
extern void rt_kprintf(const char *fmt, ...);
#define eprintf(...)    rt_kprintf(__VA_ARGS__)

#endif /* __APPLICFG_H__ */
