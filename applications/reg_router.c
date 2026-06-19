#include "reg_router.h"
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"

#define DBG_TAG "router"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* SDO timeout: max wait per transfer (ms) */
#define SDO_TIMEOUT_MS     500
#define SDO_POLL_INTERVAL  20

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
    for (int i = 0; i < ROUTE_COUNT; i++) {
        if (addr >= g_routes[i].addr_start &&
            addr <= g_routes[i].addr_end) {
            return &g_routes[i];
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
        int poll_cnt = SDO_TIMEOUT_MS / SDO_POLL_INTERVAL;
        UNS8 got_size = size;
        UNS8 sdo_ok = 0;
        while (poll_cnt--) {
            if (getReadResultNetworkDict(&CanOpenMaster_Data, e->node_id,
                                         out_buf + total_bytes,
                                         &got_size, &abortCode) == SDO_FINISHED) {
                sdo_ok = 1;
                break;
            }
            rt_thread_mdelay(SDO_POLL_INTERVAL);
        }
        closeSDOtransfer(&CanOpenMaster_Data, e->node_id, SDO_CLIENT);

        if (!sdo_ok) {
            LOG_E("SDO read timeout: 0x%04X (abort=0x%04lX)",
                  addr, (unsigned long)abortCode);
            return -1;
        }

        /* ── Cache result for contiguous same-entry addresses ── */
        last_entry = e;
        last_size = got_size;
        memcpy(last_data, out_buf + total_bytes, got_size);

        total_bytes += got_size;
    }
    return total_bytes;
}

int reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data)
{
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
        UNS32 abortCode;
        UNS8 rc = writeNetworkDict(&CanOpenMaster_Data, e->node_id,
                                   e->od_index, e->od_subindex,
                                   size, e->data_type,
                                   (void *)(data + offset), 0);
        if (rc) {
            LOG_E("SDO write failed: addr=0x%04X, rc=%d", addr, rc);
            return -1;
        }

        /* ── Poll for completion ── */
        int poll_cnt = SDO_TIMEOUT_MS / SDO_POLL_INTERVAL;
        UNS8 sdo_ok = 0;
        while (poll_cnt--) {
            if (getWriteResultNetworkDict(&CanOpenMaster_Data, e->node_id,
                                          &abortCode) == SDO_FINISHED) {
                sdo_ok = 1;
                break;
            }
            rt_thread_mdelay(SDO_POLL_INTERVAL);
        }
        closeSDOtransfer(&CanOpenMaster_Data, e->node_id, SDO_CLIENT);

        if (!sdo_ok) {
            LOG_E("SDO write timeout: 0x%04X (abort=0x%04lX)",
                  addr, (unsigned long)abortCode);
            return -1;
        }

        offset += size;
    }
    return 0;
}
