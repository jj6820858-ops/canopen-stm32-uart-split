/*
 * sampling.h — Modbus → CANopen 命令转发模块
 *
 * 上位机通过 Modbus 一步一步下发指令, MCU 负责:
 *   1. 收到寄存器写 → 转发为 CANopen 动作 (RPDO2 / 控制字)
 *   2. 收到寄存器读 → 返回最新 CAN TPDO 反馈值
 *   3. 周期性同步 CAN 反馈 → Modbus 寄存器 (reg_od_sync_in)
 *
 * MCU 本身不驱动流程 — 流程由上位机控制。
 */
#ifndef __SAMPLING_H__
#define __SAMPLING_H__

#include <stdint.h>

/* ── 孔位 CAN 位置定义 ── */
#define SLOT_COUNT        7
#define SLOT_IDLE_POS    (-19)

extern const int32_t slot_positions[SLOT_COUNT];

/* ── 模块接口 ── */
int  sampling_init(void);

/*
 * Modbus 寄存器写入回调。
 * reg_router 在 reg_write() 完成后调用此函数,
 * MCU 据此触发对应的 CANopen 动作。
 *
 * 上位机 write → reg_router.reg_write() → 更新 regs[] → sampling_on_reg_write()
 *                                                                    │
 *                                                     ┌──────────────┘
 *                                                     ▼
 *                                          设置 OD 变量 → PDO 自动发送
 */
void sampling_on_reg_write(uint16_t addr, uint16_t value);

/*
 * 周期性同步: CAN TPDO 反馈 → Modbus 寄存器。
 * 由 CANopen post_TPDO 回调驱动。
 */
void sampling_sync_can_to_modbus(void);

#endif /* __SAMPLING_H__ */
