/* NON_MATCHING func_L16_002E8B30 -- src/overlays/l16_kalebo3/vendor_002E7C70.c
 * Best so far: SIZE ours 888 / retail 884, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   - Sample trial into stack, adjust height and setw2, trace. Clear trace accepts parameter/vertical increment/ne
 *   - Wrap parameter once, then flagA6 calls old-position update helper.
 *   1. SIZE888: invert A6 low bit explicitly, invert helper condition to place failure branch in delay slot, and r
 *   2. SIZE888: low-bit inversion and branch layouts now exact. First maxstep evaluation needs to precede independ
 *   3. SIZE888: acceleration timing now exact; remaining extra speed argument move and trial sample F12 slot depen
 *   4. SIZE888: float-first sampling fixes every trial-call argument slot. Remaining duplicate gap-threshold load 
 *   5. SIZE888: void smoothing return and later pointer alias leave identical instructions in remaining threshold/
 *   6. SIZE888: ternary gap normalization leaves the same threshold/speed scheduling differences for the third dis
 */
#include "common.h"
extern int func_L00_0028EB98(void *,int);
extern int func_0022ED80(int,int,void *);
extern void func_0020D678(void *);
extern void func_L16_002E9018(void *);
extern void func_00215CA8(float,int *,int,void *,float *,int);
extern void func_00214D28(float,float,float *);
extern int func_L00_00200290(char *,float);
extern char *D_L16_001B0C30[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L16_0015EE70;
extern short D_L16_00161F38,D_L16_00161F28,D_L16_00161F2C,D_L16_00161F3C,D_L16_00161F40;
extern short D_L16_00161F4C,D_L16_00161F50,D_L16_00161F30,D_L16_00161F34;
#define GF(a) (*(float *)&(a))
#define B(p,o) (*(unsigned char *)((char *)(p)+(o)))
#define H(p,o) (*(unsigned short *)((char *)(p)+(o)))
#define F(p,o) (*(float *)((char *)(p)+(o)))
typedef struct {
    int path_index; float parameter,speed,period; char *owner;
    float height_step,target_speed; short countdown,token;
} L16WalkerData;
/* Follow a shared path, maintain spacing, and test randomized movement ahead. */
void func_L16_002E8B30(char *m) {
    float old[4],rotation[4],trial[4];
    int failed=0;
    L16WalkerData *d=*(L16WalkerData **)(m+0x78);
    int *path=(int *)D_L16_001B0C30[d->path_index];
    if(((H(m,0xA6)^1)&1)!=0) {
        if(func_L00_0028EB98(m,d->token)==0) d->token=func_0022ED80(0,4,m);
        else failed=1;
    } else qcopy(old,m+0x10);
    switch(B(m,0x20)) {
    case 0:
        if(B(m,0x21)==255) {func_0020D678(m);return;}
        func_L16_002E9018(m);
        break;
    case 1: {
        L16WalkerData *other=*(L16WalkerData **)(d->owner+0x78);
        float *speed;
        float gap,step,next,height,acceleration;
        func_00215CA8(d->parameter,path,0,m+0x10,(float *)(m+0x40),0);
        acceleration=GF(D_L16_00161F38)*D_0015EE70;
        F(m,0x18)+=d->height_step;
        d->parameter+=d->speed;
        func_00214D28(d->target_speed,acceleration,&d->speed);
        gap=other->parameter-d->parameter;
        gap=gap<0.0f ? gap+d->period : gap;
        speed=&d->speed;
        if(gap<GF(D_L16_00161F28) && d->target_speed>other->target_speed) {
            float saved=d->target_speed;
            d->target_speed=other->target_speed;
            other->target_speed=saved;
        }
        if(gap<1.5f) {
            step=GF(D_L16_0015EE70)*50.0f;
            func_00214D28(d->target_speed,step,speed);
            func_00214D28(other->target_speed,step,&other->speed);
        }
        if(B(m,0x31)==0 && failed==0) {
        if(gap>GF(D_L16_00161F2C)+1.0f) {
            height=func_002140F8(GF(D_L16_00161F3C),GF(D_L16_00161F40));
            if(B(d->owner,0x31)!=0 && d->countdown!=0) {
                next=other->parameter-func_002140F8(GF(D_L16_00161F28),GF(D_L16_00161F2C));
                d->countdown--;
            } else next=d->parameter+GF(D_L16_00161F4C)*D_0015EE6C;
            if(next<0.0f) next+=d->period;
            func_00215CA8(next,path,0,trial,rotation,1);
            trial[2]+=height;trial[3]=2.0f;
            if(func_L00_00200290((char *)trial,(float)*(int *)&D_L16_00161F50)==-1) {
                float random=func_002140F8(GF(D_L16_00161F30),GF(D_L16_00161F34));
                d->parameter=next;d->height_step=height;d->target_speed=random*D_0015EE6C;
            }
        }
        } else d->countdown=30;
        if(d->parameter>d->period) d->parameter-=d->period;
        break;
    }
    }
    if(H(m,0xA6)&1) func_L16_002E8EA8(m,old);
}
