#include "common.h"
typedef unsigned int u128 __attribute__((mode(TI)));
extern void func_00282CF0(void *,void *,void *);
extern void func_00282CC0(void *,void *,void *);
extern void func_00282D30(void *,void *,float);
extern void func_00282F20(void *,void *,float);
extern void func_00282FB8(void *,void *,float);
extern float func_00282DF0(void *);
extern float func_00283CE0(int);
extern float func_002A7798(float);
extern float func_L01_00310BC0(float,float,float,void *,float,float);
extern void func_002A8240(void *,void *,int,float,float,float);
int func_L08_00425FE8(char *a,float accel,float speed,float turn,float blend,float angle) {
    float old[4] __attribute__((aligned(16)));
    float dir[4] __attribute__((aligned(16)));
    char *q=*(char **)(a+0x68);
    float distance, velocity, limit, timer;
    int braking=0;
    *(u128 *)old=*(u128 *)(a+0x10);
    speed *= 0.016666668f;
    accel *= 0.00027777778f;
    func_00282CF0(dir,q+0x180,a+0x10);
    distance=func_00282DF0(dir);
    velocity=func_00282DF0(q+0x190);
    if (distance < velocity*(velocity/accel)) {velocity-=accel; braking=1;}
    else velocity += accel*func_002A7798((speed-velocity)/speed);
    func_00282F20(dir,dir,velocity);
    func_00282D30(q+0x190,q+0x190,blend);
    func_00282D30(dir,dir,1.0f-blend);
    func_00282CC0(q+0x190,q+0x190,dir);
    func_00282CC0(q+0x190,q+0x190,q+0x1A0);
    *(float *)(a+0x18)+=2.0f;
    timer=func_00283CE0(*(s16 *)(q+0x1BE));
    { float divisor=func_00283CE0(120); timer *= 6.0f; limit=timer/divisor; }
    if(limit>6.0f)limit=6.0f;
    func_002A8240(a,q+0x190,0,0.0f,limit,0.0f);
    *(float *)(a+0x18)-=2.0f;
    func_00282CF0(q+0x190,q+0x190,q+0x1A0);
    *(float *)(q+0x208)=func_L01_00310BC0(*(float *)(q+0x208),turn,0.0005817764904350042f,q+0x1C0,0.0017453294713050127f,0.03490659222006798f);
    *(float *)(a+0xF4)=func_L01_00310BC0(*(float *)(a+0xF4),angle,0.0004848137323278934f,q+0x220,0.0017453294713050127f,0.03490659222006798f);
    func_00282D30(q+0x1A0,q+0x1A0,0.95f);
    func_00282CF0(dir,a+0x10,old);
    func_00282FB8(dir,dir,0.13333334f);
    func_00282CC0(a+0x10,old,dir);
    return braking;
}
