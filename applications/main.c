/*
 * CANopen Master + Manual Modbus Slave
 * Modbus UART2: FC03(读) / FC06(写单个)
 * Modbus写→变量→PDO自动发送
 */
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"

#define POLY 0xA001
#define SLAVE 1
#define REGS 200
static uint16_t regs[REGS] = {0};

/* ObjDict variables (PDO映射对象, 赋值后PDO引擎自动发送) */
extern UNS8  CanOpenMaster_obj2001; /* mX_modes          reg4  */
extern UNS32 CanOpenMaster_obj2002; /* mX_position       reg164 */
extern UNS32 CanOpenMaster_obj2003; /* mX_velocity       reg153 */
extern UNS16 CanOpenMaster_obj2004; /* mX_status_word    reg28  */
extern UNS16 CanOpenMaster_obj2005; /* mX_control_word   reg5   */
extern UNS16 CanOpenMaster_obj201A; /* photometer_ch0    reg16  */
extern UNS16 CanOpenMaster_obj201B; /* photometer_ch1    reg17  */

static uint16_t crc16(uint8_t *d, int len) {
    uint16_t c = 0xFFFF;
    for (int i = 0; i < len; i++) { c ^= d[i];
        for (int j = 0; j < 8; j++) c = (c >> 1) ^ ((c & 1) ? POLY : 0); }
    return c;
}

/* 路由表: Modbus地址→OD变量 */
typedef struct { uint16_t addr; void *var; int size; } mb_route_t;
static const mb_route_t routes[] = {
    {3, &CanOpenMaster_obj2001, 1},   /* reg 4  mX_modes */
    {4, &CanOpenMaster_obj2005, 2},   /* reg 5  mX_control_word */
    {27, &CanOpenMaster_obj2004, 2},  /* reg 28 mX_status_word */
    {152, &CanOpenMaster_obj2003, 4}, /* reg 153 mX_velocity */
    {163, &CanOpenMaster_obj2002, 4}, /* reg 164 mX_position */
    {15, &CanOpenMaster_obj201A, 2},  /* reg 16 photometer_ch0 */
    {16, &CanOpenMaster_obj201B, 2},  /* reg 17 photometer_ch1 */
};
#define NROUTES (sizeof(routes)/sizeof(routes[0]))

static void mb_write_od(uint16_t mb_addr, uint16_t val) {
    for (int i = 0; i < NROUTES; i++) {
        if (routes[i].addr == mb_addr) {
            if (routes[i].size == 1)       *(uint8_t*)routes[i].var  = (uint8_t)val;
            else if (routes[i].size == 2)  *(uint16_t*)routes[i].var = val;
            else if (routes[i].size == 4)  *(uint32_t*)routes[i].var = val;
            rt_kprintf("[OD] reg%d=%d\n", mb_addr + 1, val);
            return;
        }
    }
    /* 不在路由表里的写本地缓冲区 */
    if (mb_addr < REGS) regs[mb_addr] = val;
}

int main(void) {
    rt_device_t u2 = rt_device_find("uart2");
    rt_device_open(u2, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

    reg_router_init();
    canopen_master_init();

    rt_kprintf("\nSystem ready. Modbus addr=%d on UART2\n", SLAVE);
    rt_kprintf("FC06 writes → OD variables → PDO auto-send\n\n");

    uint8_t buf[256];
    int pos = 0, idle = 0;

    while (1) {
        int n = rt_device_read(u2, 0, buf + pos, sizeof(buf) - pos);
        if (n > 0) { pos += n; idle = 0; }
        else {
            idle++;
            if (idle > 8 && pos > 0) {
                int done = 0;
                for (int chk = pos; chk >= 4 && !done; chk--) {
                    if (buf[0]==SLAVE && crc16(buf,chk-2)==((uint16_t)buf[chk-2]|(buf[chk-1]<<8))) {
                        uint8_t rsp[64]; int rlen = 0;
                        if (buf[1]==0x03) {
                            uint16_t a=(buf[2]<<8)|buf[3], c=(buf[4]<<8)|buf[5];
                            rsp[0]=SLAVE; rsp[1]=0x03; rsp[2]=c*2;
                            for(int i=0;i<c&&a+i<REGS;i++){rsp[3+i*2]=regs[a+i]>>8;rsp[3+i*2+1]=regs[a+i]&0xFF;}
                            rlen=3+c*2;
                        } else if (buf[1]==0x06) {
                            uint16_t a=(buf[2]<<8)|buf[3], v=(buf[4]<<8)|buf[5];
                            mb_write_od(a, v);
                            rlen=chk-2; memcpy(rsp,buf,rlen);
                        } else {
                            rsp[0]=SLAVE; rsp[1]=buf[1]|0x80; rsp[2]=1; rlen=3;
                        }
                        uint16_t cr=crc16(rsp,rlen); rsp[rlen]=cr&0xFF; rsp[rlen+1]=cr>>8;
                        rt_device_write(u2,0,rsp,rlen+2);
                        if(chk<pos)memmove(buf,buf+chk,pos-chk); pos-=chk; done=1;
                    }
                }
                if(!done&&pos>64)pos=0;
            }
        }
        rt_thread_mdelay(5);
    }
}
