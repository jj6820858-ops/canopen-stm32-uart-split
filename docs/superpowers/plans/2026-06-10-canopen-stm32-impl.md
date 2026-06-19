# CANopen 主控协议转换 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) for multi-file tasks or superpowers:executing-plans for inline execution. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 `canopen_stm32` 工程中集成 CANfestival 主站、串口协议解析、寄存器路由表，实现「上位机串口 → CANopen SDO/PDO → 下位机从站」的全链路协议转换。

**Architecture:** 方案 B（路由表方案）—— 主控不镜像业务数据，仅维护静态路由表将协议地址映射到 CANopen 节点 OD。串口收自定义帧 → 解帧 → 查路由表 → SDO/PDO 读写从站。

**Tech Stack:** RT-Thread v4.0.3, STM32F103RCT6, HAL CAN, CANfestival (CANopen), SCons build

---

## 文件结构

```
canopen_stm32/
├── applications/
│   ├── main.c                    # 修改：初始化 + 启动线程
│   ├── serial_protocol.c         # 新建
│   ├── serial_protocol.h         # 新建
│   ├── reg_router.c              # 新建
│   ├── reg_router.h              # 新建
│   ├── canopen_master.c          # 新建
│   └── canopen_master.h          # 新建
├── canfestival/                   # 新建目录
│   ├── src/                      # 复制 13 个 .c
│   ├── include/                  # 复制 18 个 .h
│   ├── port/
│   │   ├── applicfg.h           # 新建
│   │   ├── canfestival.h        # 新建
│   │   ├── timerscfg.h          # 新建
│   │   ├── can_stm32.c          # 新建
│   │   ├── can_stm32.h          # 新建
│   │   ├── timer_rtthread.c     # 新建
│   │   └── timer_rtthread.h     # 新建
│   ├── od_master/
│   │   ├── ObjDict.c            # objdictgen 生成
│   │   ├── ObjDict.h            # objdictgen 生成
│   │   └── ObjDict.od           # objdictgen 生成
│   └── SConscript                # 新建
├── rtconfig.h                    # 修改：启用 CAN 相关配置
└── drivers/
    └── board.c                   # 可能需修改：CAN 引脚时钟初始化
```

---

### Task 1: 复制 CANfestival 核心源文件到工程

**Files:**
- Create: `canfestival/src/*.c`, `canfestival/include/*.h` (从 D:\workspace\app\canfestival 复制)

- [ ] **Step 1: 创建目标目录**

```powershell
New-Item -ItemType Directory -Force -Path "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src"
New-Item -ItemType Directory -Force -Path "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include"
New-Item -ItemType Directory -Force -Path "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\port"
New-Item -ItemType Directory -Force -Path "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\od_master"
```

- [ ] **Step 2: 复制 src 文件（13 个 C 文件）**

```powershell
Copy-Item "D:\workspace\app\canfestival\src\dcf.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\emcy.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\lifegrd.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\lss.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\nmtMaster.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\nmtSlave.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\objacces.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\pdo.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\sdo.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\states.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\sync.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\timer.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
Copy-Item "D:\workspace\app\canfestival\src\symbols.c" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\"
```

- [ ] **Step 3: 复制 include 文件（18 个头文件）**

```powershell
Copy-Item "D:\workspace\app\canfestival\include\can.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\can_driver.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\data.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\dcf.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\def.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\emcy.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\lifegrd.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\lss.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\nmtMaster.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\nmtSlave.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\objacces.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\objdictdef.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\pdo.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\sdo.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\states.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\sync.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\sysdep.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\timers.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
Copy-Item "D:\workspace\app\canfestival\include\timers_driver.h" -Destination "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\"
```

Run: verify file count:
```powershell
(Get-ChildItem "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\src\*.c").Count
(Get-ChildItem "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\include\*.h").Count
```
Expected: 13 C files, 19 H files

---

### Task 2: 创建平台适配层 `port/applicfg.h`

**Files:**
- Create: `canfestival/port/applicfg.h`

- [ ] **Step 1: 写入 applicfg.h**

```c
/*
 * Platform configuration for RT-Thread + STM32F103
 */

#ifndef __APPLICFG_H__
#define __APPLICFG_H__

#include <rtthread.h>

/* Integer types (8/16/32-bit) */
#define INTEGER8    signed char
#define INTEGER16   signed short
#define INTEGER32   signed long
#define UNS8        unsigned char
#define UNS16       unsigned short
#define UNS32       unsigned long
#define UNS64       unsigned long long

/* CAN bus count */
#define MAX_CAN_BUS_ID  1

/* Disable dynamic loading (embedded platform) */
#define NOT_USE_DYNAMIC_LOADING

/* Enable master callbacks */
#define CO_MASTER_CALLBACK_TABLE

/* RT-Thread mutex for thread-safety */
#define EnterMutex()    { }
#define LeaveMutex()    { }

/* Debug printing */
#include "finsh.h"
#define eprintf(...)    rt_kprintf(__VA_ARGS__)

#endif /* __APPLICFG_H__ */
```

---

### Task 3: 创建平台适配层 `port/timerscfg.h`

**Files:**
- Create: `canfestival/port/timerscfg.h`

- [ ] **Step 1: 写入 timerscfg.h**

```c
#ifndef __TIMERSCFG_H__
#define __TIMERSCFG_H__

/* Timer value type (RT-Thread tick = 1ms) */
#define TIMEVAL             UNS32
#define TIMEVAL_MAX         UNS32_MAX
#define MS_TO_TIMEVAL(ms)   ((ms))
#define US_TO_TIMEVAL(us)   (((us) + 999) / 1000)

#endif /* __TIMERSCFG_H__ */
```

---

### Task 4: 创建平台适配层 `port/canfestival.h`

**Files:**
- Create: `canfestival/port/canfestival.h`

- [ ] **Step 1: 写入 canfestival.h（统一 include 入口）**

```c
#ifndef __CANFESTIVAL_PORT_H__
#define __CANFESTIVAL_PORT_H__

/* 1. Platform config */
#include "applicfg.h"
#include "timerscfg.h"

/* 2. CANfestival core headers */
#include "can.h"
#include "can_driver.h"
#include "data.h"
#include "def.h"
#include "objacces.h"
#include "objdictdef.h"
#include "pdo.h"
#include "sdo.h"
#include "states.h"
#include "sync.h"
#include "timer.h"
#include "timers_driver.h"
#include "nmtMaster.h"
#include "nmtSlave.h"
#include "emcy.h"
#include "lifegrd.h"
#include "lss.h"
#include "dcf.h"

#endif /* __CANFESTIVAL_PORT_H__ */
```

