#include "reg_router.h"
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"

#define DBG_TAG "router"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* SDO timeout: max wait per transfer (ms) */
#define APP_SDO_TIMEOUT_MS      CONFIG_SDO_TIMEOUT_MS
#define APP_SDO_POLL_INTERVAL   CONFIG_SDO_POLL_INTERVAL_MS

/* ── Complete routing table ──────────────────────── */

static const reg_route_entry_t g_routes[] = {
    /* ═══ Control & Status (0x2000~0x201F) ═══ */

    { 0x0001, 0x0003, 0x02, 0x2000, 0x00, REG_RO,  DTTYPE_UNS64 }, /* 孔位状态 */
    { 0x0004, 0x0004, 0x02, 0x2001, 0x00, REG_RW,  DTTYPE_UNS8  }, /* 清洗针头 */
    { 0x0005, 0x0005, 0x02, 0x2002, 0x00, REG_RW,  DTTYPE_UNS8  }, /* 取液/注液/清洗 */
    { 0x0006, 0x0007, 0x02, 0x2003, 0x00, REG_RW,  DTTYPE_UNS32 }, /* 柱塞泵液量 */
    { 0x0008, 0x0008, 0x02, 0x2004, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 蠕动泵转速 */
    { 0x0009, 0x0009, 0x02, 0x2005, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 蠕动泵圈数 */
    { 0x000A, 0x000A, 0x02, 0x2006, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 操作位号 */
    { 0x000B, 0x000B, 0x02, 0x2007, 0x00, REG_RW,  DTTYPE_UNS8  }, /* 转盘使能 */
    { 0x000C, 0x000E, 0x02, 0x2008, 0x00, REG_RW,  DTTYPE_UNS64 }, /* 转盘指定位号 */
    { 0x000F, 0x000F, 0x02, 0x2009, 0x00, REG_RW,  DTTYPE_UNS8  }, /* 制冷使能 */
    { 0x0010, 0x0011, 0x02, 0x200A, 0x00, REG_RO,  DTTYPE_UNS32 }, /* 光强读数 */
    { 0x0012, 0x0013, 0x02, 0x200B, 0x00, REG_RO,  DTTYPE_UNS32 }, /* 仪器状态 */
    { 0x0014, 0x0014, 0x02, 0x200C, 0x00, REG_RO,  DTTYPE_UNS8  }, /* 清水箱异常 */
    { 0x0015, 0x0015, 0x02, 0x200D, 0x00, REG_RO,  DTTYPE_UNS8  }, /* 废水箱异常 */
    { 0x0016, 0x0016, 0x02, 0x200E, 0x00, REG_RO,  DTTYPE_UNS8  }, /* 缓冲液异常 */
    { 0x0017, 0x0017, 0x02, 0x200F, 0x00, REG_RO,  DTTYPE_UNS8  }, /* 试剂异常 */
    { 0x0018, 0x0019, 0x02, 0x2010, 0x00, REG_RO,  DTTYPE_UNS16 }, /* 样品异常 */
    { 0x001A, 0x001B, 0x02, 0x2011, 0x00, REG_RO,  DTTYPE_UNS16 }, /* 比色皿异常 */
    { 0x001C, 0x001E, 0x02, 0x2012, 0x00, REG_RO,  DTTYPE_UNS32 }, /* 针头异常 */
    { 0x001F, 0x001F, 0x02, 0x2013, 0x00, REG_RO,  DTTYPE_UNS8  }, /* 温控异常 */
    { 0x0020, 0x0021, 0x02, 0x2014, 0x00, REG_RO,  DTTYPE_UNS16 }, /* 比色皿脏污 */

    /* ═══ Real-time Data (0x2100~0x2104) ═══ */

    { 0x0064, 0x0064, 0x02, 0x2100, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 转盘加热温度 */
    { 0x0065, 0x0065, 0x02, 0x2101, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 制冷温度 */
    { 0x0066, 0x0066, 0x02, 0x2102, 0x00, REG_RO,  DTTYPE_UNS16 }, /* 清水瓶重量 */
    { 0x0067, 0x0067, 0x02, 0x2103, 0x00, REG_RO,  DTTYPE_UNS16 }, /* 缓冲液瓶重量 */
    { 0x0068, 0x0068, 0x02, 0x2104, 0x00, REG_RO,  DTTYPE_UNS16 }, /* 废液瓶重量 */

    /* ═══ Parameter Settings (0x2200~0x224E) ═══ */

    { 0x0078, 0x0078, 0x02, 0x2200, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 参数控制字 */
    { 0x0079, 0x0079, 0x02, 0x2201, 0x00, REG_RW,  DTTYPE_UNS8  }, /* 称重控制 */
    { 0x0082, 0x0082, 0x02, 0x2202, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 机械臂深度 */
    { 0x0083, 0x0083, 0x02, 0x2203, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 机械臂角度 */
    { 0x0084, 0x0084, 0x02, 0x2204, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 转盘角度 */

    /* 启停速度 (0x2210~0x2214) */
    { 0x0096, 0x0096, 0x02, 0x2210, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 样品盘启停速度 */
    { 0x0097, 0x0097, 0x02, 0x2211, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 反应盘启停速度 */
    { 0x0098, 0x0098, 0x02, 0x2212, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 旋转臂启停速度 */
    { 0x0099, 0x0099, 0x02, 0x2213, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 升降臂启停速度 */
    { 0x009A, 0x009A, 0x02, 0x2214, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 柱塞泵启停速度 */

    /* 运转速度 (0x2215~0x221D) */
    { 0x009B, 0x009B, 0x02, 0x2215, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 样品盘运转速度 */
    { 0x009C, 0x009C, 0x02, 0x2216, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 反应盘运转速度 */
    { 0x009D, 0x009D, 0x02, 0x2217, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 旋转臂运转速度 */
    { 0x009E, 0x009E, 0x02, 0x2218, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 升降臂运转速度 */
    { 0x009F, 0x009F, 0x02, 0x2219, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 清水蠕动泵运转速度 */
    { 0x00A0, 0x00A0, 0x02, 0x221A, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 废液蠕动泵运转速度 */
    { 0x00A1, 0x00A1, 0x02, 0x221B, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 比色皿废液蠕动泵 */
    { 0x00A2, 0x00A2, 0x02, 0x221C, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 试剂蠕动泵运转速度 */
    { 0x00A3, 0x00A3, 0x02, 0x221D, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 柱塞泵运转速度 */

    /* 升降臂下降深度 (0x221E~0x2225) */
    { 0x00A4, 0x00A4, 0x02, 0x221E, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 洗针深度 */
    { 0x00A5, 0x00A5, 0x02, 0x221F, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 洗比色皿深度 */
    { 0x00A6, 0x00A6, 0x02, 0x2220, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 取试剂深度 */
    { 0x00A7, 0x00A7, 0x02, 0x2221, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 取样品深度 */
    { 0x00A8, 0x00A8, 0x02, 0x2222, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 加缓冲液到样品 */
    { 0x00A9, 0x00A9, 0x02, 0x2223, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 加样品到比色皿 */
    { 0x00AA, 0x00AA, 0x02, 0x2224, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 加酶试剂到比色皿 */
    { 0x00AB, 0x00AB, 0x02, 0x2225, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 加其他试剂到比色皿 */

    /* 加样臂角度 (0x2226~0x222D) */
    { 0x00AC, 0x00AC, 0x02, 0x2226, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 洗针位角度 */
    { 0x00AD, 0x00AD, 0x02, 0x2227, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 样品位角度 */
    { 0x00AE, 0x00AE, 0x02, 0x2228, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 比色皿位角度 */
    { 0x00AF, 0x00AF, 0x02, 0x2229, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 酶试剂位角度 */
    { 0x00B0, 0x00B0, 0x02, 0x222A, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 试剂1角度 */
    { 0x00B1, 0x00B1, 0x02, 0x222B, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 试剂2角度 */
    { 0x00B2, 0x00B2, 0x02, 0x222C, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 试剂3角度 */
    { 0x00B3, 0x00B3, 0x02, 0x222D, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 试剂4角度 */

    /* 转盘角度 (0x222E~0x2231) */
    { 0x00B4, 0x00B4, 0x02, 0x222E, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 样品间距 */
    { 0x00B5, 0x00B5, 0x02, 0x222F, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 比色皿间距 */
    { 0x00B6, 0x00B6, 0x02, 0x2230, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 首位样品位置 */
    { 0x00B7, 0x00B7, 0x02, 0x2231, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 首位比色皿位置 */

    /* 清洗 (0x2232~0x2237) */
    { 0x00B8, 0x00B8, 0x02, 0x2232, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 清水清洗速度 */
    { 0x00B9, 0x00B9, 0x02, 0x2233, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 清洗针圈数 */
    { 0x00BA, 0x00BA, 0x02, 0x2234, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 针排空圈数 */
    { 0x00BB, 0x00BB, 0x02, 0x2235, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 比色皿抽废液时间 */
    { 0x00BC, 0x00BC, 0x02, 0x2236, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 清洗比色皿圈数 */
    { 0x00BD, 0x00BD, 0x02, 0x2237, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 清洗比色皿次数 */

    /* 搅拌混合 (0x2238~0x2239) */
    { 0x00BE, 0x00BE, 0x02, 0x2238, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 搅拌混合次数 */
    { 0x00BF, 0x00BF, 0x02, 0x2239, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 搅拌混合吞吐量 */

    /* 温度控制 (0x223A~0x223B) */
    { 0x00C0, 0x00C0, 0x02, 0x223A, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 转盘加热控制温度 */
    { 0x00C1, 0x00C1, 0x02, 0x223B, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 酶试剂制冷控制温度 */

    /* 震荡 (0x223C~0x223E) */
    { 0x00C2, 0x00C2, 0x02, 0x223C, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 震荡幅度 */
    { 0x00C3, 0x00C3, 0x02, 0x223D, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 震荡速度 */
    { 0x00C4, 0x00C4, 0x02, 0x223E, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 震荡变频 */

    /* 柱塞泵 (0x223F~0x2240) */
    { 0x00C5, 0x00C5, 0x02, 0x223F, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 柱塞泵取液量 */
    { 0x00C6, 0x00C6, 0x02, 0x2240, 0x00, REG_RW,  DTTYPE_UNS16 }, /* 柱塞泵吐液量 */

    /* End marker */
    { 0x0000, 0x0000, 0x00, 0x0000, 0x00, 0,      0             },
};

#define ROUTE_COUNT \
    ((sizeof(g_routes) / sizeof(g_routes[0])) - 1)  /* exclude end marker */

/* ── API ────────────────────────────────────────── */

void reg_router_init(void)
{
    int ro = 0, rw = 0;
    for (int i = 0; i < ROUTE_COUNT; i++) {
        if (g_routes[i].access & REG_RO) ro++;
        if (g_routes[i].access & REG_WO) rw++;
    }
    LOG_I("Router: %d entries (%d read, %d write)", ROUTE_COUNT, ro, rw);
}

const reg_route_entry_t *reg_lookup(uint16_t addr)
{
    /* Binary search on sorted non-overlapping routing table */
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
            LOG_E("Write-only register: 0x%04X", addr);
            return -1;
        }

        UNS8 size = type_size(e->data_type);

        /* ── Same OD entry as previous? Reuse cached data ── */
        if (last_entry && e == last_entry) {
            memcpy(out_buf + total_bytes, last_data, last_size);
            total_bytes += last_size;
            continue;
        }

        /* ── Start SDO read ── */
        UNS32 abortCode;
        UNS8 rc = readNetworkDict(&CanOpenMaster_Data, e->node_id,
                                  e->od_index, e->od_subindex,
                                  e->data_type, 0);
        if (rc) {
            LOG_E("SDO read failed: addr=0x%04X, rc=%d", addr, rc);
            return -1;
        }

        /* ── Poll for completion (responsive + bounded timeout) ── */
        int poll_cnt = APP_SDO_TIMEOUT_MS / APP_SDO_POLL_INTERVAL;
        UNS32 got_size = size;
        UNS8 sdo_ok = 0;
        while (poll_cnt--) {
            if (getReadResultNetworkDict(&CanOpenMaster_Data, e->node_id,
                                         out_buf + total_bytes,
                                         &got_size, &abortCode) == SDO_FINISHED) {
                sdo_ok = 1;
                break;
            }
            rt_thread_mdelay(APP_SDO_POLL_INTERVAL);
        }
        closeSDOtransfer(&CanOpenMaster_Data, e->node_id, SDO_CLIENT);

        if (!sdo_ok) {
            LOG_E("SDO read timeout: 0x%04X (abort=0x%04lX)",
                  addr, (unsigned long)abortCode);
            return -1;
        }

        /* ── Cache result for contiguous same-entry addresses ── */
        last_entry = e;
        last_size = (uint8_t)got_size;
        memcpy(last_data, out_buf + total_bytes, got_size);

        total_bytes += got_size;
    }
    return total_bytes;
}

/* ── Single SDO write (helper, used by reg_write batch flush) ── */
static int do_sdo_write(const reg_route_entry_t *e,
                        const uint8_t *data, int len)
{
    UNS32 abortCode;
    UNS8 rc = writeNetworkDict(&CanOpenMaster_Data, e->node_id,
                               e->od_index, e->od_subindex,
                               len, e->data_type,
                               (void *)data, 0);
    if (rc) {
        LOG_E("SDO write failed: OD=0x%04X sub=0x%02X rc=%d",
              e->od_index, e->od_subindex, rc);
        return -1;
    }

    int poll_cnt = APP_SDO_TIMEOUT_MS / APP_SDO_POLL_INTERVAL;
    UNS8 sdo_ok = 0;
    while (poll_cnt--) {
        if (getWriteResultNetworkDict(&CanOpenMaster_Data, e->node_id,
                                      &abortCode) == SDO_FINISHED) {
            sdo_ok = 1;
            break;
        }
        rt_thread_mdelay(APP_SDO_POLL_INTERVAL);
    }
    closeSDOtransfer(&CanOpenMaster_Data, e->node_id, SDO_CLIENT);

    if (!sdo_ok) {
        LOG_E("SDO write timeout: OD=0x%04X (abort=0x%04lX)",
              e->od_index, (unsigned long)abortCode);
        return -1;
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════════
 * Async SDO write — non-blocking, callback-chained batch
 *
 * Pre-batches consecutive same-OD entries, kicks off SDOs via
 * writeNetworkDictCallBack. The completion callback chains the
 * next batch until all are done, then calls user's done().
 *
 * Only one async write can be in-flight at a time (static context).
 * ══════════════════════════════════════════════════════════════════ */

#define ASYNC_MAX_BATCH 8

/* Max time to wait for an async SDO batch to complete before giving up.
 * Without a slave on the bus, SDO callbacks never fire — this timeout
 * prevents g_aw from being stuck forever. */
#define ASYNC_SDO_TIMEOUT_TICKS  (RT_TICK_PER_SECOND * 2)  /* 2 s */

typedef struct {
    const reg_route_entry_t *entry[ASYNC_MAX_BATCH];
    uint8_t data[ASYNC_MAX_BATCH][8];
    uint8_t len[ASYNC_MAX_BATCH];
    uint8_t count;
    uint8_t current;
    void (*done)(int result);
    int result;
    rt_tick_t start_tick;       /* tick when current batch was kicked off */
    uint8_t active;             /* 1 = SDO chain in flight */
} async_write_t;

static async_write_t g_aw;

/* Reset g_aw to idle (done callback already dispatched by caller) */
static void async_write_reset(void)
{
    g_aw.count = 0;
    g_aw.current = 0;
    g_aw.result = 0;
    g_aw.done = NULL;
    g_aw.active = 0;
}

/* Helper: force-close a pending SDO transfer for the current batch's node.
 * Safe to call even if no transfer is actually pending. */
static void async_write_abort(void)
{
    if (!g_aw.active)
        return;
    UNS8 node_id = g_aw.entry[g_aw.current]->node_id;
    closeSDOtransfer(&CanOpenMaster_Data, node_id, SDO_CLIENT);
    g_aw.active = 0;
}

static void async_write_cb(CO_Data *d, UNS8 nodeId)
{
    (void)d; (void)nodeId;

    /* Guard: if the transfer was aborted (e.g. timeout or cancelled by a
     * new write), ignore stale callback. */
    if (!g_aw.active)
        return;

    UNS32 abortCode;
    if (getWriteResultNetworkDict(d, nodeId, &abortCode) != SDO_FINISHED) {
        LOG_E("Async SDO write fail: batch %d abort=0x%04lX",
              g_aw.current, (unsigned long)abortCode);
        g_aw.result = -1;
    }
    closeSDOtransfer(d, nodeId, SDO_CLIENT);

    g_aw.current++;
    if (g_aw.current >= g_aw.count) {
        void (*cb)(int) = g_aw.done;
        int r = g_aw.result;
        async_write_reset();
        if (cb) cb(r);
        return;
    }

    /* Chain next batch */
    g_aw.start_tick = rt_tick_get();
    writeNetworkDictCallBack(d, nodeId,
        g_aw.entry[g_aw.current]->od_index,
        g_aw.entry[g_aw.current]->od_subindex,
        g_aw.len[g_aw.current],
        g_aw.entry[g_aw.current]->data_type,
        g_aw.data[g_aw.current],
        async_write_cb, 0);
}

int reg_write_async(uint16_t start_addr, uint8_t count,
                    const uint8_t *data, void (*done)(int result))
{
    if (g_aw.active) {
        rt_tick_t elapsed = rt_tick_get() - g_aw.start_tick;
        if (elapsed < ASYNC_SDO_TIMEOUT_TICKS) {
            /* Previous SDO still in-flight and hasn't timed out.
             * Data is already in the local buffer — SDO sync is
             * best-effort.  Skip silently to avoid log spam and
             * keep start_tick stable so the timeout can actually fire. */
            return 0;
        }
        /* Timed out — slave likely offline.  Log once and abort. */
        LOG_W("Async SDO timed out (slave offline), aborting old chain");
        async_write_abort();
        if (g_aw.done) {
            g_aw.done(-1);
        }
    }

    /* Phase 1: pre-batch */
    const reg_route_entry_t *batch_entry = NULL;
    int offset = 0;
    g_aw.count = 0;
    g_aw.current = 0;
    g_aw.result = 0;
    g_aw.done = done;

    for (uint8_t i = 0; i < count; i++) {
        uint16_t addr = start_addr + i;
        const reg_route_entry_t *e = reg_lookup(addr);
        if (!e || !(e->access & REG_WO)) {
            LOG_E("Async write: invalid addr 0x%04X", addr);
            if (done) done(-1);
            return -1;
        }

        UNS8 size = type_size(e->data_type);

        if (batch_entry && e == batch_entry) {
            /* Same entry: accumulate */
            memcpy(g_aw.data[g_aw.count - 1] + g_aw.len[g_aw.count - 1],
                   data + offset, size);
            g_aw.len[g_aw.count - 1] += size;
        } else {
            /* New entry: start a new batch slot */
            if (g_aw.count >= ASYNC_MAX_BATCH) {
                LOG_E("Async write: too many batches (%d max)", ASYNC_MAX_BATCH);
                if (done) done(-1);
                return -1;
            }
            g_aw.entry[g_aw.count] = e;
            memcpy(g_aw.data[g_aw.count], data + offset, size);
            g_aw.len[g_aw.count] = size;
            g_aw.count++;
            batch_entry = e;
        }
        offset += size;
    }

    if (g_aw.count == 0) {
        if (done) done(0);
        return 0;
    }

    /* Phase 2: kick off first batch */
    g_aw.start_tick = rt_tick_get();
    g_aw.active = 1;
    writeNetworkDictCallBack(&CanOpenMaster_Data,
        g_aw.entry[0]->node_id,
        g_aw.entry[0]->od_index,
        g_aw.entry[0]->od_subindex,
        g_aw.len[0],
        g_aw.entry[0]->data_type,
        g_aw.data[0],
        async_write_cb, 0);

    return 0;
}

int reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data)
{
    const reg_route_entry_t *batch_entry = NULL;
    uint8_t batch_data[8] = {0};          /* Max CANopen OD byte count */
    int batch_len = 0;
    int offset = 0;

    for (uint8_t i = 0; i < count; i++) {
        uint16_t addr = start_addr + i;
        const reg_route_entry_t *e = reg_lookup(addr);

        if (!e) {
            LOG_E("Route not found: 0x%04X", addr);
            return -1;
        }
        if (!(e->access & REG_WO)) {
            LOG_E("Read-only register: 0x%04X", addr);
            return -1;
        }

        UNS8 size = type_size(e->data_type);

        /* ── Same OD entry as current batch? Accumulate ── */
        if (batch_entry && e == batch_entry) {
            memcpy(batch_data + batch_len, data + offset, size);
            batch_len += size;
            offset += size;
            continue;
        }

        /* ── Flush previous batch ── */
        if (batch_entry && batch_len > 0) {
            if (do_sdo_write(batch_entry, batch_data, batch_len) != 0)
                return -1;
        }

        /* ── Start new batch ── */
        batch_entry = e;
        memcpy(batch_data, data + offset, size);
        batch_len = size;
        offset += size;
    }

    /* ── Flush final batch ── */
    if (batch_entry && batch_len > 0) {
        if (do_sdo_write(batch_entry, batch_data, batch_len) != 0)
            return -1;
    }

    return 0;
}
