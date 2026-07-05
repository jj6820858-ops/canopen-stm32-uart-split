/*
 * reg_router.c — Modbus Register Map per 快检设备交互协议20250609
 *
 * Architecture:
 *   regs[200] local buffer ←→ OD variables (bidirectional sync)
 *
 *   Modbus write (FC06): regs[] → od_sync_out() → OD var → TPDO → slave
 *   Slave response:       RPDO → OD var → od_sync_in() → regs[]
 *
 *   Address convention: 0-based Modbus frame address
 *   (holding register N → frame addr = N-1)
 *
 * OD variable → Modbus register mapping:
 *   X axis (mX_*) = 针/泵操作 (Needle/Pump)
 *   Y axis (mY_*) = 蠕动泵 (Peristaltic pump)
 *   Z axis (mZ_*) = (reserved for future axis)
 *   E axis (mE_*) = 升降臂 (Lift/Elevation)
 *   T axis (mT_*) = 转盘 (Turntable)
 */

#include "reg_router.h"
#include "sampling.h"
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"

#define DBG_TAG "router"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ═══════════════════════════════════════════════════════════════
 * Holding Register File (200 × UNS16)
 * ═══════════════════════════════════════════════════════════════ */
static UNS16 regs[REGS_COUNT];

/* ═══════════════════════════════════════════════════════════════
 * OD Sync Table — registers that mirror OD variables for PDO
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    uint16_t reg_addr;  /* 0-based Modbus frame address */
    void    *od_var;    /* pointer to OD variable */
    uint8_t  od_bytes;  /* OD var size: 1, 2, or 4 */
} od_sync_t;

static const od_sync_t od_sync[] = {
    /* ── X轴: 针/泵操作 (TPDO1/TPDO2 mapped) ── */
    {  3, &mX_modes,          1 },  /* 00004  清洗针头 */
    {  4, &mX_control_word,   2 },  /* 00005  取液/注液/清洗 */
    {  5, &mX_position,       4 },  /* 00006~00007  柱塞泵液量 */

    /* ── X轴速度: 蠕动泵转速 ── */
    {  7, &mX_velocity,       4 },  /* 00008  蠕动泵转速 → mX_velocity */
    /* ── Y轴: 蠕动泵 ── */
    {  8, &mY_position,       4 },  /* 00009  蠕动泵圈数 */

    /* ── T轴: 转盘 (TPDO5 mapped) ── */
    { 10, &mT_modes,          1 },  /* 00011  转盘功能使能 */
    { 11, &mT_position,       4 },  /* 00012~00013  转盘位置 (lo 32-bit of 48) */

    /* ── 制冷 ── */
    { 14, &refrigeration_target, 1 }, /* 00015  酶孔位制冷 (UNS8) */

    /* ── 光强 (RPDO6 mapped) ── */
    { 15, &photometer_ch0,    2 },  /* 00016  光强 ch0 (lo 16) */
    { 16, &photometer_ch1,    2 },  /* 00017  光强 ch1 (hi 16) */

    /* ── 实时数据 (RPDO7/RPDO8/RPDO9 mapped) ── */
    { 99, &current_heating,     2 }, /* 0100  转盘加热温度 */
    {100, &current_refrigeration,2 }, /* 0101  酶试剂制冷温度 */
    {101, &weight_clean_water,  4 }, /* 0102  清水瓶重量 */
    {102, &weight_buff_liq,     4 }, /* 0103  缓冲液瓶重量 */
    {103, &weight_waste_liq,    4 }, /* 0104  废液瓶重量 */

    /* ── 温度控制 (TPDO11/TPDO12 mapped) ── */
    /* Modbus protocol each uses 1 register (2 bytes), OD stores as UNS32.
       Use od_bytes=2 to avoid 4-byte overlap (addr 191-192 vs 192-193). */
    {191, &heating_target,       2 }, /* 00192  加热目标温度 (×100,LE) */
    {192, &refrigeration_target, 2 }, /* 00193  制冷目标温度 (×100,LE) */

    /* ── 升降臂: E轴 (TPDO3 mapped) ── */
    /* 0164~0171 depth values → mE_position, mE_velocity handled in app layer */

    /* ── 加样臂: X轴 angle params ── */
    /* 0172~0179 angle values → mX_* handled in app layer */

    /* ── 状态字 (RPDO receive) ── */
    { 17, &mX_status_word,    2 },  /* 00018 lo  仪器状态 → X status */
    { 19, &mT_status_word,    2 },  /* 00020 is where equipment status starts, just map key ones */

    /* Terminator */
    {0, NULL, 0}
};

