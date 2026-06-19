# CANopen 主控协议转换网关

将串口自定义协议转为 CANopen，让上位机通过简单串口帧即可控制 CANopen 从站设备。

```
上位机 ──串口(AA55帧)──▶ STM32(CANopen Master) ──CANopen──▶ 从站MCU(CANfestival Slave)
```

## 项目背景

原有架构是「上位机 ↔ 串口 ↔ 从机 MCU」，上位机通过自定义寄存器协议直接与下位机通信。现在需要引入 CANopen 总线，但上位机软件不方便改协议栈。因此做了这个网关：

**串口进去，CANopen 出来**——上位机完全不知道 CANopen 的存在，照常发它的寄存器读写帧，网关负责查路由表、发 SDO/PDO、收应答、转成串口响应送回上位机。

## 硬件需求

| 组件 | 型号/规格 |
|------|----------|
| 主控 MCU | **STM32F103RCT6** (256KB Flash, 48KB SRAM) |
| 开发板 | 任意带 CAN + UART 的 STM32F103RC 板 |
| 调试串口 (控制台) | USB-TTL CH340/CP2102，接 PA9(TX) PA10(RX) |
| 协议串口 (AA55) | USB-TTL CH340/CP2102，接 PA2(TX) PA3(RX) |
| CAN 收发器 | TJA1050/SN65HVD230，接 PA12(CAN1 TX) PA13(CAN1 RX) |
| 波特率 | 串口 115200 / CAN 125Kbps |

### 接线

```
## 接线

### 调试串口 (UART1 — 控制台 / FINSH / rt_kprintf)
```
PC(串口助手)          STM32
    TX ──────────── PA10(RX)
    RX ──────────── PA9(TX)
    GND ──────────  GND
```

### 协议串口 (UART2 — AA55 帧协议 / CANopen 命令)
```
PC(上位机)            STM32
    TX ──────────── PA3(RX)
    RX ──────────── PA2(TX)
    GND ──────────  GND
```

### CAN 总线
```
STM32 CAN1           CAN 总线
    PA12(TX) ──┬── TJA1050(1) ── CAN_H
    PA13(RX) ──┘             └── CAN_L
```
```

## 软件架构

### 模块概览

```
applications/
├── main.c                  初始化入口，启动三个模块
├── serial_protocol.c/.h    AA55 帧状态机解析（收/发/校验）
├── reg_router.c/.h         协议地址 → CANopen OD 映射表（路由表）
└── canopen_master.c/.h     CANfestival 主站封装（初始化/回调/SDO链式配置）

canfestival/                CANfestival 协议栈（移植版）
├── src/                    协议核心（NMT/SDO/PDO/SYNC/心跳…）
├── include/                协议头文件
├── port/
│   ├── can_stm32.c/.h      STM32 HAL CAN 驱动适配
│   ├── timer_rtthread.c/.h RT-Thread Timer 适配
│   └── applicfg.h          平台类型定义
└── od_master/              主站对象字典（objdictgen 生成）
```

### 数据流

```
串口 RX 收到 AA55 帧
      │
      ▼
serial_protocol 解析帧 → 校验 XOR → 识别命令
      │
      ▼
reg_router 查路由表：协议地址 → (NodeID=0x02, OD Index/SubIndex)
      │
      ▼
canopen_master 执行 SDO 上传/下载，或通过 PDO 收发
      │
      ▼
CAN 总线 → 从站 (Node 0x02) 响应 → 串口回帧给上位机
```

## 串口协议定义

### 帧格式

```
起始符  命令   地址(LE)  长度   数据     校验
┌────┐ ┌──┐ ┌──────┐ ┌──┐ ┌────┐ ┌──────┐
AA 55 | CMD | AddrL AddrH | Len | Data… | XOR |
```