---

### Task 5: 创建 CAN 驱动适配 `port/can_stm32.h` + `port/can_stm32.c`

**Files:**
- Create: `canfestival/port/can_stm32.h`
- Create: `canfestival/port/can_stm32.c`

- [ ] **Step 1: 写入 can_stm32.h**

```c
#ifndef __CAN_STM32_H__
#define __CAN_STM32_H__

#include "applicfg.h"
#include "can_driver.h"

/* CAN hardware handle (1-based index as CANfestival convention) */
#define CAN_PORT_NUM    1

void can_hardware_init(void);
UNS8 canChangeBaudRate_driver(CAN_HANDLE fd, char *baud);

#endif /* __CAN_STM32_H__ */
```

- [ ] **Step 2: 写入 can_stm32.c**

```c
#include <rtthread.h>
#include "can_stm32.h"
#include "stm32f1xx_hal.h"

#define CAN_BAUDRATE_125K  125
#define CAN_BAUDRATE_250K  250
#define CAN_BAUDRATE_500K  500
#define CAN_BAUDRATE_1M    1000

#define CAN_RX_BUF_SIZE    32

static CAN_HandleTypeDef  hcan1;
static Message            rx_buf[CAN_RX_BUF_SIZE];
static volatile uint8_t   rx_head = 0;
static volatile uint8_t   rx_tail = 0;
static volatile uint8_t   rx_count = 0;

/* Convert CANfestival baud string to prescaler */
static void can_set_baudrate(char *baud)
{
    CAN_InitTypeDef *init = &hcan1.Init;
    int rate = 0;

    if (strstr(baud, "1M"))  rate = CAN_BAUDRATE_1M;
    else if (strstr(baud, "500K")) rate = CAN_BAUDRATE_500K;
    else if (strstr(baud, "250K")) rate = CAN_BAUDRATE_250K;
    else rate = CAN_BAUDRATE_125K;

    /* APB1 = 36MHz.  Prescaler: 36MHz / rate / (1+BS1+BS2)  */
    /* For 125K: 36M / 125K / 18 = 16 */
    switch (rate) {
        case CAN_BAUDRATE_1M:
            init->Prescaler = 4;
            init->TimeSeg1 = CAN_BS1_5TQ;
            init->TimeSeg2 = CAN_BS2_3TQ;
            break;
        case CAN_BAUDRATE_500K:
            init->Prescaler = 8;
            init->TimeSeg1 = CAN_BS1_5TQ;
            init->TimeSeg2 = CAN_BS2_3TQ;
            break;
        case CAN_BAUDRATE_250K:
            init->Prescaler = 16;
            init->TimeSeg1 = CAN_BS1_5TQ;
            init->TimeSeg2 = CAN_BS2_3TQ;
            break;
        case CAN_BAUDRATE_125K:
        default:
            init->Prescaler = 16;
            init->TimeSeg1 = CAN_BS1_8TQ;
            init->TimeSeg2 = CAN_BS2_7TQ;
            break;
    }
    init->Mode = CAN_MODE_NORMAL;
    init->SyncJumpWidth = CAN_SJW_1TQ;
}

void can_hardware_init(void)
{
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA12=CAN_TX, PA13=CAN_RX */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hcan1.Instance = CAN1;
    can_set_baudrate("125K");
    HAL_CAN_Init(&hcan1);

    /* Configure filter: accept all messages */
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan1, &filter);

    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

/* HAL RX interrupt callback → push to ring buffer */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    Message msg;

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header,
                         msg.data);

    msg.cob_id = rx_header.StdId;
    msg.len = rx_header.DLC;
    msg.rtr = (rx_header.RTR == CAN_RTR_REMOTE) ? 1 : 0;

    if (rx_count < CAN_RX_BUF_SIZE) {
        rx_buf[rx_head] = msg;
        rx_head = (rx_head + 1) % CAN_RX_BUF_SIZE;
        rx_count++;
    }
}

/* CANfestival driver: receive one message from ring buffer */
UNS8 canReceive_driver(CAN_HANDLE fd, Message *m)
{
    if (rx_count == 0) return 0;

    rt_enter_critical();
    *m = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % CAN_RX_BUF_SIZE;
    rx_count--;
    rt_exit_critical();

    return 1;
}

/* CANfestival driver: send one CAN message */
UNS8 canSend_driver(CAN_HANDLE fd, Message const *m)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox;

    tx_header.StdId = m->cob_id;
    tx_header.ExtId = 0;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = m->rtr ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC = m->len;
    tx_header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, (uint8_t *)m->data,
                             &tx_mailbox) != HAL_OK) {
        return 0;
    }
    return 1;
}

/* CANfestival driver: open CAN port */
CAN_HANDLE canOpen_driver(s_BOARD *board)
{
    /* Hardware already initialised in can_hardware_init() */
    return (CAN_HANDLE)CAN_PORT_NUM;
}

/* CANfestival driver: close CAN port */
int canClose_driver(CAN_HANDLE fd)
{
    HAL_CAN_Stop(&hcan1);
    return 0;
}

/* CANfestival driver: change baudrate */
UNS8 canChangeBaudRate_driver(CAN_HANDLE fd, char *baud)
{
    HAL_CAN_Stop(&hcan1);
    can_set_baudrate(baud);
    HAL_CAN_Init(&hcan1);
    HAL_CAN_Start(&hcan1);
    return 0;
}
```

---

### Task 6: 创建定时器适配 `port/timer_rtthread.h` + `port/timer_rtthread.c`

**Files:**
- Create: `canfestival/port/timer_rtthread.h`
- Create: `canfestival/port/timer_rtthread.c`

- [ ] **Step 1: 写入 timer_rtthread.h**

```c
#ifndef __TIMER_RTTHREAD_H__
#define __TIMER_RTTHREAD_H__

#include "applicfg.h"

void TimerInit(void);
void TimerCleanup(void);
void StartTimerLoop(void (*callback)(CO_Data *, UNS32));
void StopTimerLoop(void (*callback)(CO_Data *, UNS32));

#endif
```

- [ ] **Step 2: 写入 timer_rtthread.c**

