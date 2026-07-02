/*
 * CANopen Master + Modbus Slave Gateway
 * Modbus UART2: FC03 (read) / FC06 (write single)
 * Modbus write → reg_router → OD variable → PDO auto-send
 *
 * CANopen config: ObjDict.c / ObjDict.h (objdictgen-generated).
 * Modbus → OD mapping: reg_router.c.
 * No runtime OD patching — OD is the single source of truth.
 */
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"
#include "reg_router.h"

#define POLY     0xA001
#define SLAVE    CONFIG_MODBUS_SLAVE_ADDR

static uint16_t crc16(uint8_t *d, int len) {
    uint16_t c = 0xFFFF;
    for (int i = 0; i < len; i++) { c ^= d[i];
        for (int j = 0; j < 8; j++) c = (c >> 1) ^ ((c & 1) ? POLY : 0); }
    return c;
}

int main(void) {
    rt_device_t u2 = rt_device_find("uart2");
    rt_device_open(u2, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

    reg_router_init();
    canopen_master_init();

    rt_kprintf("\nSystem ready. Modbus addr=%d on UART2\n", SLAVE);
    rt_kprintf("FC03/FC06 → reg_router → OD variables → PDO auto-send\n\n");

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
                        uint8_t rsp[128]; int rlen = 0;
                        if (buf[1]==0x03) {
                            uint16_t a=(buf[2]<<8)|buf[3], c=(buf[4]<<8)|buf[5];
                            int nb = reg_read(a, c, &rsp[3]);
                            if (nb > 0) {
                                rsp[0]=SLAVE; rsp[1]=0x03; rsp[2]=nb; rlen=3+nb;
                            } else {
                                rsp[0]=SLAVE; rsp[1]=0x83; rsp[2]=2; rlen=3;
                            }
                        } else if (buf[1]==0x06) {
                            uint16_t a=(buf[2]<<8)|buf[3];
                            if (reg_write(a, 1, &buf[4]) == 0) {
                                rlen=chk-2; memcpy(rsp,buf,rlen);
                            } else {
                                rsp[0]=SLAVE; rsp[1]=0x86; rsp[2]=2; rlen=3;
                            }
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