/* ═══════════════════════════════════════════════════════════════
 * Register name table (for debug / motor_stat display)
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    uint16_t addr_start;
    uint16_t addr_end;
    const char *name;
    const char *desc;
    uint8_t  access;   /* 'R','W','B' */
    uint8_t  bytes;    /* 1,2,4,6 */
} reg_def_t;

static const reg_def_t reg_defs[] = {
    /* 功能/状态区 */
    {  0,  2, "孔位状态",       "bitmap:试剂+样品+比色皿+水箱",   'R',6},
    {  3,  3, "清洗针头",       "0排空气1洗内2洗外3洗内外4排液5保养",'B',1},
    {  4,  4, "取液注液清洗",   "0停1取液2加液3混合4洗皿5蠕取6蠕加",'B',1},
    {  5,  6, "柱塞泵液量",     "×1000整数,LE",                   'B',4},
    {  7,  7, "蠕动泵转速",     "转速",                           'B',2},
    {  8,  8, "蠕动泵圈数",     "圈数",                           'B',2},
    {  9,  9, "操作位号",       "bitmap:试剂+缓冲液+样品+比色皿+洗针",'B',2},
    { 10, 10, "转盘功能使能",   "加热/震荡/读光/旋转/归位",       'B',1},
    { 11, 13, "转盘指定位号",   "bitmap:试剂+样品+比色皿位号",     'B',6},
    { 14, 14, "酶孔位制冷",     "0关1开",                         'B',1},
    { 15, 16, "光强读数",       "×1000整数,LE",                   'R',4},
    { 17, 18, "仪器状态",       "bitmap:水箱+试剂+针+温控+原点等", 'R',4},
    { 19, 19, "清水箱异常码",   "0正常1水满",                     'R',1},
    { 20, 20, "废水箱异常码",   "0正常1异常",                     'R',1},
    { 21, 21, "缓冲液异常码",   "0正常1异常",                     'R',1},
    { 22, 22, "试剂异常码",     "bitmap:洗针位+试剂1~5",          'R',1},
    { 23, 24, "样品异常码",     "bitmap:样品瓶1~18",              'R',2},
    { 25, 26, "比色皿异常码",   "bitmap:比色瓶1~18",              'R',2},
    { 27, 29, "针头异常码",     "bitmap:试剂+样品+比色+水+缓冲+洗针",'R',3},
    { 30, 30, "温控异常码",     "bit0加热 bit1制冷",              'R',1},
    { 31, 32, "比色皿脏污码",   "bitmap:比色瓶1~18",              'R',2},
    /* 实时数据区 */
    { 99, 99, "转盘加热温度",   "×100",                           'B',2},
    {100,100, "酶试剂制冷温度", "×100",                           'B',2},
    {101,101, "清水瓶重量",     "克",                             'R',2},
    {102,102, "缓冲液瓶重量",   "克",                             'R',2},
    {103,103, "废液瓶重量",     "克",                             'R',2},
    /* 参数设置区 */
    {119,119, "参数控制字",     "0存1深2角3转4洗针5洗皿6混7震8热9冷...",'B',2},
    {120,120, "称重控制",       "bitmap:标定+去皮+容量",           'B',1},
    {129,129, "机械臂测试深度",  "",                               'B',2},
    {130,130, "机械臂测试角度",  "",                               'B',2},
    {131,131, "转盘测试角度",    "",                               'B',2},
    {149,149, "样品盘启停速度",  "0-1000",                         'B',2},
    {150,150, "反应盘启停速度",  "0-1000",                         'B',2},
    {151,151, "旋转臂启停速度",  "0-1000",                         'B',2},
    {152,152, "升降臂启停速度",  "0-1000",                         'B',2},
    {153,153, "柱塞泵启停速度",  "0-1000",                         'B',2},
    {154,154, "样品盘运转速度",  "0-3000",                         'B',2},
    {155,155, "反应盘运转速度",  "0-3000",                         'B',2},
    {156,156, "旋转臂运转速度",  "0-3000",                         'B',2},
    {157,157, "升降臂运转速度",  "0-3000",                         'B',2},
    {158,158, "清水蠕动泵速度",  "0-3000",                         'B',2},
    {159,159, "废液蠕动泵速度",  "0-3000",                         'B',2},
    {160,160, "比色皿废液泵速度","0-3000",                         'B',2},
    {161,161, "试剂蠕动泵速度",  "0-3000",                         'B',2},
    {162,162, "柱塞泵运转速度",  "0-3000",                         'B',2},
    {163,163, "升降臂深-洗针",   "0.1mm",                          'B',2},
    {164,164, "升降臂深-洗皿",   "0.1mm",                          'B',2},
    {165,165, "升降臂深-取试剂", "0.1mm",                          'B',2},
    {166,166, "升降臂深-取样品", "0.1mm",                          'B',2},
    {167,167, "升降臂深-加缓冲液","0.1mm",                         'B',2},
    {168,168, "升降臂深-加样品", "0.1mm",                          'B',2},
    {169,169, "升降臂深-加酶",   "0.1mm",                          'B',2},
    {170,170, "升降臂深-加试剂", "0.1mm",                          'B',2},
    {171,171, "加样臂角-洗针位", "0-8000",                         'B',2},
    {172,172, "加样臂角-样品位", "0-8000",                         'B',2},
    {173,173, "加样臂角-比色皿", "0-8000",                         'B',2},
    {174,174, "加样臂角-酶试剂", "0-8000",                         'B',2},
    {175,175, "加样臂角-试剂1",  "0-8000",                         'B',2},
    {176,176, "加样臂角-试剂2",  "0-8000",                         'B',2},
    {177,177, "加样臂角-试剂3",  "0-8000",                         'B',2},
    {178,178, "加样臂角-试剂4",  "0-8000",                         'B',2},
    {179,179, "转盘角-样品间距",  "",                               'B',2},
    {180,180, "转盘角-比色皿间距","",                               'B',2},
    {181,181, "转盘角-首样品位",  "",                               'B',2},
    {182,182, "转盘角-首比色皿位","",                               'B',2},
    {183,183, "清洗-清水速度",    "0-1000",                         'B',2},
    {184,184, "清洗-针圈数",      "",                               'B',2},
    {185,185, "清洗-排空圈数",    "",                               'B',2},
    {186,186, "清洗-皿抽废液时间","100ms",                          'B',2},
    {187,187, "清洗-皿圈数",      "",                               'B',2},
    {188,188, "清洗-皿次数",      "",                               'B',2},
    {189,189, "搅拌-次数",        "",                               'B',2},
    {190,190, "搅拌-吞吐量",      "",                               'B',2},
    {191,191, "加热目标温度",     "×100,LE",                        'B',2},
    {192,192, "制冷目标温度",     "×100,LE",                        'B',2},
    {193,193, "震荡-幅度",        "0-4000",                         'B',2},
    {194,194, "震荡-速度",        "0-3000rpm",                      'B',2},
    {195,195, "震荡-变频",        "",                               'B',2},
    {196,196, "柱塞泵-取液量",    "μL",                             'B',2},
    {197,197, "柱塞泵-吐液量",    "μL",                             'B',2},
};

