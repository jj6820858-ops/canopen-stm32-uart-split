# CANopen 总线分析 + Modbus ↔ CAN 映射

> **数据**：`上电校验.csv`（24900 帧, ~24 秒上电校验过程）
> **关联**：与 `modbus-protocol-deep-analysis.md` 对照，建立 Modbus 寄存器 → CANopen 对象的映射
> **目的**：MCU 代码实现 — STM32 作为 Modbus 从站 + CanOpen 主站
> **轴映射修订**：0x301=X/针头旋转, 0x302=Y/转盘, 0x303=Z/上下, 0x304=E/柱塞泵。

---

## 一、CANopen 网络拓扑

```
                    STM32 (CanOpen Master)
                    Node ID: 可能是 7 或独立
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   ┌────▼────┐    ┌─────▼─────┐    ┌─────▼─────┐
   │ Node 1  │    │  Node 2   │    │  Node 3   │    Node 4
   │ X针头旋转│    │  Y转盘     │    │  Z上下     │    E柱塞泵
   └─────────┘    └───────────┘    └───────────┘    └──────────┘
   
   ┌─────────┐    ┌───────────┐
   │ Node 6  │    │  Node 8   │
   │ 主编码器?│    │ 编码器反馈?│
   └─────────┘    └───────────┘
   
   Node 5: 未激活 (仅 1 帧)
   Node 7: 极少流量 (可能为 MCU 自身)
```

### COB-ID 完整分配

| COB-ID | CANopen 标识 | 帧数 | 方向 | 数据长度 | 推测用途 |
|--------|-------------|------|------|---------|---------|
| **0x000** | NMT | 3 | — | 2B | 启动 Node 1,4,7 |
| 0x181 | TPDO1 Node1 | 257 | 反馈 | 2B | 电机A 状态 |
| 0x182 | TPDO1 Node2 | 258 | 反馈 | 2B | 电机B 状态 |
| 0x183 | TPDO1 Node3 | 226 | 反馈 | 2B | 电机C 状态 |
| 0x184 | TPDO1 Node4 | 225 | 反馈 | 2B | 电机D 状态 |
| **0x186** | **TPDO1 Node6** | **488** | **反馈** | **4B** | **位置反馈（32bit）** |
| **0x188** | **TPDO1 Node8** | **245** | **反馈** | **8B** | **双通道位置反馈** |
| 0x206 | RPDO1 Node6 | 1299 | 命令 | 4B | 0x00→0x18 切换 |
| 0x207 | RPDO1 Node7 | 123 | 命令 | 8B | 稀疏 |
| 0x288 | TPDO2 Node8 | 229 | 反馈 | 4B | Node8 第二 PDO |
| **0x301** | **RPDO2 Node1** | **7677** | **命令** | **8B** | **X / 针头旋转位置命令** |
| **0x302** | **RPDO2 Node2** | **4541** | **命令** | **8B** | **Y / 转盘位置命令** |
| **0x303** | **RPDO2 Node3** | **4630** | **命令** | **8B** | **Z / 上下位置命令** |
| **0x304** | **RPDO2 Node4** | **4546** | **命令** | **8B** | **E / 柱塞泵位置命令** |
| 0x305 | RPDO2 Node5 | 1 | — | 8B | 未使用 |
| 0x306 | RPDO2 Node6 | 56 | 命令 | 3B | 特殊配置 |
| 0x307 | RPDO2 Node7 | 45 | 命令 | 4B | 稀疏 |
| 0x581/601 | SDO Node1 | 15 | 双向 | 8B | 参数配置 |
| 0x582/602 | SDO Node2 | 2 | 双向 | 8B | 参数配置 |
| 0x583/603 | SDO Node3 | 5 | 双向 | 8B | 参数配置 |
| 0x701-708 | Heartbeat | 33 | 反馈 | 1B | 节点心跳 |

---

## 二、SDO 配置序列

### 上电配置流程

```
t=0.001s  — 批量配置阶段 ——————————————————————
  Node2: 0x6083/00 (Profile Acceleration) = 50
  Node1: 0x6083/00 (Profile Acceleration) = 300
  Node3: 0x6083/00 (Profile Acceleration) = 500
  Node3: NMT 特殊命令 (0x80 00 00 00 21...)
  Node3: 0x6084/00 (Profile Deceleration) = 500

t=1.000s  — Node1 参数调整 —————————————————————
  Node1: 0x6084/00 (Profile Deceleration) = 100
  Node1: 0x6083/00 (Profile Acceleration) = 100
  Node1: NMT 特殊命令
  Node1: 0x6084/00 = 100

t=20.000s — 参数恢复 —————————————————————————
  Node1: 0x6083/00 = 300
  Node1: 0x6084/00 = 300
  Node1: 0x6083/00 = 300
```

### CANopen 对象字典关键索引

| 对象 | 名称 | 写入值 | 含义 |
|------|------|--------|------|
| 0x6083 | Profile Acceleration | 50/100/300/500 | 加速度 (单位取决于驱动器) |
| 0x6084 | Profile Deceleration | 100/300/500 | 减速度 |

---

## 三、RPDO2 — 运动命令流分析

