# CANopen 主控协议转换设计

> 日期：2026-06-10 | 状态：设计完成，待评审

## 1. 概述

### 1.1 背景

快检设备当前架构为「上位机 ←→ 串口 ←→ 下位机」，上位机通过自定义寄存器协议下发任务。现需升级为：

```
上位机 ──串口(自定义帧)──▶ 主控STM32(CANfestival Master) ──CANopen──▶ 下位机MCU(CANfestival Slave)
```

- **主控**：`canopen_stm32` 工程，RT-Thread v4.0.3 + STM32F103RCT6
- **从站**：自研 MCU 跑 CANfestival Slave，先做 1 个验证，后续扩展
- **协议**：参考 `快检设备交互协议20250609.xlsx`，80+ 个寄存器
- **CANopen 库**：CANfestival（`D:\workspace\app\canfestival`）

### 1.2 硬件配置

| 项目 | 配置 |
|------|------|
| MCU | STM32F103RCT6（256KB Flash, 48KB SRAM） |
| 串口 | USART1 — PA9(TX), PA10(RX) |
| CAN | CAN1 — PA12(TX), PA13(RX) |

### 1.3 方案选择

选**方案 B（路由表方案）**：

- 主控不镜像业务数据，只维护「协议地址 → (节点ID, OD Index)」路由表
- 上位机串口读写 → 路由表查表 → SDO/PDO 直达从站
- 主控 OL 精简，扩展新从站只需修改路由表 + 新增从站 OD

## 2. 架构 & 模块

```
canopen_stm32/
├── applications/
│   ├── main.c                    # 初始化 + 启动线程
│   ├── serial_protocol.c/.h      # 串口帧解析
│   ├── reg_router.c/.h           # 寄存器路由表
│   └── canopen_master.c/.h       # CANfestival 主站封装（回调、初始化）
├── canfestival/                   # CANfestival 移植
│   ├── src/                      # 核心协议栈（复制自 D:\workspace\app\canfestival\src）
│   ├── include/                  # 协议栈头文件（复制自 D:\workspace\app\canfestival\include）
│   ├── port/                     # RT-Thread + STM32 适配层
│   │   ├── applicfg.h           # 平台配置
│   │   ├── canfestival.h        # include 封装
│   │   ├── timerscfg.h          # 定时器配置
│   │   ├── can_stm32.c/.h       # CAN 驱动（HAL CAN）
│   │   └── timer_rtthread.c/.h  # 定时器（RT-Thread Timer）
│   ├── od_master/                # 主站对象字典（objdictgen 生成）
│   │   └── ObjDict.c/.h/.od
│   └── SConscript                # 构建集成
└── drivers/                       # 需启用 CAN 驱动 (drv_can.c)
```

### 数据流

```
串口 RX → serial_protocol（解帧）
              │
              ▼
         reg_router（查路由表）
              │
              ▼
      canopen_master（SDO/PDO）
              │
              ▼
         CAN 总线 → 从站
```

## 3. 串口帧协议

### 3.1 帧格式

```
| 帧头 2B  | 命令 1B | 地址 2B(LE) | 长度 1B | 数据 NB | 校验 1B(XOR) |
| 0xAA 0x55|  CMD   |    Addr     |    N    | D[0..N] |     XOR      |
```

校验 = 帧头、命令、地址、长度、数据所有字节的 XOR。

### 3.2 命令字

| 命令 | 含义 | 说明 |
|------|------|------|
| `0x03` | 读寄存器 | 批量读，响应中「长度」为数据字节数 |
| `0x06` | 写单个 | 固定 2 字节数据 |
| `0x10` | 写多个 | 连续写多个寄存器 |

### 3.3 地址映射

| 协议地址范围 | 功能分类 | 从站 OD Index |
|-------------|---------|--------------|
| `00001~00033` | 孔位/针/转盘/状态 | `0x2000~0x201F` |
| `0100~0104` | 实时数据 | `0x2100~0x2104` |
| `0120~0198` | 参数设置 | `0x2200~0x224E` |

## 4. CANopen 对象字典

### 4.1 主站 OD（STM32，Node ID = 0x01）

