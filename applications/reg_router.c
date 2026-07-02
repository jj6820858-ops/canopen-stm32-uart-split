/*
 * reg_router.c — Modbus → CANopen direct variable access
 * Variables: CanOpenMaster_objXXXX (hand-written ObjDict.c)
 */
#include "reg_router.h"
#include <rtthread.h>
#include <string.h>
#include "canopen_master.h"

#define DBG_TAG "router"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ── Extern variables from ObjDict.c ── */
extern UNS8  mX_modes; extern UNS32 mX_position;
extern UNS32 mX_velocity; extern UNS16 mX_status_word;
extern UNS16 mX_control_word; extern UNS8  mY_modes;
extern UNS32 mY_position; extern UNS32 mY_velocity;
extern UNS16 mY_status_word; extern UNS16 mY_control_word;
extern UNS8  mZ_modes; extern UNS32 mZ_position;
extern UNS32 mZ_velocity; extern UNS16 mZ_status_word;
extern UNS16 mZ_control_word; extern UNS8  mE_modes;
extern UNS32 mE_position; extern UNS32 mE_velocity;
extern UNS16 mE_status_word; extern UNS16 mE_control_word;
extern UNS8  mT_modes; extern UNS32 mT_position;
extern UNS32 mT_velocity; extern UNS16 mT_status_word;
extern UNS16 mT_control_word; extern UNS16 photometer_ch0;
extern UNS16 photometer_ch1; extern UNS32 photometer_led;
extern UNS32 TEMP_control_word; extern UNS32 heating_target;
extern UNS32 refrigeration_target; extern UNS32 current_heating;
extern UNS32 current_refrigeration; extern UNS16 photometer_rate;
extern UNS8  photometer_gain; extern UNS32 TEMP_status_word;
extern UNS32 weight_clean_water; extern UNS32 weight_buff_liq;
extern UNS32 weight_waste_liq; extern UNS16 mX_Current_actual;
extern UNS16 mY_Current_actual; extern UNS16 mZ_Current_actual;
extern UNS16 mB_Current_actual;

typedef struct { uint16_t s,e; void *v; uint8_t z,a; } vr_t;
static const vr_t g_routes[] = {
    {0x0004,0x0004,&mX_modes, 1, REG_RW},
    {0x0005,0x0005,&mX_control_word, 2, REG_RW},
    {0x001C,0x001C,&mX_status_word, 2, REG_RO},
    {0x0099,0x0099,&mX_velocity, 2, REG_RW},
    {0x00A4,0x00A5,&mX_position, 4, REG_RW},
    {0x00D2,0x00D2,&mX_Current_actual, 2, REG_RO},
    {0x000A,0x000A,&mY_modes, 1, REG_RW},
    {0x001D,0x001D,&mY_control_word, 2, REG_RW},
    {0x0012,0x0012,&mY_status_word, 2, REG_RO},
    {0x009D,0x009D,&mY_velocity, 2, REG_RW},
    {0x00AC,0x00AD,&mY_position, 4, REG_RW},
    {0x00D3,0x00D3,&mY_Current_actual, 2, REG_RO},
    {0x000B,0x000B,&mZ_control_word, 2, REG_RW},
    {0x000C,0x000D,&mZ_position, 4, REG_RW},
    {0x00AE,0x00AE,&mZ_modes, 1, REG_RW},
    {0x009C,0x009C,&mZ_velocity, 2, REG_RW},
    {0x00CB,0x00CB,&mZ_status_word, 2, REG_RO},
    {0x00D4,0x00D4,&mZ_Current_actual, 2, REG_RO},
    {0x0008,0x0008,&mE_velocity, 2, REG_RW},
    {0x0009,0x0009,&mE_position, 2, REG_RW},
    {0x000F,0x000F,&mE_modes, 1, REG_RW},
    {0x0014,0x0014,&mE_status_word, 2, REG_RO},
    {0x009F,0x009F,&mE_control_word, 2, REG_RW},
    {0x0006,0x0007,&mT_position, 4, REG_RW},
    {0x0078,0x0078,&mT_modes, 1, REG_RW},
    {0x0015,0x0015,&mT_status_word, 2, REG_RO},
    {0x00A3,0x00A3,&mT_velocity, 2, REG_RW},
    {0x00CC,0x00CC,&mT_control_word, 2, REG_RW},
    {0x0010,0x0010,&photometer_ch0, 2, REG_RO},
    {0x0011,0x0011,&photometer_ch1, 2, REG_RO},
    {0x00C7,0x00C8,&photometer_led, 4, REG_RW},
    {0x00C9,0x00C9,&photometer_rate, 2, REG_RW},
    {0x00CA,0x00CA,&photometer_gain, 1, REG_RW},
    {0x00D0,0x00D1,&TEMP_control_word, 4, REG_RW},
    {0x00C0,0x00C0,&heating_target, 2, REG_RW},
    {0x00C1,0x00C1,&refrigeration_target, 2, REG_RW},
    {0x0064,0x0064,&current_heating, 2, REG_RW},
    {0x0065,0x0065,&current_refrigeration, 2, REG_RW},
    {0x001F,0x001F,&TEMP_status_word, 2, REG_RO},
    {0x0066,0x0066,&weight_clean_water, 2, REG_RO},
    {0x0067,0x0067,&weight_buff_liq, 2, REG_RO},
    {0x0068,0x0068,&weight_waste_liq, 2, REG_RO},
    {0x00D5,0x00D5,&mB_Current_actual, 2, REG_RO},
    {0,0,NULL,0,0}};