### 数据格式（每个节点 8 字节）

```
Byte 0-3: 位置命令低 32bit (signed int32)
Byte 4-7: 速度/控制字 32bit
```

### 各节点命令值

| 节点 | COB-ID | 位置命令 (bytes 0-3) | 控制字 (bytes 4-7) | 帧率 |
|------|--------|---------------------|-------------------|------|
| 1 | 0x301 | **0x0001F4 (500)** | 0x00000000 | ~320Hz |
| 2 | 0x302 | **0x00014D55 (85333)** | 0x00000000 | ~190Hz |
| 3 | 0x303 | 变化 (0x0ADC→0xFFFFFFED) | 0x00055355 | ~193Hz |
| 4 | 0x304 | **0xFFFF82FD (-32003)** | 0x00019000 | ~189Hz |

**关键发现**：
- Node1/X 位置恒定为 500（针头旋转基准/回零位置）
- Node2/Y 位置恒定为 85333（转盘基准位置）
- Node3/Z 位置在变化 → 上下轴校验动作
- Node4/E 位置为负值 → 柱塞泵校验行程

### 帧间隔：约 150µs ~ 320µs

这意味着 CAN 总线以 **1 Mbps** 运行，MCU 需要极高的实时性来处理 RPDO2 发送。

---

## 四、TPDO1 — 位置反馈分析

### Node6 (0x186): 4 字节位置反馈

```
完整序列 (32bit little-endian):
  t=0.000s  0x000001B7  (439)     初始位置
  t=1.000s  0x000059E8  (23016)   快速跳变!
  t=2.000s  0x000059DC  (23004)
  t=4.000s  0x00004673  (18035)
  t=6.000s  0x00005A79  (23161)   达到峰值
  t=7-10s   0x00005A3E~0x5A61     平台振荡
  t=11.000s 0x00000144  (324)     跳变到低位
  t=11.000s 0x00001312  (4882)    再次跳变
  t=12-24s  0x00005A1E~0x5A57     稳定在 0x5Axx 附近
  
解码: 0x5A00 ≈ 23040 → 对应转盘位置 23040 编码单位
```

### Node8 (0x188): 8 字节双通道反馈

```
格式: [int32_le ch1][int32_le ch2]

  t=1.000s  ch1=0xFFFFFFD8 (-40)   ch2=0xFFFFFEE9 (-279)
  t=2.000s  ch1=0xFFFFFFAD (-83)   ch2=0xFFFFFDD1 (-559)
  ...       ch1≈0xFFFFFFA0 (-96)   ch2≈0xFFFFFD6F (-657)
  
两个通道都是负值，缓慢向零漂移。
可能是编码器偏移或零位校准过程。
```

### Node1-4 TPDO1: 2 字节状态

```
值: 0x0000 ↔ 0x0004 交替
含义: bit 2 (0x0004) = 目标到达 / 就绪标志
      bit 0 (0x0000) = 运动中
```

---

## 五、时间线总览

```
t=0.000s  ┌── Heartbeat + NMT 启动 ──────────────────┐
          │  Node2→Node1→Node3 发送心跳              │
          │  0x000=01 01 (Start Node1)               │
          │  0x000=01 04 (Start Node4)               │
          │  0x000=01 07 (Start Node7)               │
          ├── SDO 参数配置 ──────────────────────────┤
t=0.001s  │  各节点写 0x6083/0x6084 (加减速度)        │
t=1.000s  │  Node1 参数微调                           │
          ├── 闲置阶段 ──────────────────────────────┤
1-10s     │  所有 RPDO2 发送零值                     │
          │  TPDO1 反馈在变化 (可能是残余运动/滤波)    │
          │  Node6 位置在 0x1B7~0x5A61 之间震荡       │
          ├── 运动开始 ──────────────────────────────┤
11-12s    │  Node3 RPDO2 位置从 0x0ADC 开始变化       │
          │  Node6 TPDO1 突然跳变 (回到零点附近)       │
          │  0x206 (RPDO1 Node6) 从 0x00→0x18        │
          ├── 运动执行 ──────────────────────────────┤
12-24s    │  Node3 RPDO2 位置持续变化 (0x0ADC→0xFFFFED)│
          │  Node6 TPDO1 稳定在 0x5Axx               │
          │  Node8 TPDO1 持续漂移                     │
t=20s     │  Node1 SDO 参数恢复 (加速度 100→300)      │
24s       └── 数据截断 ──────────────────────────────┘
```

---

## 六、Modbus 寄存器 → CANopen 映射

基于两次采集的对比分析：