```c
#include <rtthread.h>
#include "timer_rtthread.h"
#include "timerscfg.h"

static rt_timer_t g_timer = RT_NULL;
static void (*g_callback)(CO_Data *, UNS32) = RT_NULL;
static CO_Data *g_canopen_data = RT_NULL;
static UNS32 g_canopen_id = 0;
static rt_thread_t g_timer_thread = RT_NULL;

/* CANfestival calls this to get current time (ms) */
TIMEVAL getElapsedTime(void)
{
    return (TIMEVAL)rt_tick_get();
}

static void timer_callback(void *param)
{
    if (g_callback) {
        g_callback(g_canopen_data, g_canopen_id);
    }
}

/* CANfestival: set a single-shot timer */
void setTimer(TIMEVAL interval)
{
    if (g_timer) {
        rt_timer_stop(g_timer);
        rt_timer_control(g_timer, RT_TIMER_CTRL_SET_TIME,
                         (void *)&interval);
        rt_timer_start(g_timer);
    }
}

TIMEVAL canDelTimer(TIMEVAL interval)
{
    /* Not needed for basic master operation */
    return 0;
}

void TimerInit(void)
{
    g_timer = rt_timer_create("cantimer", timer_callback,
                              RT_NULL, 10,
                              RT_TIMER_FLAG_PERIODIC |
                              RT_TIMER_FLAG_SOFT_TIMER);
}

void TimerCleanup(void)
{
    if (g_timer) {
        rt_timer_stop(g_timer);
        rt_timer_delete(g_timer);
        g_timer = RT_NULL;
    }
}

static void timer_thread_entry(void *param)
{
    g_callback(g_canopen_data, g_canopen_id);

    /* Keep thread alive for timer callbacks.
     * CANfestival expects setTimer() to be called
     * which creates next timer event automatically */
    while (1) {
        rt_thread_mdelay(10);
    }
}

void StartTimerLoop(void (*callback)(CO_Data *, UNS32))
{
    g_callback = callback;
    g_canopen_data = RT_NULL;
    g_canopen_id = 0;
    TimerInit();
    g_timer_thread = rt_thread_create("cantloop", timer_thread_entry,
                                       RT_NULL, 1024, 8, 10);
    rt_thread_startup(g_timer_thread);
}

void StopTimerLoop(void (*callback)(CO_Data *, UNS32))
{
    if (g_timer_thread) {
        rt_thread_delete(g_timer_thread);
        g_timer_thread = RT_NULL;
    }
    TimerCleanup();
    g_callback = RT_NULL;
}
```

---

### Task 7: 生成主站对象字典 `od_master/ObjDict`

**Files:**
- Create: `canfestival/od_master/ObjDict.od`
- Generate (via Python): `canfestival/od_master/ObjDict.c`
- Generate (via Python): `canfestival/od_master/ObjDict.h`

- [ ] **Step 1: 创建最简主站 OD 文件 `ObjDict.od`**

CANfestival 使用 XML pickle 格式 (.od)，用 objdictgen 或手工编写。此处手工写最小化 OD：

```xml
<?xml version="1.0"?>
<!DOCTYPE PyObject SYSTEM "PyObjects.dtd">
<PyObject module="node" class="Node" id="0">
<attr name="Profile" type="dict" id="100">
</attr>
<attr name="Name" type="string">CanOpenMaster</attr>
<attr name="Dictionary" type="dict" id="200">
  <entry><key type="numeric" value="4096"/><val type="numeric" value="0"/></entry>
  <entry><key type="numeric" value="4097"/><val type="numeric" value="0"/></entry>
  <entry><key type="numeric" value="4101"/><val type="numeric" value="1073741824"/></entry>
  <entry><key type="numeric" value="4102"/><val type="numeric" value="50000"/></entry>
  <entry><key type="numeric" value="5632"/><val type="list" id="300">
    <item type="numeric" value="537853953"/>
    <item type="numeric" value="537919489"/>
    <item type="numeric" value="537985025"/>
    <item type="numeric" value="538050561"/>
    <item type="numeric" value="538116097"/>
    <item type="numeric" value="538181633"/>
    <item type="numeric" value="538247169"/>
    <item type="numeric" value="538312705"/>
  </val></entry>
  <entry><key type="numeric" value="5120"/><val type="list" id="400">
    <item type="numeric" value="384"/><item type="numeric" value="1"/>
    <item type="numeric" value="0"/><item type="numeric" value="0"/><item type="numeric" value="0"/>
  </val></entry>
  <entry><key type="numeric" value="6144"/><val type="list" id="500">
    <item type="numeric" value="512"/><item type="numeric" value="1"/>
    <item type="numeric" value="0"/><item type="numeric" value="0"/><item type="numeric" value="0"/>
  </val></entry>
  <entry><key type="numeric" value="4736"/><val type="list" id="600">
    <item type="numeric" value="1600"/><item type="numeric" value="1472"/><item type="numeric" value="64"/>
  </val></entry>
  <entry><key type="numeric" value="6656"/><val type="list" id="700">
    <item type="numeric" value="536870920"/>
  </val></entry>
  <entry><key type="numeric" value="4118"/><val type="list" id="800">
    <item type="numeric" value="4195804"/>
  </val></entry>
  <entry><key type="numeric" value="4120"/><val type="list" id="900">
    <item type="numeric" value="0"/><item type="numeric" value="0"/>
    <item type="numeric" value="0"/><item type="numeric" value="0"/>
  </val></entry>
</attr>
<attr name="SpecificMenu" type="list" id="1000">
</attr>
<attr name="ParamsDictionary" type="dict" id="1100">
</attr>
<attr name="UserMapping" type="dict" id="1200">
</attr>
<attr name="DS302" type="dict" id="1300">
</attr>
<attr name="ProfileName" type="string" value="DS-301"/>
<attr name="Type" type="string">master</attr>
<attr name="ID" type="numeric" value="1"/>
</PyObject>
```

- [ ] **Step 2: 用 Python/OBDGen 生成 ObjDict.c 和 ObjDict.h**

使用 CANfestival 自带的 `gen_cfile.py` 生成 C 代码：

```powershell
python "D:\workspace\app\canfestival\objdictgen\gen_cfile.py" `
  "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\od_master\ObjDict.od" `
  "D:\RT-ThreadStudio\workspace\canopen_stm32\canfestival\od_master"
```

> 如果 gen_cfile.py 报错 GDK/import 问题，则手工编写 ObjDict.c/.h（见下一步）。

- [ ] **Step 3: 备选——手工编写 ObjDict.h**

```c
#ifndef OBJDICT_H
#define OBJDICT_H

#include "data.h"

/* Master object dictionary — minimal for CANopen master operation */
extern CO_Data CanOpenMaster_Data;

const indextable *CanOpenMaster_scanIndexOD(UNS16 wIndex,
    UNS32 *errorCode, ODCallback_t **callbacks);

UNS32 CanOpenMaster_valueRangeTest(UNS8 typeValue, void *value);

#endif
```

- [ ] **Step 4: 备选——手工编写 ObjDict.c（最简主站 OD）**

