/*
 * reg_router.c — Modbus ↔ CANopen (Master Local OD) routing table
 *
 * Maps the 快检设备 Modbus protocol (Excel: 快检设备交互协议20250609.xlsx)
 * to CANopen Master local Object Dictionary (Master.od).
 *
 * Access: local OD via getODentry / setODentry (big-endian, matches Modbus).
 * Registers not listed in g_routes[] use the Modbus local buffer fallback.
 */
#include "reg_router.h"
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"
#include "objacces.h"        /* getODentry / setODentry */

#define DBG_TAG "router"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ── Routing table: Modbus register → Master.od local index ────
 *
 * All 43 Master.od user objects (0x2001-0x202B) mapped to Modbus.
 * Sorted by Modbus address for binary search.
 * Access: local OD via getODentry/setODentry.
 */
static const reg_route_entry_t g_routes[] = {

    /* ═══════════════════════════════════════════════════════════════
     * Control & Status (Modbus 1-33, Excel protocol)
     * ═══════════════════════════════════════════════════════════════ */

    /* 4: 清洗针头 → mX_modes (INT8) */
    { 0x0004, 0x0004, 0x00, 0x2001, 0x00, REG_RW, DTTYPE_UNS8  },
    /* 5: 取液/注液/清洗 → mX_control_word (UINT16) */
    { 0x0005, 0x0005, 0x00, 0x2005, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 6-7: 柱塞泵液量 → mT_position (INT32) */
    { 0x0006, 0x0007, 0x00, 0x2016, 0x00, REG_RW, DTTYPE_UNS32 },
    /* 8: 蠕动泵转速 → mE_velocity (INT32, low 16 bits) */
    { 0x0008, 0x0008, 0x00, 0x2012, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 9: 蠕动泵圈数 → mE_position (INT32, low 16 bits) */
    { 0x0009, 0x0009, 0x00, 0x2011, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 10: 操作位号 → mY_modes (INT8) */
    { 0x000A, 0x000A, 0x00, 0x2006, 0x00, REG_RW, DTTYPE_UNS8  },
    /* 11: 转盘使能 → mZ_control_word (UINT16) */
    { 0x000B, 0x000B, 0x00, 0x200F, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 12-13: 转盘指定位号 → mZ_position (INT32) */
    { 0x000C, 0x000D, 0x00, 0x200C, 0x00, REG_RW, DTTYPE_UNS32 },
    /* 15: 制冷使能 → mE_modes (INT8) */
    { 0x000F, 0x000F, 0x00, 0x2010, 0x00, REG_RW, DTTYPE_UNS8  },
    /* 16-17: 光强读数 ch0+ch1 → photometer_ch0+ch1 (UINT16 x2) */
    { 0x0010, 0x0010, 0x00, 0x201A, 0x00, REG_RO, DTTYPE_UNS16 },
    { 0x0011, 0x0011, 0x00, 0x201B, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 18: 仪器状态 (low 16 bits) → mY_status_word (UINT16) */
    { 0x0012, 0x0012, 0x00, 0x2009, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 20: 清水箱异常 → mE_status_word (UINT16) */
    { 0x0014, 0x0014, 0x00, 0x2013, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 21: 废水箱异常 → mT_status_word (UINT16) */
    { 0x0015, 0x0015, 0x00, 0x2018, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 28: 针头异常 (low 16 bits) → mX_status_word (UINT16) */
    { 0x001C, 0x001C, 0x00, 0x2004, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 29: 针头异常 (mid 16 bits) → mY_control_word (UINT16) */
    { 0x001D, 0x001D, 0x00, 0x200A, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 31: 温控异常 → TEMP_status_word low16 (UINT16) */
    { 0x001F, 0x001F, 0x00, 0x2024, 0x00, REG_RO, DTTYPE_UNS16 },

    /* ═══════════════════════════════════════════════════════════════
     * Real-time Data (Modbus 100-104, Excel protocol)
     * ═══════════════════════════════════════════════════════════════ */

    /* 100: 转盘加热温度 → current_heating low16 (UINT32→UINT16) */
    { 0x0064, 0x0064, 0x00, 0x2020, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 101: 酶试剂制冷温度 → current_refrigeration low16 (UINT32→UINT16) */
    { 0x0065, 0x0065, 0x00, 0x2021, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 102: 清水瓶重量 → weight_clean_water low16 (INT32→UINT16) */
    { 0x0066, 0x0066, 0x00, 0x2025, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 103: 缓冲液瓶重量 → weight_buff_liq low16 (INT32→UINT16) */
    { 0x0067, 0x0067, 0x00, 0x2026, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 104: 废液瓶重量 → weight_waste_liq low16 (INT32→UINT16) */
    { 0x0068, 0x0068, 0x00, 0x2027, 0x00, REG_RO, DTTYPE_UNS16 },

    /* ═══════════════════════════════════════════════════════════════
     * Parameters → Axis position/velocity/control (Excel protocol)
     * ═══════════════════════════════════════════════════════════════ */

    /* 120: 参数控制字 → mT_modes (INT8) */
    { 0x0078, 0x0078, 0x00, 0x2015, 0x00, REG_RW, DTTYPE_UNS8  },
    /* 153: 升降臂启停速度 → mX_velocity low16 (INT32→UINT16) */
    { 0x0099, 0x0099, 0x00, 0x2003, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 156: 反应盘运转速度 → mZ_velocity low16 (INT32→UINT16) */
    { 0x009C, 0x009C, 0x00, 0x200D, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 157: 旋转臂运转速度 → mY_velocity low16 (INT32→UINT16) */
    { 0x009D, 0x009D, 0x00, 0x2008, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 159: 清水蠕动泵运转速度 → mE_control_word (UINT16) */
    { 0x009F, 0x009F, 0x00, 0x2014, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 163: 柱塞泵运转速度 → mT_velocity low16 (INT32→UINT16) */
    { 0x00A3, 0x00A3, 0x00, 0x2017, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 164-165: 洗针深度 → mX_position (INT32) */
    { 0x00A4, 0x00A5, 0x00, 0x2002, 0x00, REG_RW, DTTYPE_UNS32 },
    /* 172-173: 洗针位角度 → mY_position (INT32) */
    { 0x00AC, 0x00AD, 0x00, 0x2007, 0x00, REG_RW, DTTYPE_UNS32 },
    /* 174-175: 比色皿位角度 → mZ_modes (using position slot for Z mode) */
    { 0x00AE, 0x00AE, 0x00, 0x200B, 0x00, REG_RW, DTTYPE_UNS8  },
    /* 192: 控制加热温度 → heating_target low16 (UINT32→UINT16) */
    { 0x00C0, 0x00C0, 0x00, 0x201E, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 193: 控制制冷温度 → refrigeration_target low16 (UINT32→UINT16) */
    { 0x00C1, 0x00C1, 0x00, 0x201F, 0x00, REG_RW, DTTYPE_UNS16 },

    /* ═══════════════════════════════════════════════════════════════
     * Extended registers (199-218) for remaining Master.od objects
     * ═══════════════════════════════════════════════════════════════ */

    /* 199-200: photometer_led (UINT32) */
    { 0x00C7, 0x00C8, 0x00, 0x201C, 0x00, REG_RW, DTTYPE_UNS32 },
    /* 201: photometer_rate (UINT16) */
    { 0x00C9, 0x00C9, 0x00, 0x2022, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 202: photometer_gain (UINT8) */
    { 0x00CA, 0x00CA, 0x00, 0x2023, 0x00, REG_RW, DTTYPE_UNS8  },
    /* 203: mZ_status_word (UINT16) */
    { 0x00CB, 0x00CB, 0x00, 0x200E, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 204: mT_control_word (UINT16) */
    { 0x00CC, 0x00CC, 0x00, 0x2019, 0x00, REG_RW, DTTYPE_UNS16 },
    /* 205: mE_control_word already at 159 — skip */
    /* 208-209: TEMP_control_word full 32 bits */
    { 0x00D0, 0x00D1, 0x00, 0x201D, 0x00, REG_RW, DTTYPE_UNS32 },
    /* 210: mX_Current_actual (INT16) */
    { 0x00D2, 0x00D2, 0x00, 0x2028, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 211: mY_Current_actual (INT16) */
    { 0x00D3, 0x00D3, 0x00, 0x2029, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 212: mZ_Current_actual (INT16) */
    { 0x00D4, 0x00D4, 0x00, 0x202A, 0x00, REG_RO, DTTYPE_UNS16 },
    /* 213: mB_Current_actual (INT16) */
    { 0x00D5, 0x00D5, 0x00, 0x202B, 0x00, REG_RO, DTTYPE_UNS16 },

    /* End marker */
    { 0x0000, 0x0000, 0x00, 0x0000, 0x00, 0,      0             },
};

#define ROUTE_COUNT \
    ((sizeof(g_routes) / sizeof(g_routes[0])) - 1)  /* exclude end marker */

/* ── Init ────────────────────────────────────────── */

void reg_router_init(void)
{
    int ro = 0, rw = 0;
    for (int i = 0; i < ROUTE_COUNT; i++) {
        if (g_routes[i].access & REG_RO) ro++;
        if (g_routes[i].access & REG_WO) rw++;
    }
    LOG_I("Router: %d routes (%d RO, %d RW) — local OD", ROUTE_COUNT, ro, rw);
}

/* ── Lookup ──────────────────────────────────────── */

const reg_route_entry_t *reg_lookup(uint16_t addr)
{
    int lo = 0, hi = ROUTE_COUNT - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (addr < g_routes[mid].addr_start) {
            hi = mid - 1;
        } else if (addr > g_routes[mid].addr_end) {
            lo = mid + 1;
        } else {
            return &g_routes[mid];
        }
    }
    return NULL;
}

/* ── Helpers ─────────────────────────────────────── */

static uint8_t type_size(uint8_t dtype)
{
    switch (dtype) {
        case DTTYPE_UNS8:  return 1;
        case DTTYPE_UNS16: return 2;
        case DTTYPE_UNS32: return 4;
        default:           return 2;
    }
}

/* ── Read: local OD getODentry (big-endian, matches Modbus) ── */

int reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf)
{
    int total_bytes = 0;
    const reg_route_entry_t *last_entry = NULL;
    uint8_t last_data[8] = {0};
    uint8_t last_size = 0;

    for (uint8_t i = 0; i < count; i++) {
        uint16_t addr = start_addr + i;
        const reg_route_entry_t *e = reg_lookup(addr);

        if (!e) {
            LOG_E("Route not found: 0x%04X", addr);
            return -1;
        }
        if (!(e->access & REG_RO)) {
            LOG_E("Write-only: 0x%04X", addr);
            return -1;
        }

        UNS8 size = type_size(e->data_type);

        /* Reuse cached data for same OD entry (multi-register reads) */
        if (last_entry && e == last_entry) {
            memcpy(out_buf + total_bytes, last_data, last_size);
            total_bytes += last_size;
            continue;
        }

        /* Read from local OD (big-endian → matches Modbus wire format).
         * For UINT8 (1-byte), value goes to low byte of 16-bit register. */
        UNS32 rsize = size;
        UNS8  dtype = 0;
        uint8_t od_buf[4];
        UNS32 ret = getODentry(&CanOpenMaster_Data, e->od_index,
                               e->od_subindex,
                               od_buf, &rsize, &dtype, 0);
        if (ret != OD_SUCCESSFUL) {
            LOG_E("OD read fail: idx=0x%04X sub=%d ret=0x%lX",
                  e->od_index, e->od_subindex, (unsigned long)ret);
            return -1;
        }

        if (size == 1) {
            /* UINT8/INT8: pad to 16-bit register (value in low byte) */
            out_buf[total_bytes]     = 0;
            out_buf[total_bytes + 1] = od_buf[0];
            last_data[0] = 0;
            last_data[1] = od_buf[0];
            last_size    = 2;
            total_bytes += 2;
        } else {
            memcpy(out_buf + total_bytes, od_buf, rsize);
            last_size = (uint8_t)rsize;
            memcpy(last_data, od_buf, rsize);
            total_bytes += rsize;
        }
        last_entry = e;
    }
    return total_bytes;
}

/* ── Write: local OD setODentry (de-endianizes from big-endian) ── */

int reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data)
{
    const reg_route_entry_t *batch_entry = NULL;
    uint8_t batch_data[8] = {0};
    int     batch_len = 0;
    int     offset = 0;

    for (uint8_t i = 0; i < count; i++) {
        uint16_t addr = start_addr + i;
        const reg_route_entry_t *e = reg_lookup(addr);

        if (!e) {
            LOG_E("Route not found: 0x%04X", addr);
            return -1;
        }
        if (!(e->access & REG_WO)) {
            LOG_E("Read-only: 0x%04X", addr);
            return -1;
        }

        UNS8 size = type_size(e->data_type);

        /* Accumulate consecutive same-OD writes.
         * For UINT8: value is in low byte of 16-bit Modbus register. */
        uint8_t val_byte;
        const uint8_t *src;
        if (size == 1) {
            val_byte = data[offset + 1];  /* low byte of Modbus register */
            src = &val_byte;
        } else {
            src = data + offset;
        }

        if (batch_entry && e == batch_entry) {
            memcpy(batch_data + batch_len, src, size);
            batch_len += size;
            offset += (size == 1) ? 2 : size;
            continue;
        }

        /* Flush previous batch */
        if (batch_entry && batch_len > 0) {
            UNS32 wsize = batch_len;
            UNS32 ret = setODentry(&CanOpenMaster_Data,
                                   batch_entry->od_index,
                                   batch_entry->od_subindex,
                                   batch_data, &wsize, 1);
            if (ret != OD_SUCCESSFUL) {
                LOG_E("OD write fail: idx=0x%04X ret=0x%lX",
                      batch_entry->od_index, (unsigned long)ret);
                return -1;
            }
        }

        batch_entry = e;
        memcpy(batch_data, src, size);
        batch_len = size;
        offset += (size == 1) ? 2 : size;
    }

    /* Flush final batch */
    if (batch_entry && batch_len > 0) {
        UNS32 wsize = batch_len;
        UNS32 ret = setODentry(&CanOpenMaster_Data,
                               batch_entry->od_index,
                               batch_entry->od_subindex,
                               batch_data, &wsize, 1);
        if (ret != OD_SUCCESSFUL) {
            LOG_E("OD write fail: idx=0x%04X ret=0x%lX",
                  batch_entry->od_index, (unsigned long)ret);
            return -1;
        }
    }

    return 0;
}

/* ── Async write: synchronous local OD write ────────
 *
 * With local OD access there is no SDO latency, so we simply
 * call reg_write() synchronously and invoke done() immediately.
 * Kept as a separate API for backward compatibility with user_mb_app.c.
 */

int reg_write_async(uint16_t start_addr, uint8_t count,
                    const uint8_t *data, void (*done)(int result))
{
    int ret = reg_write(start_addr, count, data);
    if (done) done(ret);
    return ret;
}