| Modbus 寄存器 | 功能 | CANopen 对应 | 说明 |
|:---:|------|-------------|------|
| 0x0000~0x0002 | 孔位编码 | **Node6 TPDO1 (0x186)** | 4 字节位置 → 6 字节孔位编码 |
| 0x0004 | 动作控制 | **Node1-4 RPDO2 命令** | 上位机写→MCU 转 CAN 命令 |
| 0x0005 | 电机状态 | **Node1-4 TPDO1** | 0x2000(运动中)↔bit0=0 |
| 0x0008 | 步数参数 | **0x6083/0x6084 SDO** | 加速度/减速度 |
| 0x0009 | 模式 | **RPDO2 控制字[4:7]** | 速度/模式选择 |
| 0x000B | 电机控制 | **NMT + RPDO2** | 0x9000(启)→RPDO2 开始发, 0x8000(停)→RPDO2 清零 |
| 0x0063 | 温度/传感器1 | **Node8 TPDO1 ch1** | 需要确认映射 |
| 0x0064 | 温度/传感器2 | **Node8 TPDO1 ch2** | 需要确认映射 |
| 0x00BF | 位置反馈 | **Node6 TPDO1 或 Node8** | 实际位置值 |

### 关键映射逻辑

```
上位机写 Modbus                      MCU 转换 CANopen
─────────────────                    ─────────────────
写 0x000B = 0x9000          →       写 Node3 RPDO2 使能位=1
写 0x0008 = 0x0BB8 (步数)   →       写 Node3 SDO 0x6083=3000
写 0x0009 = 0x002F (模式)   →       写 Node3 RPDO2 控制字

读 0x0000~0x0002            ←       读 Node6 TPDO1 位置
                                    转为孔位编码 (6字节格式)

读 0x0005 (电机状态)        ←       读 Node1-4 TPDO1 状态字
                                    转为 0x2000/0x0000
```

---

## 七、MCU 代码关键约束

### 7.1 CAN 总线时序

```
RPDO2 发送间隔: 150µs ~ 320µs
CAN 波特率: 1 Mbps
一帧 CAN (8字节数据): ~130µs @ 1Mbps
连续发送 4 帧 RPDO2: ~600µs

MCU 必须在 ~5ms 内完成:
  1. 接收 CAN 反馈 (TPDO1)
  2. 处理 Modbus 请求
  3. 计算下一周期的 RPDO2 命令
  4. 发送 RPDO2
```

### 7.2 代码结构建议

```c
// CANopen 节点定义
#define NODE_AXIS_X     1  // 0x301 RPDO2, 0x181 TPDO1: 针头旋转
#define NODE_AXIS_Y     2  // 0x302 RPDO2, 0x182 TPDO1: 转盘
#define NODE_AXIS_Z     3  // 0x303 RPDO2, 0x183 TPDO1: 上下
#define NODE_AXIS_E     4  // 0x304 RPDO2, 0x184 TPDO1: 柱塞泵
#define NODE_ENCODER    6  // 0x186 TPDO1 (位置反馈)
#define NODE_SENSOR     8  // 0x188 TPDO1 (双通道传感器)

// RPDO2 8字节映射结构
typedef struct {
    int32_t  target_position;   // bytes 0-3
    uint32_t control_word;      // bytes 4-7
} rpdo2_cmd_t;

// TPDO1 Node6 位置解码 → Modbus 孔位编码
void encode_hole_position(int32_t pos, uint16_t regs[3]) {
    regs[0] = (uint16_t)(pos >> 16);           // 孔位索引
    regs[1] = (micro_step(pos) << 8) | 0x00;   // 微步编码
    regs[2] = calc_checksum(pos);               // CRC
}

// 电机控制流程
void motor_control(uint16_t cmd) {
    switch (cmd) {
    case 0x9000:  // 启动
        can_send_rpdo2(NODE_MOTOR_C, target_pos, speed);
        break;
    case 0x8000:  // 停止
        can_send_rpdo2(NODE_MOTOR_C, 0, 0);
        break;
    case 0xC000:  // 回零
        can_send_nmt_homing(NODE_MOTOR_C);
        break;
    }
}
```

### 7.3 实时性要求

```
任务                   周期      优先级
────────────────────────────────────────
CAN 接收中断           实时      最高
RPDO2 发送 (运动控制)   ~300µs    高
Modbus 帧处理           < 25ms    中
传感器数据更新          ~1ms      中
孔位编码更新            ~30ms     中
```

---

## 八、上电校验流程总结

```
Phase 0 (t=0s):    心跳检测 + NMT 启动 Node 1,4,7
Phase 1 (t=1ms):   SDO 配置加减速度参数
Phase 2 (t=1-10s): 闲置期 — 所有 RPDO2=0，TPDO 反馈稳定
Phase 3 (t=11s):   运动开始 — RPDO2 Node3 开始变化
Phase 4 (t=12-24s): 运动执行 — 位置持续变化, Node6 反馈变化
Phase 5 (t=20s):   参数恢复

总时长: 24 秒 (采集截断，实际可能更长)
```

---

## 九、下一步

- [ ] 采集完整的 Modbus + CAN 同步数据（一次正常检测流程）
- [ ] 确认传感器寄存器 (0x0063/0x0064) 与 CAN Node8 的映射
- [ ] 验证孔位编码公式与 CAN 位置值的换算关系
- [ ] 确认 0x00BF 寄存器是否对应 Node6 的实际位置
- [ ] CAN 采集应包括发送和接收两个方向（本数据只记录了接收）

---

*分析完成: 2026-07-04*
*CAN 数据: 24900 帧, 30 个 COB-ID*
*Modbus 数据: 4582 帧, 跨越 4 个主要 Phase*