```c
#include "ObjDict.h"
#include "objdictdef.h"

/* Device type (0x1000) = 0 (no profile) */
static UNS32 device_type = 0x00000000;

/* Error register (0x1001) */
static UNS8 error_register = 0x00;

/* SYNC COB-ID (0x1005) */
static UNS32 sync_cobid = 0x00000080;

/* Communication cycle period (0x1006) = 50000us = 50ms */
static UNS32 sync_period = 50000;

/* RPDO1 COB-ID (0x1400.01) — will be set at runtime */
static subindex RPDO1_COB_ID_sub[] = {
    {RO, sizeof(UNS32), (void *)0},
    {RW, sizeof(UNS32), (void *)0},
    {RW, sizeof(UNS8),  (void *)0},
    {RW, sizeof(UNS8),  (void *)0},
    {RW, sizeof(UNS16), (void *)0},
    {RW, sizeof(UNS8),  (void *)0},
};

static UNS32 RPDO1_cobid_default = 0x80000182;
static UNS8  RPDO1_trans_type = 0xFF;
static UNS8  RPDO1_inhibit = 0x00;
static UNS16 RPDO1_event_timer = 0x0000;
static UNS8  RPDO1_sync_start = 0x00;

static ODCallback_t RPDO1_callbacks[] = { NULL };

/* TPDO1 COB-ID (0x1800.01) — will be set at runtime */
static subindex TPDO1_COB_ID_sub[] = {
    {RO, sizeof(UNS32), (void *)0},
    {RW, sizeof(UNS32), (void *)0},
    {RW, sizeof(UNS8),  (void *)0},
    {RW, sizeof(UNS8),  (void *)0},
    {RW, sizeof(UNS16), (void *)0},
    {RW, sizeof(UNS8),  (void *)0},
};

static UNS32 TPDO1_cobid_default = 0x40000202;
static UNS8  TPDO1_trans_type = 0xFF;
static UNS8  TPDO1_inhibit = 0x00;
static UNS16 TPDO1_event_timer = 0x0000;
static UNS8  TPDO1_sync_start = 0x00;

static ODCallback_t TPDO1_callbacks[] = { NULL };

/* Heartbeat consumer */
static UNS32 heartbeat_consumer = 0x00000000;

/* Index table */
static const indextable CanOpenMaster_idx[] = {
    { (subindex *)&device_type,       sizeof(UNS32), 0 },
    { (subindex *)&error_register,    sizeof(UNS8),  0 },
    { (subindex *)&sync_cobid,        sizeof(UNS32), 0 },
    { (subindex *)&sync_period,       sizeof(UNS32), 0 },
    { (subindex *)RPDO1_COB_ID_sub,   sizeof(UNS32), 5 },
    { (subindex *)TPDO1_COB_ID_sub,   sizeof(UNS32), 5 },
    { (subindex *)&heartbeat_consumer, sizeof(UNS32), 0 },
};

static const UNS16 CanOpenMaster_idx_size = sizeof(CanOpenMaster_idx) /
                                            sizeof(indextable);

/* Scan OD — called by CANfestival to look up an index */
const indextable *CanOpenMaster_scanIndexOD(UNS16 wIndex,
    UNS32 *errorCode, ODCallback_t **callbacks)
{
    static const UNS16 base_indices[] = {
        0x1000, 0x1001, 0x1005, 0x1006, 0x1400, 0x1800, 0x1016,
    };
    static const UNS16 base_count = sizeof(base_indices) / sizeof(UNS16);

    for (int i = 0; i < base_count; i++) {
        if (wIndex == base_indices[i]) {
            *errorCode = OD_SUCCESSFUL;
            return &CanOpenMaster_idx[i];
        }
    }
    *errorCode = OD_NO_SUCH_OBJECT;
    return NULL;
}

/* Value range test — always OK for now */
UNS32 CanOpenMaster_valueRangeTest(UNS8 typeValue, void *value)
{
    return 0;
}

/* Global OD data instance */
CO_Data CanOpenMaster_Data = CANOPEN_NODE_DATA_INITIALIZER(CanOpenMaster);
```

---

### Task 8: 创建 CANopen 主站模块 `canopen_master.c/h`

**Files:**
- Create: `applications/canopen_master.h`
- Create: `applications/canopen_master.c`

- [ ] **Step 1: 写入 canopen_master.h**

```c
#ifndef __CANOPEN_MASTER_H__
#define __CANOPEN_MASTER_H__

#include <rtthread.h>
#include "canfestival/port/canfestival.h"
#include "canfestival/port/can_stm32.h"
#include "canfestival/port/timer_rtthread.h"
#include "canfestival/od_master/ObjDict.h"

#define SLAVE_NODE_ID  0x02

int canopen_master_init(void);

/* External: CANopen master data (for SDO operations) */
extern CO_Data CanOpenMaster_Data;

#endif
```

- [ ] **Step 2: 写入 canopen_master.c**

> Steps 2-4 split for clarity, but single file.