| 字段 | 长度 | 说明 |
|------|------|------|
| `AA 55` | 2B | 帧起始 |
| CMD | 1B | `0x03`=读取，`0x06`=写单个，`0x10`=写多个 |
| Addr | 2B | 协议寄存器地址，小端序 |
| Len | 1B | 读=寄存器个数；写=数据字节数 |
| Data | NB | 写操作的数据载荷 |
| XOR | 1B | 前面所有字节的异或校验和 |

### 错误响应

```
AA 55 FF | 错误码 | ADDR_L | ADDR_H | 00 | XOR
```

| 错误码 | 含义 |
|--------|------|
| `0x01` | 校验和错误 |
| `0x02` | 未知命令 |
| `0x03` | 地址无路由（或 SDO 超时） |
| `0x04` | 访问权限错误 |

## 寄存器映射

当前所有寄存器映射到从站 Node 0x02 的对象字典：

| 协议地址 | OD Index | 类型 | 访问 | 说明 |
|---------|----------|------|------|------|
| `0x0001~0x0003` | `0x2000` | UNS64 | RO | 孔位状态 |
| `0x0004` | `0x2001` | UNS8 | RW | 清洗针头 |
| `0x0005` | `0x2002` | UNS8 | RW | 取液/注液/清洗 |
| `0x0006~0x0007` | `0x2003` | UNS32 | RW | 柱塞泵液量 |
| `0x0008` | `0x2004` | UNS16 | RW | 蠕动泵转速 |
| `0x000A` | `0x2006` | UNS16 | RW | 操作位号 |
| `0x0010~0x0011` | `0x200A` | UNS32 | RO | 光强读数 |
| `0x0064` | `0x2100` | UNS16 | RW | 转盘加热温度(×100) |
| `0x0065` | `0x2101` | UNS16 | RW | 制冷温度(×100) |
| `0x0078~0x0198` | `0x2200~0x224E` | 详见文档 | RW | 参数设置区 |

## 快速开始

### 1. 硬件连接

STM32 上电前接好 USB-TTL 和 CAN 收发器。

### 2. 烧录固件

通过 RT-Thread Studio 导入工程编译烧录，或：

```bash
scons --target=makefile -s
make -j
# 用 ST-Link 烧录 Debug/rtthread.bin
```

### 3. 串口测试

打开串口助手（115200 8N1，**HEX 模式发送/接收**）：

```
# 读蠕动泵转速
发送: AA 55 03 08 00 01 FE
响应: AA 55 03 08 00 02 [2字节数据] [XOR]

# 写转速=500 (0x01F4)
发送: AA 55 06 08 00 02 F4 01 52
响应: AA 55 06 08 00 00 FE
```

上电后串口助手会先收到系统日志：
```
====================================
 CANopen Master Protocol Gateway
 STM32F103RCT6, RT-Thread v4.0.3
====================================
```

**注意**：串口助手不要追加回车换行，否则 XOR 校验失败。

## 构建状态

已编译固件占用：
- `.text` = 73,364 字节
- `.data` = 2,960 字节
- `.bss` = 3,964 字节
- 总 Flash ≈ **80KB**（256KB 余量充足）

## 相关文档

| 文档 | 说明 |
|------|------|
| [设计文档](docs/2026-06-10-canopen-stm32-design.md) | 架构设计、方案选型、OD 定义 |
| [实现计划](docs/2026-06-10-canopen-stm32-impl.md) | 实施步骤、代码说明 |
| [串口测试](docs/2026-06-11-serial-test-plan.md) | 详细测试用例，含 XOR 速查表 |

## 路线图

- [x] CANfestival 移植到 RT-Thread + STM32F1
- [x] 串口 AA55 帧协议解析
- [x] 寄存器路由表（30+ 条映射）
- [x] SDO 链式从站配置
- [x] PDO 策略（SYNC 触发 TPDO / 异步 RPDO）
- [ ] 多从站支持（路由表扩展）
- [ ] 串口日志级别可配置
- [ ] 看门狗 + 掉线重连