#define N ((sizeof(g_routes)/sizeof(g_routes[0]))-1)

void reg_router_init(void){
    int ro=0,rw=0;
    for(int i=0;i<N;i++){if(g_routes[i].a&REG_RO)ro++;if(g_routes[i].a&REG_WO)rw++;}
    LOG_I("Router: %d routes (%d RO, %d RW) direct var",N,ro,rw);
}

static const vr_t *find(uint16_t a){
    int l=0,h=N-1;
    while(l<=h){int m=l+(h-l)/2;if(a<g_routes[m].s)h=m-1;else if(a>g_routes[m].e)l=m+1;else return &g_routes[m];}
    return NULL;
}

int reg_read(uint16_t s,uint8_t n,uint8_t *o){
    int t=0;const vr_t *L=NULL;uint8_t d[4];uint8_t z=0;
    for(uint8_t i=0;i<n;i++){const vr_t*e=find(s+i);
        if(!e||!(e->a&REG_RO))return -1;
        if(L&&e==L){memcpy(o+t,d,z);t+=z;continue;}
        if(e->z==1){uint8_t v=*(uint8_t*)e->v;o[t]=0;o[t+1]=v;d[0]=0;d[1]=v;z=2;t+=2;}
        else if(e->z==2){uint16_t v=*(uint16_t*)e->v;o[t]=v>>8;o[t+1]=v;d[0]=o[t];d[1]=o[t+1];z=2;t+=2;}
        else if(e->z==4){uint32_t v=*(uint32_t*)e->v;o[t]=v>>24;o[t+1]=v>>16;o[t+2]=v>>8;o[t+3]=v;memcpy(d,o+t,4);z=4;t+=4;}
        L=e;
    }
    return t;
}

int reg_write(uint16_t s,uint8_t n,const uint8_t *d){
    int off=0;
    for(uint8_t i=0;i<n;i++){const vr_t*e=find(s+i);
        if(!e||!(e->a&REG_WO))return -1;
        uint32_t old=0;
        if(e->z==1){old=*(uint8_t*)e->v;*(uint8_t*)e->v=d[off+1];off+=2;}
        else if(e->z==2){old=*(uint16_t*)e->v;*(uint16_t*)e->v=((uint16_t)d[off]<<8)|d[off+1];off+=2;}
        else if(e->z==4){old=*(uint32_t*)e->v;*(uint32_t*)e->v=((uint32_t)d[off]<<24)|((uint32_t)d[off+1]<<16)|((uint32_t)d[off+2]<<8)|d[off+3];off+=4;}
        rt_kprintf("[OD_WR] reg=%d old=%d\n",e->s,(int)old);
    }
    return 0;
}

int reg_write_async(uint16_t s,uint8_t n,const uint8_t *d,void(*cb)(int r)){int r=reg_write(s,n,d);if(cb)cb(r);return r;}