| Index | 内容 | 说明 |
|-------|------|------|
| `0x1000` | 设备类型 | `0x00000000`（纯主站） |
| `0x1001` | 错误寄存器 | 标准 |
| `0x1005` | SYNC COB-ID | `0x00000080` |
| `0x1006` | SYNC 周期 | 按需（如 50ms） |
| `0x1016` | 消费者心跳时间 | 监控从站 |
| `0x1400` | RPDO1 通信参数 | 绑定从站 TPDO1 |
| `0x1600` | RPDO1 映射 | 接收从站实时状态 |
| `0x1800` | TPDO1 通信参数 | 绑定从站 RPDO1 |
| `0x1A00` | TPDO1 映射 | 发送控制指令 |

### 4.2 从站 OD（下位机，Node ID = 0x02）

#### 控制 & 状态区 `0x2000~0x201F`

| OD Index | 协议地址 | 名称 | 类型 | 访问 | PDO |
|----------|---------|------|------|------|-----|
| `0x2000` | `00001~00003` | 孔位状态 | UNS64 | R | ✅ |
| `0x2001` | `00004` | 针头清洗 | UNS8 | RW | |
| `0x2002` | `00005` | 取液/注液/清洗 | UNS8 | RW | |
| `0x2003` | `00006~00007` | 柱塞泵液量 | UNS32 | RW | |
| `0x2004` | `00008` | 蠕动泵转速 | UNS16 | RW | |
| `0x2005` | `00009` | 蠕动泵圈数 | UNS16 | RW | |
| `0x2006` | `00010` | 操作位号 | UNS16 | RW | |
| `0x2007` | `00011` | 转盘功能使能 | UNS8 | RW | |
| `0x2008` | `00012~00014` | 转盘指定位号 | UNS64 | RW | |
| `0x2009` | `00015` | 酶孔位制冷 | UNS8 | RW | |
| `0x200A` | `00016~00017` | 光强读数 | UNS32 | R | ✅ |
| `0x200B` | `00018~00019` | 仪器设备状态 | UNS32 | R | ✅ |
| `0x200C`~`0x201F` | `00020~00033` | 异常状态码 | UNS8/16 | R | |

#### 实时数据区 `0x2100~0x2104`

| OD Index | 协议地址 | 名称 | 类型 | PDO |
|----------|---------|------|------|-----|
| `0x2100` | `0100` | 转盘加热温度(×100) | INTEGER16 | ✅ |
| `0x2101` | `0101` | 酶试剂制冷温度(×100) | INTEGER16 | ✅ |
| `0x2102` | `0102` | 清水瓶重量 | UNS16 | ✅ |
| `0x2103` | `0103` | 缓冲液瓶重量 | UNS16 | ✅ |
| `0x2104` | `0104` | 废液瓶重量 | UNS16 | ✅ |

#### 参数设置区 `0x2200~0x224E`

| OD Index | 协议地址 | 名称 | 类型 |
|----------|---------|------|------|
| `0x2200` | `0120` | 参数控制字 | UNS16 |
| `0x2201` | `0121` | 称重控制 | UNS8 |
| `0x2202`~`0x2204` | `0130~0132` | 测试区参数 | UNS16×3 |
| `0x2205`~`0x2212` | `0150~0163` | 启停/运转速度 | UNS16×14 |
| `0x2213`~`0x221A` | `0164~0171` | 升降臂深度 | UNS16×8 |
| `0x221B`~`0x2222` | `0172~0179` | 加样臂角度 | UNS16×8 |
| `0x2223`~`0x2226` | `0180~0183` | 转盘角度 | UNS16×4 |
| `0x2227`~`0x222C` | `0184~0189` | 清洗参数 | UNS16×6 |
| `0x222D`~`0x223E` | `0190~0198` | 混合/温度/震荡/泵 | UNS16×11 |

### 4.3 PDO 策略

- **TPDO1（从站→主站，SYNC 触发）**：实时数据 `0x2100~0x2104` + 仪器状态 `0x200B`，周期 100ms
- **RPDO1（主站→从站，异步触发）**：控制命令 `0x2001~0x2002`
- **其余全部走 SDO**

## 5. 路由表

### 5.1 数据结构