/* ═══════════════════════════════════════════════════════════════
 *  OD → regs[]  sync helper (read OD var, pack into regs)
 * ═══════════════════════════════════════════════════════════════ */
static void od_to_regs(const od_sync_t *s)
{
    uint32_t v32; uint16_t v16; uint8_t v8;
    switch (s->od_bytes) {
    case 1:
        v8 = *(UNS8*)s->od_var;
        regs[s->reg_addr] = v8;
        break;
    case 2:
        v16 = *(UNS16*)s->od_var;
        regs[s->reg_addr] = v16;
        break;
    case 4:
        v32 = *(UNS32*)s->od_var;
        regs[s->reg_addr]     = (UNS16)(v32 >> 16);       /* hi 16 */
        regs[s->reg_addr + 1] = (UNS16)(v32 & 0xFFFF);    /* lo 16 */
        break;
    }
}

/* regs[] → OD var (Modbus write path) */
static void regs_to_od(const od_sync_t *s)
{
    uint32_t v32;
    switch (s->od_bytes) {
    case 1:
        *(UNS8*)s->od_var  = (UNS8)regs[s->reg_addr];
        break;
    case 2:
        *(UNS16*)s->od_var = regs[s->reg_addr];
        break;
    case 4:
        v32 = ((uint32_t)regs[s->reg_addr] << 16) | regs[s->reg_addr + 1];
        *(UNS32*)s->od_var = v32;
        break;
    }
}

/* Find OD sync entry for a register address, or NULL */
static const od_sync_t *od_sync_find(uint16_t addr)
{
    for (const od_sync_t *s = od_sync; s->od_var; s++) {
        uint8_t n = (s->od_bytes + 1) / 2;  /* 1→1, 2→1, 4→2 */
        if (addr >= s->reg_addr && addr < s->reg_addr + n)
            return s;
    }
    return NULL;
}

/* ═══════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════ */