```c
#include "canopen_master.h"

#define DBG_TAG "canopen"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static s_BOARD master_board = {"can0", "125K"};
static int init_step = 0;

/* ── Callbacks ────────────────────────────────────── */

static void CheckSDOAndContinue(CO_Data *d, UNS8 nodeId)
{
    UNS32 abortCode;
    if (getWriteResultNetworkDict(d, nodeId, &abortCode) != SDO_FINISHED) {
        LOG_E("SDO fail: node 0x%02X, step %d, abort=0x%04X",
              nodeId, init_step, (unsigned)abortCode);
    }
    closeSDOtransfer(d, nodeId, SDO_CLIENT);
    ConfigureSlaveNode(d, nodeId);
}

static void ConfigureSlaveNode(CO_Data *d, UNS8 nodeId)
{
    UNS8 res;

    switch (++init_step) {
    case 1: { /* Disable slave TPDO1 */
        UNS32 cobid = 0x80000180 + nodeId;
        res = writeNetworkDictCallBack(d, nodeId, 0x1800, 0x01, 4, 0,
                                       &cobid, CheckSDOAndContinue, 0);
    } break;
    case 2: { /* Set slave TPDO1 to SYNC transmit */
        UNS8 type = 0x01;
        res = writeNetworkDictCallBack(d, nodeId, 0x1800, 0x02, 1, 0,
                                       &type, CheckSDOAndContinue, 0);
    } break;
    case 3: { /* Re-enable slave TPDO1 */
        UNS32 cobid = 0x00000180 + nodeId;
        res = writeNetworkDictCallBack(d, nodeId, 0x1800, 0x01, 4, 0,
                                       &cobid, CheckSDOAndContinue, 0);
    } break;
    case 4: { /* Disable slave RPDO1 */
        UNS32 cobid = 0x80000200 + nodeId;
        res = writeNetworkDictCallBack(d, nodeId, 0x1400, 0x01, 4, 0,
                                       &cobid, CheckSDOAndContinue, 0);
    } break;
    case 5: { /* Set slave RPDO1 receive type */
        UNS8 type = 0xFF;  /* asynchronous */
        res = writeNetworkDictCallBack(d, nodeId, 0x1400, 0x02, 1, 0,
                                       &type, CheckSDOAndContinue, 0);
    } break;
    case 6: { /* Re-enable slave RPDO1 */
        UNS32 cobid = 0x00000200 + nodeId;
        res = writeNetworkDictCallBack(d, nodeId, 0x1400, 0x01, 4, 0,
                                       &cobid, CheckSDOAndContinue, 0);
    } break;
    case 7: { /* Set heartbeat producer time = 1000ms */
        UNS16 hb_time = 1000;
        res = writeNetworkDictCallBack(d, nodeId, 0x1017, 0x00, 2, 0,
                                       &hb_time, CheckSDOAndContinue, 0);
    } break;
    case 8: { /* All done — go operational */
        setState(d, Operational);
        masterSendNMTstateChange(d, nodeId, NMT_Start_Node);
        LOG_I("Slave 0x%02X configured and started", nodeId);
        init_step = 0;
    } break;
    }
}

/* ── State transition callbacks ──────────────────── */

void TestMaster_initialisation(CO_Data *d)
{
    LOG_I("Master: initialisation");
    /* Bind RPDO1 to receive slave TPDO1 */
    UNS32 rpdo_cobid = 0x0180 + SLAVE_NODE_ID;
    UNS32 tpdo_cobid = 0x0200 + SLAVE_NODE_ID;
    UNS32 size = sizeof(UNS32);

    writeLocalDict(d, 0x1400, 0x01, &rpdo_cobid, &size, RW);
    writeLocalDict(d, 0x1800, 0x01, &tpdo_cobid, &size, RW);
}

void TestMaster_preOperational(CO_Data *d)
{
    LOG_I("Master: preOperational");
    ConfigureSlaveNode(d, SLAVE_NODE_ID);
}

void TestMaster_operational(CO_Data *d)
{
    LOG_I("Master: operational");
}

void TestMaster_stopped(CO_Data *d)
{
    LOG_I("Master: stopped");
}

void TestMaster_post_sync(CO_Data *d)
{
    /* Optional: read PDO data here */
}

void TestMaster_post_TPDO(CO_Data *d)
{
}

void TestMaster_heartbeatError(CO_Data *d, UNS8 id)
{
    LOG_E("Heartbeat lost: node 0x%02X", id);
}

void TestMaster_post_SlaveBootup(CO_Data *d, UNS8 nodeId)
{
    LOG_I("Slave 0x%02X booted, starting...", nodeId);
    masterSendNMTstateChange(d, nodeId, NMT_Start_Node);
}

/* ── Init / Deinit ──────────────────────────────── */

static void InitNodes(CO_Data *d, UNS32 id)
{
    setNodeId(&CanOpenMaster_Data, 0x01);
    setState(&CanOpenMaster_Data, Initialisation);
}

int canopen_master_init(void)
{
    LOG_I("Initialising CANopen master...");

    /* 1. Hardware */
    can_hardware_init();

    /* 2. Set callbacks */
    CanOpenMaster_Data.initialisation    = TestMaster_initialisation;
    CanOpenMaster_Data.preOperational    = TestMaster_preOperational;
    CanOpenMaster_Data.operational       = TestMaster_operational;
    CanOpenMaster_Data.stopped           = TestMaster_stopped;
    CanOpenMaster_Data.post_sync         = TestMaster_post_sync;
    CanOpenMaster_Data.post_TPDO         = TestMaster_post_TPDO;
    CanOpenMaster_Data.heartbeatError    = TestMaster_heartbeatError;
    CanOpenMaster_Data.post_SlaveBootup  = TestMaster_post_SlaveBootup;

    /* 3. Open CAN */
    if (!canOpen(&master_board, &CanOpenMaster_Data)) {
        LOG_E("Cannot open CAN bus!");
        return -1;
    }

    /* 4. Start timer loop */
    StartTimerLoop(&InitNodes);

    LOG_I("CANopen master started (Node ID=0x01)");
    return 0;
}
```

---

### Task 9: 创建寄存器路由表 `reg_router.c/h`

**Files:**
- Create: `applications/reg_router.h`
- Create: `applications/reg_router.c`

- [ ] **Step 1: 写入 reg_router.h**

```c
#ifndef __REG_ROUTER_H__
#define __REG_ROUTER_H__

#include <stdint.h>

/* Access flags */
#define REG_RO  0x01
#define REG_WO  0x02
#define REG_RW  0x03

/* CANopen data types (matching CANfestival) */
#define DTTYPE_UNS8   0x05
#define DTTYPE_UNS16  0x06
#define DTTYPE_UNS32  0x07
#define DTTYPE_UNS64  0x1B

typedef struct {
    uint16_t addr_start;
    uint16_t addr_end;
    uint8_t  node_id;
    uint16_t od_index;
    uint8_t  od_subindex;
    uint8_t  access;
    uint8_t  data_type;    /* CANfestival data type constant */
} reg_route_entry_t;

void reg_router_init(void);
const reg_route_entry_t *reg_lookup(uint16_t addr);
int  reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf);
int  reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data);

#endif
```

- [ ] **Step 2: 写入 reg_router.c（含完整路由表）**

