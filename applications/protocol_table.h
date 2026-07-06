/*
 * protocol_table.h - 快检设备 Modbus 协议表
 */
#ifndef __PROTOCOL_TABLE_H__
#define __PROTOCOL_TABLE_H__

#include <stdint.h>

typedef struct {
    uint16_t reg_addr;   /* 0 起始 Modbus 帧地址 */
    void    *od_var;     /* 对应的对象字典变量 */
    uint8_t  reg_words;  /* 占用的 Modbus 寄存器数量 */
    uint8_t  od_bytes;   /* 对象字典变量字节数: 1, 2, 4 */
} od_sync_t;

typedef struct {
    uint16_t addr_start;
    uint16_t addr_end;
    const char *name;
    const char *desc;
    uint8_t access;      /* R=只读, W=只写, B=读写 */
    uint8_t bytes;       /* 协议数据字节数 */
} reg_def_t;

const od_sync_t *protocol_od_sync_table(uint16_t *count);
const reg_def_t *protocol_reg_def_table(uint16_t *count);
const reg_def_t *protocol_reg_def_find(uint16_t addr);

#endif /* __PROTOCOL_TABLE_H__ */
