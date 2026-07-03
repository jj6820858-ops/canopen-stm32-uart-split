# 快检设备 CANopen 网关 — 整体架构 & 电机测试记录

> 日期：2026-07-03 | 状态：X 轴电机控制验证通过

## 1. 核心参考文件

| 文件 | 路径 | 说明 |
|------|------|------|
| Modbus 协议 | `C:\Users\lenovo\Desktop\快检设备交互协议20250609.xlsx` | 上位机 ↔ 主控通信协议，200 个寄存器（00001~00198） |
| Object Dictionary | `C:\Users\lenovo\Desktop\Master.c` | objdictgen 生成，37 个 OD 映射变量，含全部 PDO/ SDO 配置 |
| Object Dictionary | `C:\Users\lenovo\Desktop\Master.h` | OD 变量 extern 声明 |

工程内对应文件：`canfestival/od_master/ObjDict.c` / `ObjDict.h`，内容已与 Desktop 完全同步。

## 2. 系统架构

```
┌─────────┐  Modbus RTU   ┌──────────────────────────────────────┐  CANopen   ┌──────────┐
│  上位机   │ ← UART2 →    │           STM32F103 主控             │ ← CAN1 →   │  从站 MCU │
│ (PC/HMI) │  115200       │                                    │  50kbps    │ Node 0x01 │
└─────────┘               │ ┌──────────┐   ┌────────────────┐  │            └──────────┘
                          │ │FreeModbus│→→→│  reg_router.c   │  │
                          │ │ Slave    │←←←│  regs[200]      │  │
                          │ │user_mb   │   │   ↕ OD sync     │  │
                          │ └──────────┘   └───────┬────────┘  │
                          │                        │            │
                          │ ┌──────────────────────┐│            │
                          │ │   ObjDict.c/h        ││            │
                          │ │  (37 OD mapped vars) │◀            │
                          │ │  mX_*, mY_*, mZ_*,   │             │
                          │ │  mE_*, mT_*, TEMP_*, │             │
                          │ │  photometer_*,       │             │
                          │ │  weight_*, heating_* │             │
                          │ └──────────────────────┘             │
                          │           │                          │
                          │ ┌─────────┴──────────┐               │
                          │ │ CANfestival Master │               │
                          │ │  Node ID = 0x00    │               │
                          │ │  sendPDOevent()    │               │
                          │ │  heartbeat 1s      │               │
                          │ └────────────────────┘               │
                          │           │                          │
                          │  UART1 ───┴── finsh/msh 控制台       │
                          │  motor_pos, motor_vel, motor_stat    │
                          └──────────────────────────────────────┘
```

### 数据流

```
Modbus FC06/FC16 → user_mb_app.c → reg_write() → regs[200] + OD 变量
                                                        │
sendPDOevent() 检测到 OD 变量变化 ─→ 构建 TPDO ─→ CAN 总线 → 从站 RPDO → 电机动作
                                                        │
从站 → CAN 总线 → canDispatch() → OD 变量 → reg_od_sync_in() → regs[] → 上位机可读
```

## 3. 测试环境

| 项目 | 配置 |
|------|------|
| 主控 | STM32F103RCT6, RT-Thread 4.0.3 |
| CAN 分析仪 | USB-CAN, 50kbps |
| 调试串口 | UART1, 115200 |
| 从站 | 电机驱动 (CiA 402), Node ID = 0x01 |

## 4. PDO 配置（来自 Master.c）

### TPDO（主站 → 从站）