```c
#include "reg_router.h"
#include <rtthread.h>
#include "canopen_master.h"

#define DBG_TAG "router"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* ── Routing table (all → Node 0x02) ──────────────────── */

static const reg_route_entry_t g_routes[] = {
    /* Control & Status (0x2000~0x201F) */
    { 0x0001, 0x0003, 0x02, 0x2000, 0x00, REG_RO,  DTTYPE_UNS64 },  /* 孔位状态 */
    { 0x0004, 0x0004, 0x02, 0x2001, 0x00, REG_RW,  DTTYPE_UNS8  },  /* 清洗针头 */
    { 0x0005, 0x0005, 0x02, 0x2002, 0x00, REG_RW,  DTTYPE_UNS8  },  /* 取液/注液/清洗 */
    { 0x0006, 0x0007, 0x02, 0x2003, 0x00, REG_RW,  DTTYPE_UNS32 },  /* 柱塞泵液量 */
    { 0x0008, 0x0008, 0x02, 0x2004, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 蠕动泵转速 */
    { 0x0009, 0x0009, 0x02, 0x2005, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 蠕动泵圈数 */
    { 0x000A, 0x000A, 0x02, 0x2006, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 操作位号 */
    { 0x000B, 0x000B, 0x02, 0x2007, 0x00, REG_RW,  DTTYPE_UNS8  },  /* 转盘使能 */
    { 0x000C, 0x000E, 0x02, 0x2008, 0x00, REG_RW,  DTTYPE_UNS64 },  /* 转盘指定位号 */
    { 0x000F, 0x000F, 0x02, 0x2009, 0x00, REG_RW,  DTTYPE_UNS8  },  /* 制冷使能 */
    { 0x0010, 0x0011, 0x02, 0x200A, 0x00, REG_RO,  DTTYPE_UNS32 },  /* 光强读数 */
    { 0x0012, 0x0013, 0x02, 0x200B, 0x00, REG_RO,  DTTYPE_UNS32 },  /* 仪器状态 */
    { 0x0014, 0x0014, 0x02, 0x200C, 0x00, REG_RO,  DTTYPE_UNS8  },  /* 清水箱异常 */
    { 0x0015, 0x0015, 0x02, 0x200D, 0x00, REG_RO,  DTTYPE_UNS8  },  /* 废水箱异常 */
    { 0x0016, 0x0016, 0x02, 0x200E, 0x00, REG_RO,  DTTYPE_UNS8  },  /* 缓冲液异常 */
    { 0x0017, 0x0017, 0x02, 0x200F, 0x00, REG_RO,  DTTYPE_UNS8  },  /* 试剂异常 */
    { 0x0018, 0x0019, 0x02, 0x2010, 0x00, REG_RO,  DTTYPE_UNS16 },  /* 样品异常 */
    { 0x001A, 0x001B, 0x02, 0x2011, 0x00, REG_RO,  DTTYPE_UNS16 },  /* 比色皿异常 */
    { 0x001C, 0x001E, 0x02, 0x2012, 0x00, REG_RO,  DTTYPE_UNS32 },  /* 针头异常 */
    { 0x001F, 0x001F, 0x02, 0x2013, 0x00, REG_RO,  DTTYPE_UNS8  },  /* 温控异常 */
    { 0x0020, 0x0021, 0x02, 0x2014, 0x00, REG_RO,  DTTYPE_UNS16 },  /* 比色皿脏污 */

    /* Real-time Data (0x2100~0x2104) */
    { 0x0064, 0x0064, 0x02, 0x2100, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 转盘加热温度 0x64=100 */
    { 0x0065, 0x0065, 0x02, 0x2101, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 制冷温度 0x65=101 */
    { 0x0066, 0x0066, 0x02, 0x2102, 0x00, REG_RO,  DTTYPE_UNS16 },  /* 清水重量 0x66=102 */
    { 0x0067, 0x0067, 0x02, 0x2103, 0x00, REG_RO,  DTTYPE_UNS16 },  /* 缓冲液重量 */
    { 0x0068, 0x0068, 0x02, 0x2104, 0x00, REG_RO,  DTTYPE_UNS16 },  /* 废液重量 */

    /* Parameter Settings (0x2200~0x224E) */
    { 0x0078, 0x0078, 0x02, 0x2200, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 参数控制字 0x78=120 */
    { 0x0079, 0x0079, 0x02, 0x2201, 0x00, REG_RW,  DTTYPE_UNS8  },  /* 称重控制 */
    { 0x0082, 0x0082, 0x02, 0x2202, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 机械臂深度 130 */
    { 0x0083, 0x0083, 0x02, 0x2203, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 机械臂角度 131 */
    { 0x0084, 0x0084, 0x02, 0x2204, 0x00, REG_RW,  DTTYPE_UNS16 },  /* 转盘角度 132 */
};

#define ROUTE_COUNT (sizeof(g_routes) / sizeof(g_routes[0]))

/* ── API ────────────────────────────────────────── */

void reg_router_init(void)
{
    LOG_I("Router: %d entries loaded", (int)ROUTE_COUNT);
}

const reg_route_entry_t *reg_lookup(uint16_t addr)
{
    for (int i = 0; i < ROUTE_COUNT; i++) {
        if (addr >= g_routes[i].addr_start &&
            addr <= g_routes[i].addr_end) {
            return &g_routes[i];
        }
    }
    return NULL;
}

/* Helper: CANfestival type → byte size */
static uint8_t type_size(uint8_t dtype)
{
    switch (dtype) {
        case DTTYPE_UNS8:  return 1;
        case DTTYPE_UNS16: return 2;
        case DTTYPE_UNS32: return 4;
        case DTTYPE_UNS64: return 8;
        default:           return 2;
    }
}

int reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf)
{
    int offset = 0;
    for (int i = 0; i < count; i++) {
        const reg_route_entry_t *e = reg_lookup(start_addr + i);
        if (!e) {
            LOG_E("Route not found: addr=0x%04X", start_addr + i);
            return -1;
        }
        if (!(e->access & REG_RO)) {
            LOG_E("Write-only: addr=0x%04X", start_addr + i);
            return -1;
        }

        UNS8 size = type_size(e->data_type);
        UNS32 abortCode;
        UNS8 res = readNetworkDict(&CanOpenMaster_Data, e->node_id,
                                   e->od_index, e->od_subindex,
                                   e->data_type, 0);
        if (res) {
            LOG_E("SDO read fail: addr=0x%04X, err=%d", start_addr + i, res);
            return -1;
        }
        /* Wait for SDO complete */
        rt_thread_mdelay(50);
        if (getReadResultNetworkDict(&CanOpenMaster_Data, e->node_id,
                                     out_buf + offset, &size, &abortCode)
            != SDO_FINISHED) {
            LOG_E("SDO read timeout: addr=0x%04X", start_addr + i);
            return -1;
        }
        closeSDOtransfer(&CanOpenMaster_Data, e->node_id, SDO_CLIENT);
        offset += size;
    }
    return offset; /* total bytes */
}

int reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data)
{
    int offset = 0;
    for (int i = 0; i < count; i++) {
        const reg_route_entry_t *e = reg_lookup(start_addr + i);
        if (!e) {
            LOG_E("Route not found: addr=0x%04X", start_addr + i);
            return -1;
        }
        if (!(e->access & REG_WO)) {
            LOG_E("Read-only: addr=0x%04X", start_addr + i);
            return -1;
        }

        UNS8 size = type_size(e->data_type);
        UNS32 abortCode;
        UNS8 res = writeNetworkDict(&CanOpenMaster_Data, e->node_id,
                                    e->od_index, e->od_subindex,
                                    size, e->data_type,
                                    (void *)(data + offset), 0);
        if (res) {
            LOG_E("SDO write fail: addr=0x%04X, err=%d", start_addr + i, res);
            return -1;
        }
        rt_thread_mdelay(50);
        if (getWriteResultNetworkDict(&CanOpenMaster_Data, e->node_id,
                                      &abortCode) != SDO_FINISHED) {
            LOG_E("SDO write timeout: addr=0x%04X", start_addr + i);
            return -1;
        }
        closeSDOtransfer(&CanOpenMaster_Data, e->node_id, SDO_CLIENT);
        offset += size;
    }
    return 0;
}
```

