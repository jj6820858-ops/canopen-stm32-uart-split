# 2026-07-03 CANopen 电机控制测试记录

## 测试环境

| 项目 | 配置 |
|------|------|
| 主控 | STM32F103RCT6, RT-Thread 4.0.3 |
| CAN 分析仪 | USB-CAN, 50kbps |
| 调试串口 | UART1, 115200 |
| 从站 | 电机驱动 (CiA 402), Node ID = 0x01 |

## 关键发现

### 1. csPDO 未置位导致 sendPDOevent 失效

**现象**: `sendPDOevent()` 返回 `sent=0`，CAN 分析仪看不到 TPDO

**原因**: `pdo.c:628` 检查 `d->CurrentCommunicationState.csPDO`，该标志位为 0 时直接返回 0

**修复**: `timer_rtthread.c` 中调用 `sendPDOevent()` 前强制 `csPDO = 1`

```c
if (!d->CurrentCommunicationState.csPDO) {
    d->CurrentCommunicationState.csPDO = 1;
}
```

### 2. 32 位 OD 变量与 Modbus 寄存器高低字节序反了

**现象**: 写 Modbus `00006=FFFF, 00007=FC18`（-1000），电机疯狂正转

**原因**: `regs_to_od()` 组装 32 位值时搞反了高低字：
```c
// 错误: v32 = regs[addr] | (regs[addr+1] << 16)
// 正确: v32 = (regs[addr] << 16) | regs[addr+1]
```

**修复**: `reg_router.c` 中 `od_to_regs()` 和 `regs_to_od()` 统一为 big-endian（高16位在低地址寄存器）

### 3. Event Timer + sendPDOevent 双机制

| 机制 | 间隔 | 作用 |
|------|------|------|
| OD Event Timer | 21ms (TPDO2) | 周期性兜底发送 |
| sendPDOevent() | 5ms (timer loop) | 值变化立即触发 |

两者独立运行，互不冲突。Event Timer 在 `TestMaster_initialisation` 中保持 OD 原值，不运行时清零。

### 4. 心跳配置

主站心跳通过直接写 OD 变量实现（不经过 writeLocalDict，因为 0x1017 不在扫描表）：

```c
Master_obj1017 = 1000;              // 主站心跳 1s, COB-ID 0x700
Master_highestSubIndex_obj1016 = 1;
Master_obj1016[0] = 0x00010BB8;    // 监控从站 node 1, 3s 超时
```

## 测试命令

### UART1 控制台 (msh)

```
motor_mode 1          # 设置 PP 模式
motor_ctrl 15         # Enable Operation (0x0F)
motor_vel 500         # 设速度
motor_pos 3200        # 设位置 → TPDO2 立即发送
motor_pos 0           # 回零
motor_pos -1000       # 反转
motor_stat            # 读取所有 X 轴状态
```

### UART2 Modbus (115200,N,8,1, addr=1)

**FC16 一次性设置全部:**
```
01 10 00 03 00 05 0A 00 01 00 0F 00 00 0C 80 01 F4 F2 C4
```

**FC16 只改位置 (00006~00007):**

| 位置 | Hex |
|------|-----|
| 0 | `01 10 00 05 00 02 04 00 00 00 00 33 90` |
| 3200 | `01 10 00 05 00 02 04 00 00 0C 80 37 30` |
| 10000 | `01 10 00 05 00 02 04 00 00 27 10 29 AC` |
| -1000 | `01 10 00 05 00 02 04 FF FF FC 18 72 BE` |

### CAN 分析仪手动帧

| COB-ID | 数据 | 说明 |
|--------|------|------|
| 0x000 | `01 01` | NMT Start node 1 |
| 0x201 | `01 00 00` | modes=PP, ctrl=0 |
| 0x201 | `01 0F 00` | modes=PP, ctrl=EnableOp |
| 0x301 | `80 0C 00 00 F4 01 00 00` | pos=3200 LE, vel=500 LE |

**TPDO2 数据格式 (0x301, 8 bytes LE):**
```
[0:3] mX_position (INTEGER32, little-endian)
[4:7] mX_velocity (INTEGER32, little-endian)
```

## 寄存器映射 (X 轴控制)

| Modbus | OD | 变量 | 说明 |
|--------|-----|------|------|
| 00004 | 0x2001 | mX_modes | 运行模式 |
| 00005 | 0x2005 | mX_control_word | 控制字 |
| 00006~00007 | 0x2002 | mX_position | 目标位置 (32-bit) |
| 00008 | 0x2003 | mX_velocity | 速度 (32-bit, lo 16) |

## 已知问题

1. csPDO 强制置 1 是 workaround，根因待查——CANfestival 进入 Operational 时应自动置位
2. 从站在主控运行时无心跳回应（`Heartbeat lost`）——从站可能不支持 heartbeat 生产者
3. 无 CAN 从站连接时 `heartbeatError` 每秒触发一次