| TPDO | COB-ID | Type | Event Timer | 映射内容 |
|------|--------|------|-------------|---------|
| TPDO1 | 0x201 | 0xFF | 0 | mX_modes (0x2001) + mX_control_word (0x2005) |
| **TPDO2** | **0x301** | 0xFF | 21ms | **mX_position (0x2002) + mX_velocity (0x2003)** |
| TPDO3 | 0x202 | 0xFF | 0 | mY_modes + mY_control_word |
| TPDO4 | 0x302 | 0xFF | 36ms | mY_position + mY_velocity |
| TPDO5 | 0x203 | 0xFF | 0 | mZ_modes + mZ_control_word |
| TPDO6 | 0x303 | 0xFF | 33ms | mZ_position + mZ_velocity |
| TPDO7 | 0x204 | 0xFF | 0 | mE_modes + mE_control_word |
| TPDO8 | 0x304 | 0xFF | 27ms | mE_position + mE_velocity |
| TPDO9 | 0x205 | 0xFF | 0 | mT_modes + mT_control_word |
| TPDO10 | 0x305 | 0xFF | 0 | mT_position + mT_velocity |
| TPDO11 | 0x206 | 0xFF | 97ms | photometer_led |
| TPDO12 | 0x207 | 0xFF | 1s | heating_target + refrigeration_target |
| TPDO13 | 0x307 | 0xFF | 1s | TEMP_control_word |
| TPDO14 | 0x306 | 0xFF | 3s | photometer_rate + photometer_gain |

### RPDO（从站 → 主站）

| RPDO | COB-ID | 映射内容 |
|------|--------|---------|
| RPDO1 | 0x181 | mX_status_word + mX_Current_actual |
| RPDO2 | 0x182 | mY_status_word + mY_Current_actual |
| RPDO3 | 0x183 | mZ_status_word + mZ_Current_actual |
| RPDO4 | 0x184 | mE_status_word + mB_Current_actual |
| RPDO5 | 0x185 | mT_status_word + mT_modes |
| RPDO6 | 0x186 | photometer_ch0 + photometer_ch1 |
| RPDO7 | 0x187 | current_heating + current_refrigeration |
| RPDO8 | 0x287 | TEMP_status_word |
| RPDO9 | 0x188 | weight_buff_liq + weight_waste_liq |
| RPDO10 | 0x288 | weight_clean_water |

### SDO 通道

| SDO | TX COB-ID | RX COB-ID | 目标节点 |
|-----|-----------|-----------|---------|
| Client 1 | 0x601 | 0x581 | 0x01 |
| Client 2 | 0x602 | 0x582 | 0x02 |
| Client 3 | 0x603 | 0x583 | 0x03 |
| Client 4 | 0x604 | 0x584 | 0x04 |
| Client 5 | 0x605 | 0x585 | 0x05 |

## 5. Modbus 寄存器完整映射

协议来源：`快检设备交互协议20250609.xlsx`，200 个保持寄存器。

### 功能/状态区 (00001~00033)

| 地址 | OD | 变量 | 访问 | 说明 |
|------|-----|------|------|------|
| 00001~00003 | - | local | R | 孔位状态 (48-bit bitmap) |
| 00004 | 0x2001 | mX_modes | RW | 清洗针头 → X 轴模式 |
| 00005 | 0x2005 | mX_control_word | RW | 取液/注液/清洗 → X 轴控制字 |
| 00006~00007 | 0x2002 | mX_position | RW | 柱塞泵液量 → X 轴位置 (32-bit) |
| 00008 | 0x2003 | mX_velocity | RW | 蠕动泵转速 → X 轴速度 (32-bit) |
| 00009 | - | local | RW | 蠕动泵圈数 |
| 00010 | - | local | RW | 操作位号 (bitmap) |
| 00011 | 0x2015 | mT_modes | RW | 转盘功能使能 |
| 00012~00014 | 0x2016 | mT_position | RW | 转盘指定位号 (48-bit → 32-bit lo) |
| 00015 | 0x201F | refrigeration_target | RW | 酶孔位制冷 |
| 00016~00017 | 0x201A/B | photometer_ch0/1 | R | 光强读数 |
| 00018~00019 | - | local | R | 仪器状态 (32-bit bitmap) |
| 00020~00033 | - | local | R | 异常状态码 |

### 实时数据区 (00100~00104)

| 地址 | OD | 变量 | 访问 | 说明 |
|------|-----|------|------|------|
| 00100 | 0x2020 | current_heating | RW | 转盘加热温度 (×100) |
| 00101 | 0x2021 | current_refrigeration | RW | 酶试剂制冷温度 (×100) |
| 00102 | 0x2025 | weight_clean_water | R | 清水瓶重量 (g) |
| 00103 | 0x2026 | weight_buff_liq | R | 缓冲液瓶重量 (g) |
| 00104 | 0x2027 | weight_waste_liq | R | 废液瓶重量 (g) |