---

### Task 10: 创建串口协议处理 `serial_protocol.c/h`

**Files:**
- Create: `applications/serial_protocol.h`
- Create: `applications/serial_protocol.c`

- [ ] **Step 1: 写入 serial_protocol.h**

```c
#ifndef __SERIAL_PROTOCOL_H__
#define __SERIAL_PROTOCOL_H__

#include <stdint.h>

void serial_protocol_init(void);

#endif
```

- [ ] **Step 2: 写入 serial_protocol.c**

```c
#include "serial_protocol.h"
#include "reg_router.h"
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG "serial"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define FRAME_HDR1  0xAA
#define FRAME_HDR2  0x55

#define CMD_READ        0x03
#define CMD_WRITE_SINGLE 0x06
#define CMD_WRITE_MULTI  0x10

#define ERR_OK          0x00
#define ERR_CHECKSUM    0x01
#define ERR_CMD         0x02
#define ERR_ADDR        0x03
#define ERR_ACCESS      0x04

static rt_device_t g_serial = RT_NULL;

/* Frame receive state machine */
static enum { S_HDR1, S_HDR2, S_CMD, S_ADDR_L, S_ADDR_H,
              S_LEN, S_DATA, S_XOR } g_state = S_HDR1;
static uint8_t  g_rx_buf[256];
static uint8_t  g_rx_idx = 0;
static uint8_t  g_rx_len = 0;

static uint8_t xor_checksum(const uint8_t *buf, uint8_t len)
{
    uint8_t x = 0;
    for (int i = 0; i < len; i++) x ^= buf[i];
    return x;
}

static void send_response(uint8_t cmd, uint16_t addr,
                          uint8_t count, const uint8_t *data)
{
    uint8_t buf[256];
    uint8_t idx = 0;

    buf[idx++] = FRAME_HDR1;
    buf[idx++] = FRAME_HDR2;
    buf[idx++] = cmd;
    buf[idx++] = addr & 0xFF;
    buf[idx++] = (addr >> 8) & 0xFF;
    buf[idx++] = count;

    for (int i = 0; i < count; i++) buf[idx++] = data[i];

    buf[idx] = xor_checksum(buf, idx);
    idx++;

    rt_device_write(g_serial, 0, buf, idx);
}

static void send_error(uint8_t code)
{
    uint8_t buf[] = { FRAME_HDR1, FRAME_HDR2, 0xFF, code, 0x00 };
    buf[4] = xor_checksum(buf, 4);
    rt_device_write(g_serial, 0, buf, 5);
}

static void process_frame(void)
{
    uint8_t cmd   = g_rx_buf[2];
    uint16_t addr = g_rx_buf[3] | (g_rx_buf[4] << 8);
    uint8_t count = g_rx_buf[5];
    uint8_t xsum  = xor_checksum(g_rx_buf, g_rx_idx);

    if (xsum != 0) {
        LOG_W("Checksum error");
        send_error(ERR_CHECKSUM);
        return;
    }

    switch (cmd) {
    case CMD_READ: {
        uint8_t resp[256];
        int n = reg_read(addr, count, resp);
        if (n < 0) { send_error(ERR_ADDR); break; }
        send_response(CMD_READ, addr, (uint8_t)n, resp);
        break;
    }
    case CMD_WRITE_SINGLE:
    case CMD_WRITE_MULTI: {
        if (reg_write(addr, count, g_rx_buf + 6) != 0) {
            send_error(ERR_ACCESS);
            break;
        }
        /* Echo back */
        send_response(cmd, addr, 0, NULL);
        break;
    }
    default:
        LOG_W("Unknown cmd: 0x%02X", cmd);
        send_error(ERR_CMD);
        break;
    }
}

static rt_err_t serial_rx_cb(rt_device_t dev, rt_size_t size)
{
    uint8_t ch;
    while (rt_device_read(dev, 0, &ch, 1) == 1) {
        switch (g_state) {
        case S_HDR1:
            if (ch == FRAME_HDR1) {
                g_rx_buf[0] = ch;
                g_rx_idx = 1;
                g_state = S_HDR2;
            }
            break;
        case S_HDR2:
            if (ch == FRAME_HDR2) {
                g_rx_buf[1] = ch;
                g_rx_idx = 2;
                g_state = S_CMD;
            } else {
                g_state = S_HDR1;
            }
            break;
        case S_CMD:
            g_rx_buf[2] = ch;
            g_rx_idx = 3;
            g_state = S_ADDR_L;
            break;
        case S_ADDR_L:
            g_rx_buf[3] = ch;
            g_rx_idx = 4;
            g_state = S_ADDR_H;
            break;
        case S_ADDR_H:
            g_rx_buf[4] = ch;
            g_rx_idx = 5;
            g_state = S_LEN;
            break;
        case S_LEN:
            g_rx_buf[5] = ch;
            g_rx_idx = 6;
            g_rx_len = ch;
            if (g_rx_len > 0) {
                g_state = S_DATA;
            } else {
                g_state = S_XOR;
            }
            break;
        case S_DATA:
            g_rx_buf[6 + (g_rx_idx - 6)] = ch;
            g_rx_idx++;
            if (g_rx_idx >= 6 + g_rx_len) {
                g_state = S_XOR;
            }
            break;
        case S_XOR:
            g_rx_buf[g_rx_idx] = ch;
            g_rx_idx++;
            process_frame();
            g_state = S_HDR1;
            break;
        }
    }
    return RT_EOK;
}

void serial_protocol_init(void)
{
    g_serial = rt_device_find("uart1");
    if (!g_serial) {
        LOG_E("Cannot find uart1");
        return;
    }
    rt_device_open(g_serial, RT_DEVICE_FLAG_RDWR |
                   RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(g_serial, serial_rx_cb);

    LOG_I("Serial protocol ready (uart1, 115200)");
}
```

---

### Task 11: 构建集成 — `canfestival/SConscript`

**Files:**
- Create: `canfestival/SConscript`