void reg_router_init(void)
{
    memset(regs, 0, sizeof(regs));
    LOG_I("Router: %d registers, %d OD-synced",
          REGS_COUNT, (int)(sizeof(od_sync)/sizeof(od_sync[0]) - 1));
}

/* OD vars → regs[]  (call from CANopen post_sync / post_TPDO) */
void reg_od_sync_in(void)
{
    for (const od_sync_t *s = od_sync; s->od_var; s++)
        od_to_regs(s);
}

/* regs[] → OD vars  (call from reg_write for synced addresses) */
void reg_od_sync_out(void)
{
    for (const od_sync_t *s = od_sync; s->od_var; s++)
        regs_to_od(s);
}

/* Read Modbus registers into big-endian byte buffer */
int reg_read(uint16_t addr, uint8_t count, uint8_t *out)
{
    if (addr + count > REGS_COUNT) return -1;
    for (uint8_t i = 0; i < count; i++) {
        UNS16 v = regs[addr + i];
        out[i * 2]     = (uint8_t)(v >> 8);
        out[i * 2 + 1] = (uint8_t)(v & 0xFF);
    }
    return count * 2;
}

/* Write Modbus registers from big-endian byte buffer */
int reg_write(uint16_t addr, uint8_t count, const uint8_t *data)
{
    if (addr + count > REGS_COUNT) return -1;
    for (uint8_t i = 0; i < count; i++) {
        UNS16 v = ((UNS16)data[i * 2] << 8) | data[i * 2 + 1];
        regs[addr + i] = v;
    }
    /* Sync to OD variables for mapped addresses */
    for (uint8_t i = 0; i < count; i++) {
        const od_sync_t *s = od_sync_find(addr + i);
        if (s) regs_to_od(s);
    }
    /* Notify sampling module of register writes */
    for (uint8_t i = 0; i < count; i++) {
        sampling_on_reg_write(addr + i, regs[addr + i]);
    }

    LOG_D("reg_write addr=%d count=%d", addr, count);
    return 0;
}

int reg_write_async(uint16_t addr, uint8_t count,
                    const uint8_t *data, void (*cb)(int result))
{
    int r = reg_write(addr, count, data);
    if (cb) cb(r);
    return r;
}

/* ═══════════════════════════════════════════════════════════════
 *  Debug: dump register
 * ═══════════════════════════════════════════════════════════════ */
#ifdef RT_USING_FINSH
#include <finsh.h>

static int reg(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: reg <addr>        — read one Modbus register\n");
        rt_kprintf("       reg <addr> <val>  — write one Modbus register\n");
        rt_kprintf("       reg list           — show all defined registers\n");
        rt_kprintf("  addr = Modbus register number (1-based, 00001~00198)\n");
        return 0;
    }
    if (strcmp(argv[1], "list") == 0) {
        rt_kprintf("%6s %-20s %s %s\n", "Addr", "Name", "A", "Bytes");
        rt_kprintf("------ -------------------- - -----\n");
        for (int i = 0; i < (int)(sizeof(reg_defs)/sizeof(reg_defs[0])); i++) {
            const reg_def_t *d = &reg_defs[i];
            rt_kprintf("%5d %-20s %c %d\n",
                       d->addr_start + 1, d->name, d->access, d->bytes);
        }
        return 0;
    }
    uint16_t rn = (uint16_t)atoi(argv[1]);  /* 1-based register number */
    if (rn < 1 || rn > REGS_COUNT) { rt_kprintf("addr out of range\n"); return -1; }
    uint16_t a = rn - 1;
    if (argc >= 3) {
        /* Write */
        UNS16 v = (UNS16)strtoul(argv[2], NULL, 0);
        regs[a] = v;
        const od_sync_t *s = od_sync_find(a);
        if (s) regs_to_od(s);
    }
    /* Read back */
    const reg_def_t *def = NULL;
    for (int i = 0; i < (int)(sizeof(reg_defs)/sizeof(reg_defs[0])); i++) {
        if (a >= reg_defs[i].addr_start && a <= reg_defs[i].addr_end)
            { def = &reg_defs[i]; break; }
    }
    const od_sync_t *s = od_sync_find(a);
    rt_kprintf("[%05d] %s = 0x%04X (%d)",
               rn, def ? def->name : "?", regs[a], regs[a]);
    if (s) {
        rt_kprintf("  OD synced (%d bytes)", s->od_bytes);
    }
    rt_kprintf("\n");
    return 0;
}
MSH_CMD_EXPORT(reg, Modbus register read/write);
#endif /* RT_USING_FINSH */