### 参数设置区 (00120~00198)

| 地址 | OD | 说明 |
|------|-----|------|
| 00120 | local | 参数控制字 |
| 00121 | local | 称重控制 |
| 00130~00132 | local | 测试区参数 |
| 00150~00163 | local | 启停/运转速度 |
| 00164~00171 | local | 升降臂下降深度 |
| 00172~00179 | local | 加样臂角度 |
| 00180~00183 | local | 转盘角度 |
| 00184~00189 | local | 清洗参数 |
| 00190~00191 | local | 搅拌混合参数 |
| 00192 | 0x201E | heating_target | 加热目标温度 (×100) |
| 00193 | 0x201F | refrigeration_target | 制冷目标温度 |
| 00194~00196 | local | 震荡参数 |
| 00197~00198 | local | 柱塞泵液量 |

## 6. 关键发现

### 6.1 csPDO 未置位导致 sendPDOevent 失效

**现象**: `sendPDOevent()` 返回 `sent=0`，CAN 分析仪看不到 TPDO

**原因**: `pdo.c:628` 检查 `d->CurrentCommunicationState.csPDO`，该标志位为 0 时直接返回 0

**修复**: `timer_rtthread.c` 中调用 `sendPDOevent()` 前强制 `csPDO = 1`

```c
if (!d->CurrentCommunicationState.csPDO) {
    d->CurrentCommunicationState.csPDO = 1;
}
```

### 6.2 32 位 OD 变量与 Modbus 寄存器高低字节序反了

**现象**: 写 Modbus `00006=FFFF, 00007=FC18`（-1000），电机疯狂正转（实际值为 0xFC18FFFF ≈ 42 亿）

**原因**: `regs_to_od()` 组装 32 位值时搞反了高低字：
```c
// 错误: v32 = regs[addr] | (regs[addr+1] << 16)
// 正确: v32 = (regs[addr] << 16) | regs[addr+1]
```

**修复**: `reg_router.c` 中 `od_to_regs()` 和 `regs_to_od()` 统一为 big-endian（高 16 位在低地址寄存器）

### 6.3 Event Timer + sendPDOevent 双机制

| 机制 | 间隔 | 作用 |
|------|------|------|
| OD Event Timer | 21ms (TPDO2) | 周期性兜底发送 |
| sendPDOevent() | 5ms (timer loop) | 值变化立即触发 |

两者独立运行，互不冲突。

### 6.4 心跳配置

主站心跳通过直接写 OD 变量实现（不经过 writeLocalDict，因为 0x1017 不在扫描表）：

```c
Master_obj1017 = 1000;              // 主站心跳 1s, COB-ID 0x700
Master_highestSubIndex_obj1016 = 1;
Master_obj1016[0] = 0x00010BB8;    // 监控从站 node 1, 3s 超时
```

## 7. 测试命令

### UART1 控制台 (msh)

```
motor_mode 1          # 设置 PP 模式
motor_ctrl 15         # Enable Operation (0x0F)
motor_vel 500         # 设速度
motor_pos 3200        # 设位置 → TPDO2 立即发送
motor_pos 0           # 回零
motor_pos -1000       # 反转
motor_stat            # 读取所有 X 轴状态
reg 6                 # 读 Modbus 寄存器 00006
reg list              # 列出全部寄存器
```

### UART2 Modbus (115200,N,8,1, addr=1)

**FC16 一次性设置全部 (00004~00008):**
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

## 8. X 轴控制寄存器

| Modbus | OD | 变量 | 说明 |
|--------|-----|------|------|
| 00004 | 0x2001 | mX_modes | 运行模式 |
| 00005 | 0x2005 | mX_control_word | 控制字 |
| 00006~00007 | 0x2002 | mX_position | 目标位置 (32-bit) |
| 00008 | 0x2003 | mX_velocity | 速度 (32-bit, lo 16) |

## 9. 已知问题

1. csPDO 强制置 1 是 workaround，根因待查——CANfestival 进入 Operational 时应自动置位
2. 从站在主控运行时无心跳回应（`Heartbeat lost`）——从站可能不支持 heartbeat 生产者
3. 无 CAN 从站连接时 `heartbeatError` 每秒触发一次