- [ ] **Step 1: 写入 canfestival/SConscript**

```python
import os
from building import *

cwd = GetCurrentDir()
src = []

# CANfestival core
src += Glob('src/*.c')

# Platform port
src += Glob('port/*.c')

# Object dictionary
src += Glob('od_master/*.c')

# Include paths
inc = [
    cwd + '/include',
    cwd + '/port',
    cwd + '/od_master',
]

group = DefineGroup('canfestival', src,
                    depend=[''],
                    CPPPATH=inc)
Return('group')
```

---

### Task 12: 修改 `applications/SConscript` 添加新源文件

**Files:**
- Modify: `applications/SConscript`

- [ ] **Step 1: 读取现有 SConscript**

```python
# 查看: D:\RT-ThreadStudio\workspace\canopen_stm32\applications\SConscript
```

- [ ] **Step 2: 如果不存在则创建**

如果 applications 目录没有自己的 SConscript（很可能没有，因为 main.c 通过顶层 SConscript 的通配编译），不需要额外操作——顶层 SConscript 的 for 循环会自动发现新 .c 文件。

如果存在 applications/SConscript，检查是否有 `Glob('*.c')` 或显式的 `src` 列表，确保新文件被包含。

---

### Task 13: 修改 `rtconfig.h` 启用 CAN

**Files:**
- Modify: `rtconfig.h`

- [ ] **Step 1: 在 rtconfig.h 末尾（`#endif` 之前）添加 CAN 配置**

```c
/* CAN Bus */
#define BSP_USING_CAN
#define BSP_USING_CAN1
```

同时确保 STM32 HAL CAN 中断使能。检查 `stm32f1xx_hal_conf.h`（`drivers/stm32f1xx_hal_conf.h`），确保 `HAL_CAN_MODULE_ENABLED` 已定义或未被注释。

---

### Task 14: 修改 `main.c` — 集成所有模块

**Files:**
- Modify: `applications/main.c`

- [ ] **Step 1: 完全替换 main.c**

```c
/*
 * CANopen Master — Protocol Gateway
 * RT-Thread + STM32F103 + CANfestival
 */

#include <rtthread.h>
#include "canopen_master.h"
#include "serial_protocol.h"
#include "reg_router.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    LOG_I("====================================");
    LOG_I(" CANopen Master Protocol Gateway");
    LOG_I(" STM32F103RCT6, RT-Thread v4.0.3");
    LOG_I("====================================");

    /* 1. Init routing table */
    reg_router_init();

    /* 2. Init CANopen master (starts CAN + timer loop) */
    if (canopen_master_init() != 0) {
        LOG_E("CANopen master init failed!");
        while (1) rt_thread_mdelay(1000);
    }

    /* 3. Init serial protocol (starts RX handler) */
    serial_protocol_init();

    LOG_I("System ready. Waiting for commands...");

    while (1) {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
```

---

### Task 15: 编译验证

- [ ] **Step 1: 首次编译**

```powershell
scons -j4
```

预期：大量编译错误（头文件路径、类型不匹配、未定义引用等）。

- [ ] **Step 2: 修复编译错误（常见问题）**

常见修复：

**a. RT-Thread 的 bool 类型冲突**

CANfestival 在 `sysdep.h` 中定义 `TRUE=1, FALSE=0`，可能与 RT-Thread 冲突。在 `applicfg.h` 中最后添加：

```c
/* Avoid conflict with RT-Thread's TRUE/FALSE */
#ifdef TRUE
  #define CANFESTIVAL_TRUE  TRUE
  #define CANFESTIVAL_FALSE FALSE
  #undef TRUE
  #undef FALSE
  #define TRUE  CANFESTIVAL_TRUE
  #define FALSE CANFESTIVAL_FALSE
#endif
```

**b. getElapsedTime 已在 timer_rtthread.c 中定义，但 CANfestival core 在 `timer.c` 中有 extern 声明**

检查 `timer.c` 中对 `getElapsedTime` 的外部引用声明是否匹配。

**c. HAL CAN 头文件路径**

确保 SCons 构建能 find STM32 HAL 头文件。检查 `rtconfig.py` 中 `CPPPATH` 是否包含 HAL 路径。需要确保 `libraries/STM32F1xx_HAL_Driver/Inc` 和 `libraries/CMSIS/Device/ST/STM32F1xx/Include` 在 include path 中。

**d. `CAN_HANDLE` 类型**

`can_driver.h` 中 `CAN_HANDLE` 定义为 `int` 或指针。`can_stm32.c` 中使用 `(CAN_HANDLE)CAN_PORT_NUM`（int 1）。确保与 `can.h` 中的定义一致。

- [ ] **Step 3: 持续迭代修复，直到编译通过**

每次修复后运行 `scons -j4`，直到 `rtthread.elf` 生成成功。

预期最终输出：
```
arm-none-eabi-size rtthread.elf
   text    data     bss     dec     hex filename
  8xxxx     xxx     xxx   xxxxx   xxxxx rtthread.elf
```

---

### Task 16: 烧录 & 功能验证

- [ ] **Step 1: 烧录到 STM32F103RCT6**

通过 ST-Link 烧录 `Debug/rtthread.bin` 或 `Debug/rtthread.elf`。

- [ ] **Step 2: 检查串口输出**

连接 USART1 (PA9/PA10)，波特率 115200。开机预期输出：

```
 CANopen Master Protocol Gateway
 STM32F103RCT6, RT-Thread v4.0.3
Router: 37 entries loaded
Master: initialisation
Master: preOperational
Master: operational
System ready. Waiting for commands...
```

- [ ] **Step 3: 用 USB-CAN 工具模拟从站**

配置 USB-CAN 作为从站（Node ID 0x02）：
- 响应对 0x1800/0x1400 的 SDO 配置（或者直接进入 Operational）
- 回复对 0x1017 的 SDO 写

验证主站能发出正确的 NMT/SDO 报文。

- [ ] **Step 4: 串口发送测试帧**

用串口助手发送（HEX）：

```
读仪器状态 (00018~00019 = 0x0012): AA 55 03 12 00 02 B6
写针头清洗 (00004 = 0x0004, 值=1):  AA 55 06 04 00 01 00 A8
```

观察逻辑分析仪 / PCAN-View 上对应的 CAN 报文是否发出。

---

### Task 17: 后处理 — 文档 & 提交

- [ ] **Step 1: 更新 TOOLS.md 或 memory 笔记**

记录新建的工程目录、CAN 配置、串口协议格式等。

- [ ] **Step 2: git commit**

```bash
git add -A
git commit -m "feat: integrate CANfestival master with serial-to-CANopen routing"
```

