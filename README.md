# CANopen 主控协议转换网关

Modbus RTU ↔ CANopen 协议网关，上位机通过 Modbus RTU 读写 STM32 本地 OD，CANopen PDO 在后台同步主站与从站数据。

```
上位机 ──Modbus RTU──▶ STM32(CANopen Master) ──CANopen──▶ 从站设备
                         │
                         └─ 本地 OD (Master.od, 43个对象, 0x2001-0x202B)
                            Modbus 读写直接走本地 OD (getODentry/setODentry)
                            PDO 后台自动同步主站OD ↔ 从站
```

## 项目背景

快检设备协议转换网关。上位机通过 Modbus RTU（快检设备交互协议）控制 STM32，STM32 作为 CANopen 主站管理 CAN 总线上的从站设备（电机驱动、传感器等）。

**v2.0 架构变更**：Modbus 不再通过 SDO 远程访问从站，改为直接读写主站本地 OD，速度更快、不依赖从站在线。

## 硬件需求

| 组件 | 型号/规格 |
|------|----------|
| 主控 MCU | **STM32F103RCT6** (256KB Flash, 48KB SRAM) |
| CAN 收发器 | TJA1050 / SN65HVD230 |
| CAN 总线 | **250 Kbps** (Prescaler=9, BS1=11TQ, BS2=4TQ, SJW=2TQ) |
| 调试串口 | UART1 (PA9 TX, PA10 RX), 115200 8N1 |
| 协议串口 | UART2 (PA2 TX, PA3 RX), Modbus RTU 115200 8N1 |

### 接线

```
CAN:     PA12(CAN1_TX) / PA11(CAN1_RX) → TJA1050 → CAN_H/CAN_L
RS:      PA15 → 低电平 (CAN transceiver normal mode)
UART1:   PA9(TX) PA10(RX) → USB-TTL → PC (控制台)
UART2:   PA2(TX) PA3(RX) → USB-TTL → 上位机 (Modbus RTU)
```

## 软件架构

```
applications/
├── main.c                   初始化入口
├── reg_router.c/.h          Modbus ↔ CANopen 路由表 (43条, 本地OD)
├── canopen_master.c/.h      CANopen 主站 (初始化/回调/PDO配置)
└── reg_router.h             路由条目结构定义

canfestival/
├── od_master/ObjDict.c/.h   主站对象字典 (Master.od, 109个索引)
├── port/can_stm32.c         STM32 HAL CAN 驱动 (250Kbps)
├── port/timer_rtthread.c    RT-Thread Timer 适配
├── src/                     CANopen 协议栈 (NMT/SDO/PDO/SYNC...)
└── include/                 协议头文件

packages/freemodbus/
└── port/user_mb_app.c       Modbus 从站回调 → reg_router
```

### 数据流

```
Modbus READ:
  上位机 → UART2 → FreeModbus → reg_read()
    → getODentry(本地OD) → 即时返回 (无SDO延迟)

Modbus WRITE:
  上位机 → UART2 → FreeModbus → usSRegHoldBuf[] (本地缓存)
    → reg_write_async() → setODentry(本地OD) → 即时写入

CANopen PDO (后台同步):
  主站OD ←→ 从站设备 (由CANfestival PDO引擎自动处理)
```

## CANopen 对象字典

### Master.od 用户对象 (0x2001-0x202B, 43个)

| 轴 | modes | position | velocity | status_word | control_word | current |
|----|-------|----------|----------|-------------|--------------|---------|
| **mX** (针头) | 0x2001 | 0x2002 | 0x2003 | 0x2004 | 0x2005 | 0x2028 |
| **mY** (旋转臂) | 0x2006 | 0x2007 | 0x2008 | 0x2009 | 0x200A | 0x2029 |
| **mZ** (转盘) | 0x200B | 0x200C | 0x200D | 0x200E | 0x200F | 0x202A |
| **mE** (蠕动泵) | 0x2010 | 0x2011 | 0x2012 | 0x2013 | 0x2014 | — |
| **mT** (柱塞泵) | 0x2015 | 0x2016 | 0x2017 | 0x2018 | 0x2019 | — |
| **mB** | — | — | — | — | — | 0x202B |

