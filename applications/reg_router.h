/*
 * reg_router.h — Modbus Register Map per 快检设备交互协议20250609
 *
 * 200 holding registers covering:
 *   00001~00033  功能/状态区 (Function/Status)
 *   00100~00104  实时数据区 (Real-time Data)
 *   00120~00198  参数设置区 (Parameter Settings)
 *
 * Modbus frame address = register_number - 1 (0-based).
 */
#ifndef __REG_ROUTER_H__
#define __REG_ROUTER_H__

#include <stdint.h>

#define REGS_COUNT  200   /* holding registers 0..199 */

/* ── Init & lookup ── */
void reg_router_init(void);
int  reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf);
int  reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data);
int  reg_write_async(uint16_t start_addr, uint8_t count,
                     const uint8_t *data, void (*done)(int result));

/* ── OD sync (called from CANopen callbacks) ── */
void reg_od_sync_out(void);   /* regs[] → OD variables (Modbus write path) */
void reg_od_sync_in(void);    /* OD variables → regs[] (RPDO receive path) */

#endif /* __REG_ROUTER_H__ */