```c
typedef struct {
    uint16_t addr_start;   /* 协议起始地址 */
    uint16_t addr_end;     /* 协议结束地址 */
    uint8_t  node_id;      /* 目标 CANopen 节点 ID */
    uint16_t od_index;     /* CANopen OD Index */
    uint8_t  od_subindex;  /* CANopen OD SubIndex */
    uint8_t  access;       /* REG_RO / REG_RW / REG_WO */
    uint8_t  data_type;    /* UNS8=0, UNS16=1, UNS32=2, UNS64=3 */
} reg_route_entry_t;
```

### 5.2 路由表（初版，全部指向 Node 0x02）

```
协议地址范围      → 节点  OD Index  子索引  访问  类型
00001 ~ 00003     0x02  0x2000    0x00    R     UNS64
00004             0x02  0x2001    0x00    RW    UNS8
00005             0x02  0x2002    0x00    RW    UNS8
00006 ~ 00007     0x02  0x2003    0x00    RW    UNS32
...
0120              0x02  0x2200    0x00    RW    UNS16
...
```

### 5.3 查表 & 分发

```c
/* 查表 */
const reg_route_entry_t *reg_lookup(uint16_t addr);

/* 读寄存器：上位机 0x03 → SDO upload → 回应 */
int reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf);

/* 写寄存器：上位机 0x06/0x10 → SDO download → 回应 */
int reg_write(uint16_t start_addr, uint8_t count, uint8_t *data);
```

## 6. CANfestival 移植要点

### 6.1 CAN 驱动（`port/can_stm32.c`）

实现 CANfestival 驱动接口（`can_driver.h`）：
- `canReceive_driver()` — 从 HAL CAN FIFO 取帧
- `canSend_driver()` — 通过 HAL CAN 发送帧
- `canOpen_driver()` — 初始化 CAN1 硬件
- `canClose_driver()` — 关闭 CAN

### 6.2 定时器（`port/timer_rtthread.c`）

- `getElapsedTime()` → `rt_tick_get()`（1ms 精度）
- `setTimer()` → 基于 `rt_timer` 实现

### 6.3 平台配置（`port/applicfg.h`）

```c
#define UNS8   unsigned char
#define UNS16  unsigned short
#define UNS32  unsigned long
#define UNS64  unsigned long long
#define NOT_USE_DYNAMIC_LOADING
#define CO_MASTER_CALLBACK_TABLE
#define MAX_CAN_BUS_ID  1
```

### 6.4 主站回调

| 回调 | 职责 |
|------|------|
| `initialisation` | 配置 RPDO1/TPDO1 COB-ID 绑定从站 |
| `preOperational` | SDO 链式配置从站 TPDO/RPDO/心跳 |
| `post_sync` | 从 RPDO 映射变量读取从站实时状态 |
| `heartbeatError` | 上报节点掉线 |
| `post_SlaveBootup` | 从站上线 → 发 NMT Start |

## 7. 串口协议线程

```
serial_protocol_thread:
  loop:
    阻塞读帧（帧头同步 0xAA 0x55）
    XOR 校验
    解析 命令/地址/长度/数据
    switch(cmd):
      0x03 → reg_read() → 组帧回应
      0x06/0x10 → reg_write() → 组帧回应
      其他 → 错误响应
```

## 8. 验证计划

### Step 1：硬件层（无 CANopen）
- USART1 自发自收或与 PC 串口助手通信
- CAN1 发裸帧，逻辑分析仪或另一 CAN 节点接收

### Step 2：主站独立验证
- PC USB-CAN 工具模拟从站
- 验证 NMT、SDO 读写、PDO 收发

### Step 3：全链路

| 测试项 | 协议地址 | 操作 | 验证 |
|--------|---------|------|------|
| 读仪器状态 | 00018~00019 | 串口 0x03 | 检查位状态 |
| 控制针取液 | 00004~00005 | 串口 0x06 写 | 从站 GPIO/LED |
| 读实时温度 | 0100 | 串口 0x03 | 模拟温度值 |

## 9. 扩展路径

后续接入更多从站：
1. 在新从站 MCU 部署对应的 CANfestival Slave + OD
2. 在路由表中修改相关条目的 `node_id`
3. 无需改动 `reg_read/reg_write` 逻辑
