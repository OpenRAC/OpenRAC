/* NON_MATCHING func_L16_002C7218 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: BYTES 44/948 (95.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   10. BYTES78: earlier ground-alias birth rotates moby/data/position and does not help. Model actual moby/veloci
 *   11. SIZE952: real struct field types preserve p7 differences exactly. Assign persistent ground pointer just af
 *   12. BYTES44/948: trace-result lifetime change removes every difference beyond position/velocity/data GPR permu
 *   13 verdict in tool output: void vector aliases tested without changing memory math. Replace remaining byte-off
 *   13 confirmed BYTES44: void aliases identical.
 *   14 verdict in tool output: named actual fields tested. Move persistent velocity assignment after integration o
 *   14 confirmed BYTES44: named fields identical.
 *   15. BYTES44: late velocity assignment with named fields also identical; STOP after three different wordings le
 */
#include "common.h"
extern void func_001F9BD8(void *,void *,void *);
extern void func_L00_001FF4B0(void *,void *,float);
extern int func_001F9850(int);
extern int func_001F9908(int *);
extern int func_L00_001EFFF0(void *,void *,int,int,int);
extern float func_001F9C78(void *,void *);
extern void func_L00_001FF610(void *,void *,void *);
extern void func_001F9C30(void *,void *,float);
extern int func_L00_001F10E0(float,void *,int,void *);
extern float func_001F9CB8(void *);
extern void func_L00_0025E4B0(void *,short *);
extern void func_L00_0025E590(void *,void *);
extern void func_L00_0025F4A8(void *,void *,void *,float,float,int,int,int,float,float,float,int,float,float,int,int,int,int);
extern char D_L16_00174300[];
extern char D_L16_0015F660[] MACRO_ADDR;
#define B(p,o) (*(unsigned char *)((char *)(p)+(o)))
#define F(p,o) (*(float *)((char *)(p)+(o)))
#define W(p,o) (*(int *)((char *)(p)+(o)))
typedef struct {char pad0[0x10]; float position[4]; unsigned char state; char pad21[0x57]; void *data;} L16FallingMoby;
typedef struct {char pad0[7]; unsigned char reaction; char pad8[8]; float velocity[4]; int collision; float gravity; int timer;} L16FallingData;
/* Move a falling effect, bounce it off surfaces, and finish its damage/death states. */
void func_L16_002C7218(L16FallingMoby *m) {
    float previous[4],normal[4];
    L16FallingData *d=m->data;
    switch(m->state) {
    case 1: {
        int hit;
        float *velocity;
        float *ground_position;
        float *old=previous;
        float *position=m->position;
        qcopy(old,position);
        func_001F9BD8(position,position,d->velocity);
        velocity=d->velocity;
        hit=func_L00_001EFFF0(old,position,5,d->collision,0);
        ground_position=m->position;
        if(hit) {
            func_L00_001FF4B0(normal,D_L16_00174300,1.0f);
            if(func_001F9C78(normal,velocity)<0.0f) {
                func_L00_001FF610(velocity,velocity,normal);
                func_001F9C30(velocity,velocity,0.25f);
            }
        }
        if(func_L00_001F10E0(0.333f,ground_position,2,0)) {
            char *surface=D_L16_00174300;
            func_L00_001FF4B0(normal,surface,0.333f);
            if(func_001F9C78(normal,velocity)<0.0f) {
                func_L00_001FF610(velocity,velocity,normal);
                func_001F9C30(velocity,velocity,0.25f);
            }
            qcopy(ground_position,surface-0x20);
            m->position[2]+=normal[2];
            if(normal[2]>=0.3f && func_001F9CB8(velocity)<D_0015EE6C*0.25f) {
                m->state=2;d->timer=func_001F9850(90);
            }
        }
        if(func_001F9908(&d->timer)) m->state=3;
        else if(d->timer<func_001F9850(45) && d->timer%func_001F9850(10)==0) {
            d->reaction=250;func_L00_0025E4B0(m,(short *)d);
        }
        d->velocity[2]-=d->gravity;
        break;
    }
    case 2:
        if(func_001F9908(&d->timer)) m->state=3;
        else if(d->timer<func_001F9850(61) && d->timer%func_001F9850(20)==0) {
            d->reaction=250;func_L00_0025E4B0(m,(short *)d);
        }
        break;
    case 3:
        func_0022ED80_i(0,0,m);
        func_L00_0025F4A8(m,D_L16_0015F660,0,1.5f,1.0f,7,10,20,3.0f,1.7f,4.0f,-1,1.0f,7.0f,0,7,-1,0);
        func_0020D678(m);
        return;
    }
    func_L00_0025E590(m,d);
}
