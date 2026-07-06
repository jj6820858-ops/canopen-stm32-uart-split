/*
 * reg_router.h - 快检设备 Modbus 寄存器路由
 *
 * 共 200 个保持寄存器:
 *   00001~00033  功能/状态区
 *   00100~00104  实时数据区
 *   00120~00198  参数设置区
 *
 * Modbus 帧地址按 0 起始计算: 帧地址 = 协议寄存器号 - 1。
 */
#ifndef __REG_ROUTER_H__
#define __REG_ROUTER_H__

#include <stdint.h>

#define REGS_COUNT  200   /* 保持寄存器 0..199 */

/* 模块初始化与读写接口 */
void reg_router_init(void);
int  reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf);
int  reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data);
int  reg_write_local(uint16_t start_addr, uint8_t count, const uint8_t *data);
int  reg_write_async(uint16_t start_addr, uint8_t count,
                     const uint8_t *data, void (*done)(int result));

/* 对象字典同步接口，由 CANopen 回调或写寄存器路径调用 */
void reg_od_sync_out(void);   /* regs[] -> 对象字典变量 */
void reg_od_sync_in(void);    /* 对象字典变量 -> regs[] */

#endif /* __REG_ROUTER_H__ */
