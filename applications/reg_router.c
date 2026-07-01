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
extern UNS8  CanOpenMaster_obj2001; extern UNS32 CanOpenMaster_obj2002;
extern UNS32 CanOpenMaster_obj2003; extern UNS16 CanOpenMaster_obj2004;
extern UNS16 CanOpenMaster_obj2005; extern UNS8  CanOpenMaster_obj2006;
extern UNS32 CanOpenMaster_obj2007; extern UNS32 CanOpenMaster_obj2008;
extern UNS16 CanOpenMaster_obj2009; extern UNS16 CanOpenMaster_obj200A;
extern UNS8  CanOpenMaster_obj200B; extern UNS32 CanOpenMaster_obj200C;
extern UNS32 CanOpenMaster_obj200D; extern UNS16 CanOpenMaster_obj200E;
extern UNS16 CanOpenMaster_obj200F; extern UNS8  CanOpenMaster_obj2010;
extern UNS32 CanOpenMaster_obj2011; extern UNS32 CanOpenMaster_obj2012;
extern UNS16 CanOpenMaster_obj2013; extern UNS16 CanOpenMaster_obj2014;
extern UNS8  CanOpenMaster_obj2015; extern UNS32 CanOpenMaster_obj2016;
extern UNS32 CanOpenMaster_obj2017; extern UNS16 CanOpenMaster_obj2018;
extern UNS16 CanOpenMaster_obj2019; extern UNS16 CanOpenMaster_obj201A;
extern UNS16 CanOpenMaster_obj201B; extern UNS32 CanOpenMaster_obj201C;
extern UNS32 CanOpenMaster_obj201D; extern UNS32 CanOpenMaster_obj201E;
extern UNS32 CanOpenMaster_obj201F; extern UNS32 CanOpenMaster_obj2020;
extern UNS32 CanOpenMaster_obj2021; extern UNS16 CanOpenMaster_obj2022;
extern UNS8  CanOpenMaster_obj2023; extern UNS32 CanOpenMaster_obj2024;
extern UNS32 CanOpenMaster_obj2025; extern UNS32 CanOpenMaster_obj2026;
extern UNS32 CanOpenMaster_obj2027; extern UNS16 CanOpenMaster_obj2028;
extern UNS16 CanOpenMaster_obj2029; extern UNS16 CanOpenMaster_obj202A;
extern UNS16 CanOpenMaster_obj202B;

typedef struct { uint16_t s,e; void *v; uint8_t z,a; } vr_t;
static const vr_t g_routes[] = {
    {0x0004,0x0004,&CanOpenMaster_obj2001, 1, REG_RW},
    {0x0005,0x0005,&CanOpenMaster_obj2005, 2, REG_RW},
    {0x001C,0x001C,&CanOpenMaster_obj2004, 2, REG_RO},
    {0x0099,0x0099,&CanOpenMaster_obj2003, 2, REG_RW},
    {0x00A4,0x00A5,&CanOpenMaster_obj2002, 4, REG_RW},
    {0x00D2,0x00D2,&CanOpenMaster_obj2028, 2, REG_RO},
    {0x000A,0x000A,&CanOpenMaster_obj2006, 1, REG_RW},
    {0x001D,0x001D,&CanOpenMaster_obj200A, 2, REG_RW},
    {0x0012,0x0012,&CanOpenMaster_obj2009, 2, REG_RO},
    {0x009D,0x009D,&CanOpenMaster_obj2008, 2, REG_RW},
    {0x00AC,0x00AD,&CanOpenMaster_obj2007, 4, REG_RW},
    {0x00D3,0x00D3,&CanOpenMaster_obj2029, 2, REG_RO},
    {0x000B,0x000B,&CanOpenMaster_obj200F, 2, REG_RW},
    {0x000C,0x000D,&CanOpenMaster_obj200C, 4, REG_RW},
    {0x00AE,0x00AE,&CanOpenMaster_obj200B, 1, REG_RW},
    {0x009C,0x009C,&CanOpenMaster_obj200D, 2, REG_RW},
    {0x00CB,0x00CB,&CanOpenMaster_obj200E, 2, REG_RO},
    {0x00D4,0x00D4,&CanOpenMaster_obj202A, 2, REG_RO},
    {0x0008,0x0008,&CanOpenMaster_obj2012, 2, REG_RW},
    {0x0009,0x0009,&CanOpenMaster_obj2011, 2, REG_RW},
    {0x000F,0x000F,&CanOpenMaster_obj2010, 1, REG_RW},
    {0x0014,0x0014,&CanOpenMaster_obj2013, 2, REG_RO},
    {0x009F,0x009F,&CanOpenMaster_obj2014, 2, REG_RW},
    {0x0006,0x0007,&CanOpenMaster_obj2016, 4, REG_RW},
    {0x0078,0x0078,&CanOpenMaster_obj2015, 1, REG_RW},
    {0x0015,0x0015,&CanOpenMaster_obj2018, 2, REG_RO},
    {0x00A3,0x00A3,&CanOpenMaster_obj2017, 2, REG_RW},
    {0x00CC,0x00CC,&CanOpenMaster_obj2019, 2, REG_RW},
    {0x0010,0x0010,&CanOpenMaster_obj201A, 2, REG_RO},
    {0x0011,0x0011,&CanOpenMaster_obj201B, 2, REG_RO},
    {0x00C7,0x00C8,&CanOpenMaster_obj201C, 4, REG_RW},
    {0x00C9,0x00C9,&CanOpenMaster_obj2022, 2, REG_RW},
    {0x00CA,0x00CA,&CanOpenMaster_obj2023, 1, REG_RW},
    {0x00D0,0x00D1,&CanOpenMaster_obj201D, 4, REG_RW},
    {0x00C0,0x00C0,&CanOpenMaster_obj201E, 2, REG_RW},
    {0x00C1,0x00C1,&CanOpenMaster_obj201F, 2, REG_RW},
    {0x0064,0x0064,&CanOpenMaster_obj2020, 2, REG_RW},
    {0x0065,0x0065,&CanOpenMaster_obj2021, 2, REG_RW},
    {0x001F,0x001F,&CanOpenMaster_obj2024, 2, REG_RO},
    {0x0066,0x0066,&CanOpenMaster_obj2025, 2, REG_RO},
    {0x0067,0x0067,&CanOpenMaster_obj2026, 2, REG_RO},
    {0x0068,0x0068,&CanOpenMaster_obj2027, 2, REG_RO},
    {0x00D5,0x00D5,&CanOpenMaster_obj202B, 2, REG_RO},
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
