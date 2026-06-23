#ifndef __REG_ROUTER_H__
#define __REG_ROUTER_H__

#include <stdint.h>

/* Access flags */
#define REG_RO  0x01
#define REG_WO  0x02
#define REG_RW  0x03

/* CANopen data types (matching CANfestival defines) */
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
    uint8_t  data_type;
} reg_route_entry_t;

void reg_router_init(void);
const reg_route_entry_t *reg_lookup(uint16_t addr);
int  reg_read(uint16_t start_addr, uint8_t count, uint8_t *out_buf);
int  reg_write(uint16_t start_addr, uint8_t count, const uint8_t *data);
int  reg_write_async(uint16_t start_addr, uint8_t count,
                     const uint8_t *data, void (*done)(int result));

#endif /* __REG_ROUTER_H__ */