| 功能 | OD Index | Name | Type |
|------|----------|------|------|
| 光度计 | 0x201A | photometer_ch0 | UINT16 |
| 光度计 | 0x201B | photometer_ch1 | UINT16 |
| 光度计 | 0x201C | photometer_led | UINT32 |
| 光度计 | 0x2022 | photometer_rate | UINT16 |
| 光度计 | 0x2023 | photometer_gain | UINT8 |
| 温控 | 0x201D | TEMP_control_word | UINT32 |
| 温控 | 0x201E | heating_target | UINT32 |
| 温控 | 0x201F | refrigeration_target | UINT32 |
| 温控 | 0x2020 | current_heating | UINT32 |
| 温控 | 0x2021 | current_refrigeration | UINT32 |
| 温控 | 0x2024 | TEMP_status_word | UINT32 |
| 称重 | 0x2025 | weight_clean_water | INT32 |
| 称重 | 0x2026 | weight_buff_liq | INT32 |
| 称重 | 0x2027 | weight_waste_liq | INT32 |

### PDO 映射 (14 TPDO + 10 RPDO)

TPDO (主站→从站命令): 每轴 modes+control_word / position+velocity / photometer / temp 各一组
RPDO (从站→主站反馈): 每轴 status_word+current_actual / 传感器数据 各一组

详见 `canfestival/od_master/ObjDict.c` 和 `C:\Users\lenovo\Desktop\Master.od`

## Modbus 寄存器映射

协议文档: `C:\Users\lenovo\Desktop\快检设备交互协议20250609.xlsx`

所有 43 个 Master.od 对象均有 Modbus 映射，详见 `applications/reg_router.c` 中 `g_routes[]` 表。

| 区域 | Modbus 地址 | 内容 |
|------|------------|------|
| 控制/状态 | 1-33 | 针头、转盘、蠕动泵、传感器状态 |
| 实时数据 | 100-104 | 温度、重量 |
| 参数设置 | 120-198 | 速度、深度、角度、温度目标 |
| 扩展区 | 199-213 | LED、电流反馈等 |

未在路由表中的寄存器（孔位状态、故障位、参数等）使用 Modbus 本地缓冲区 `usSRegHoldBuf[]`。

### Modbus 帧示例 (从站地址=1)

```
# FC03 读寄存器 100 (加热温度)
发送: 01 03 00 63 00 01 74 14
响应: 01 03 02 [2字节数据] [CRC]

# FC06 写寄存器 4 (清洗针头) = 0x0001
发送: 01 06 00 03 00 01 39 E9
响应: 01 06 00 03 00 01 39 E9
```

## 快速开始

### 烧录

通过 RT-Thread Studio 或 ST-Link 烧录 `Debug/rtthread.bin`

### 串口测试

UART1 (控制台) 115200 8N1:
```
CANopen Master + Modbus Slave GW
STM32F103RCT6, RT-Thread v4.0.3
CAN 250Kbps, Modbus addr=1
```

MSH 命令:
```
msh> mb_read 8          # 读寄存器 00008
msh> mb_write 4 123     # 写 00004 = 123
msh> mb_dump            # 打印全部寄存器
msh> can_test_start 200 # CAN 测试发送 (200ms间隔)
```

## CAN 配置

| 参数 | 值 |
|------|-----|
| 波特率 | **250 Kbps** |
| APB1 时钟 | 36 MHz |
| Prescaler (BRP) | 9 |
| TimeSeg1 (BS1) | 11 TQ |
| TimeSeg2 (BS2) | 4 TQ |
| SyncJumpWidth (SJW) | 2 TQ |
| 采样点 | 75% (12/16 TQ) |
| Mode | Normal |
| AutoRetransmission | DISABLE |
| Filter | 32-bit ID Mask, accept all |

## 版本历史

| 版本 | 标签 | 说明 |
|------|------|------|
| **v2.0** | `v2.0` | 本地OD架构: Master.od 43对象完整映射, Modbus→本地OD, 250Kbps CAN, CAN SJW=2TQ |
| v1.1 | — | SDO异步链式写入, 从站配置, FreeModbus集成 |
| v1.0 | — | 初始版本: CANopen主站, 模拟从站SDO路由 |

## 相关文档

| 文档 | 说明 |
|------|------|
| `C:\Users\lenovo\Desktop\Master.od` | CANopen 主站对象字典 (CanFestival objdictedit) |
| `C:\Users\lenovo\Desktop\快检设备交互协议20250609.xlsx` | Modbus 寄存器协议定义 |
| `docs/2026-06-10-canopen-stm32-design.md` | 架构设计文档 |
| `docs/2026-06-10-canopen-stm32-impl.md` | 实现计划 |
| `docs/农药残留智能检测仪使用说明书.doc` | 仪器使用说明书（桂林品创科技） |
| `docs/农药残留智能检测仪使用说明书.txt` | 说明书文本提取 |
| `docs/快检设备交互协议20250609.xlsx` | Modbus 寄存器协议定义（80+寄存器） |
| `canfestival/od_master/Master.c` | CANopen 主站对象字典源码 |
| `canfestival/od_master/Master.h` | CANopen 主站对象字典头文件 |
