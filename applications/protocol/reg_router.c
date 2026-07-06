/*
 * reg_router.c - Modbus 寄存器路由
 *
 * 设计约定:
 *   1. regs[] 是本地保持寄存器缓存;
 *   2. 上位机写 Modbus 后，同步到对象字典变量，再由 PDO 发到 CAN 从站;
 *   3. CAN 从站反馈更新对象字典变量后，再同步回 Modbus 寄存器;
 *   4. Modbus 帧地址使用 0-based，协议文档寄存器号 = 帧地址 + 1。
 */
#include "reg_router.h"

#include <rtthread.h>
#include <stdlib.h>
#include <string.h>

#include "canopen_master.h"
#include "protocol_table.h"
#include "sampling.h"

#define DBG_TAG "router"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static UNS16 regs[REGS_COUNT];

static void od_to_regs(const od_sync_t *sync)
{
    uint32_t value = 0;

    switch (sync->od_bytes) {
    case 1:
        value = *(UNS8 *)sync->od_var;
        break;
    case 2:
        value = *(UNS16 *)sync->od_var;
        break;
    case 4:
        value = *(UNS32 *)sync->od_var;
        break;
    default:
        return;
    }

    regs[sync->reg_addr] = (UNS16)(value & 0xFFFF);
    if (sync->reg_words >= 2) {
        regs[sync->reg_addr + 1] = (UNS16)(value >> 16);
    }
}

static void regs_to_od(const od_sync_t *sync)
{
    uint32_t value = regs[sync->reg_addr];

    if (sync->reg_words >= 2) {
        value |= ((uint32_t)regs[sync->reg_addr + 1] << 16);
    }

    switch (sync->od_bytes) {
    case 1:
        *(UNS8 *)sync->od_var = (UNS8)value;
        break;
    case 2:
        *(UNS16 *)sync->od_var = (UNS16)value;
        break;
    case 4:
        *(UNS32 *)sync->od_var = value;
        break;
    default:
        break;
    }
}

static const od_sync_t *od_sync_find(uint16_t addr)
{
    uint16_t count;
    const od_sync_t *table = protocol_od_sync_table(&count);

    for (uint16_t i = 0; i < count; i++) {
        const od_sync_t *sync = &table[i];
        uint16_t end = sync->reg_addr + sync->reg_words;

        if (addr >= sync->reg_addr && addr < end) {
            return sync;
        }
    }

    return RT_NULL;
}

void reg_router_init(void)
{
    uint16_t sync_count;

    memset(regs, 0, sizeof(regs));
    protocol_od_sync_table(&sync_count);

    LOG_I("Router: %d registers, %d OD-synced", REGS_COUNT, sync_count);
}

void reg_od_sync_in(void)
{
    uint16_t count;
    const od_sync_t *table = protocol_od_sync_table(&count);

    for (uint16_t i = 0; i < count; i++) {
        od_to_regs(&table[i]);
    }
}

void reg_od_sync_out(void)
{
    uint16_t count;
    const od_sync_t *table = protocol_od_sync_table(&count);

    for (uint16_t i = 0; i < count; i++) {
        regs_to_od(&table[i]);
    }
}

int reg_read(uint16_t addr, uint8_t count, uint8_t *out)
{
    if (addr + count > REGS_COUNT) {
        return -1;
    }

    for (uint8_t i = 0; i < count; i++) {
        UNS16 value = regs[addr + i];

        out[i * 2] = (uint8_t)(value >> 8);
        out[i * 2 + 1] = (uint8_t)(value & 0xFF);
    }

    return count * 2;
}

static int regs_write_only(uint16_t addr, uint8_t count, const uint8_t *data)
{
    if (addr + count > REGS_COUNT) {
        return -1;
    }

    for (uint8_t i = 0; i < count; i++) {
        UNS16 value = ((UNS16)data[i * 2] << 8) | data[i * 2 + 1];
        regs[addr + i] = value;
    }

    return 0;
}

int reg_write_local(uint16_t addr, uint8_t count, const uint8_t *data)
{
    return regs_write_only(addr, count, data);
}

int reg_write(uint16_t addr, uint8_t count, const uint8_t *data)
{
    if (regs_write_only(addr, count, data) != 0) {
        return -1;
    }

    for (uint8_t i = 0; i < count; i++) {
        const od_sync_t *sync = od_sync_find(addr + i);

        if (sync) {
            regs_to_od(sync);
        }
    }

    for (uint8_t i = 0; i < count; i++) {
        sampling_on_reg_write(addr + i, regs[addr + i]);
    }

    LOG_D("reg_write addr=%d count=%d", addr, count);
    return 0;
}

int reg_write_async(uint16_t addr, uint8_t count,
                    const uint8_t *data, void (*cb)(int result))
{
    int ret = reg_write(addr, count, data);

    if (cb) {
        cb(ret);
    }

    return ret;
}

#ifdef RT_USING_FINSH
#include <finsh.h>

static int reg(int argc, char **argv)
{
    uint16_t reg_no;
    uint16_t addr;
    uint8_t data[2];
    uint8_t out[2];
    const reg_def_t *def;
    const od_sync_t *sync;

    if (argc < 2) {
        rt_kprintf("用法: reg <寄存器号>\n");
        rt_kprintf("      reg <寄存器号> <值>\n");
        rt_kprintf("      reg list\n");
        rt_kprintf("说明: 寄存器号按协议文档填写，范围 00001~00198\n");
        return 0;
    }

    if (strcmp(argv[1], "list") == 0) {
        uint16_t count;
        const reg_def_t *table = protocol_reg_def_table(&count);

        rt_kprintf("%6s %-20s %s %s\n", "Addr", "Name", "A", "Bytes");
        rt_kprintf("------ -------------------- - -----\n");
        for (uint16_t i = 0; i < count; i++) {
            rt_kprintf("%5d %-20s %c %d\n",
                       table[i].addr_start + 1,
                       table[i].name,
                       table[i].access,
                       table[i].bytes);
        }

        return 0;
    }

    reg_no = (uint16_t)strtoul(argv[1], RT_NULL, 0);
    if (reg_no < 1 || reg_no > REGS_COUNT) {
        rt_kprintf("寄存器号超出范围\n");
        return -1;
    }

    addr = reg_no - 1;
    if (argc >= 3) {
        UNS16 value = (UNS16)strtoul(argv[2], RT_NULL, 0);

        data[0] = (uint8_t)(value >> 8);
        data[1] = (uint8_t)(value & 0xFF);
        reg_write(addr, 1, data);
    }

    reg_read(addr, 1, out);
    def = protocol_reg_def_find(addr);
    sync = od_sync_find(addr);

    rt_kprintf("[%05d] %s = 0x%04X (%d)",
               reg_no,
               def ? def->name : "?",
               ((UNS16)out[0] << 8) | out[1],
               ((UNS16)out[0] << 8) | out[1]);
    if (sync) {
        rt_kprintf("  已同步对象字典(%d字节)", sync->od_bytes);
    }
    rt_kprintf("\n");

    return 0;
}
MSH_CMD_EXPORT(reg, Modbus register read/write);
#endif /* RT_USING_FINSH */
