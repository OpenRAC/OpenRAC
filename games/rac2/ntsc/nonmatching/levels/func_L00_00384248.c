/*
 * Tier: GPT-6.1 Sol (high), task v0943, 29 October 2026.
 * Current US check.py: 1072/1072 bytes, 0/27 placements; no linked match.
 * First differing instruction: +0x2B4 (commuted operands in a pointer add).
 * Profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
 * The work layout below reproduces the 0xA0 frame and saved-register allocation.
 * Repeated per-player input selections are required; the earlier persistent
 * control pointer incorrectly collapsed these branches. The two conversions
 * in the alpha pulse must both precede multiplying the converted phase.
 * SDATA preserves the measured 16-byte vector extent while selecting its GP
 * address. The camera's position and basis share the base at +0 and +0x230.
 * Remaining: three commuted object-table pointer additions, button-selection
 * pointer allocation, and timer-store/vector-copy address allocation/scheduling.
 * Explicit slot expressions, qcopy and moving the slot initializer tie at the
 * same bytes. Whole-file -mno-split-addresses changes much of the allocation.
 * Needs the declarations below. Any eventual match must also qualify symbol
 * bindings and linked placements; this retained draft has not passed those.
 */
#include "common.h"

typedef int Quadword __attribute__((mode(TI)));
typedef union { Quadword bits; float f[4]; } Vector4;
#define FIELD(type, p, off) (*(type *)((char *)(p) + (off)))

extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_002AEC18(void *, int);
extern void func_002A05A8(char *);
extern void func_002A0F48(void *);
extern float func_002AA2E8(float *, float, float);
extern int func_00282948(int *);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern float func_00283198(float);
extern void func_00282F20(void *, void *, float);
extern void *func_L00_00384CA8(void *, void *, void *, unsigned int);
extern int func_002E5CF8(int, int, char *);

extern char D_L00_001B8DC0[];
extern float D_L00_001AEDA0[2] __attribute__((sda));
extern int D_L00_001AEDA8[2] __attribute__((sda));
extern float D_L00_001AEDB0 __attribute__((sda));
extern float D_L00_001AEDB4 __attribute__((sda));
extern float D_L00_001AEDBC __attribute__((sda));
extern int D_L00_001AEDD0[2] __attribute__((sda));
extern Vector4 D_L00_001AEDC0 SDATA(D_L00_001AEDC0);
extern int D_L00_001B19C8;

/* Measured input fields; other fields are not yet identified. */
typedef struct {
    char pad[0x10C];
    float axis;
    char pad110[8];
    float negative, positive;
    char pad120[0x80];
    int buttons;
} PlayerControl;
extern PlayerControl D_00138500, D_00138180;

typedef struct {
    Vector4 basis[2];
    Vector4 motion[2];
    int timers[2];
    char *objects[2];
} AttachmentWork;

void func_L00_00384248(char *owner)
{
    AttachmentWork *work=FIELD(AttachmentWork *,owner,0x68);
    char **slots=work->objects;
    Vector4 position,velocity;
    float scale,step,input,target,phase,period;
    int i,alpha;
    for(i=0;i<2;++i) {
        func_00283098(slots[i]+0x10,&work->basis[i],(D_L00_001B8DC0+0x230));
        func_00282CC0(slots[i]+0x10,slots[i]+0x10,D_L00_001B8DC0);
        FIELD(float,slots[i],0x2C)=FIELD(float,FIELD(char *,slots[i],0x24),0x24)*D_L00_001AEDA0[i];
        func_002AEC18(slots[i],D_L00_001AEDA8[i]);
        func_002A05A8(slots[i]);
        func_002A0F48(slots[i]);
    }
    scale=D_L00_001AEDB0*0.016666668f;
    step=D_L00_001AEDBC*0.00027777778f;
    for(i=0;i<2;++i) {
        if(i)input=D_00138180.axis;else input=D_00138500.axis;
        if(input==0) { if(i)input=D_00138180.positive-D_00138180.negative; else input=D_00138500.positive-D_00138500.negative; }
        target=-input*scale;
        if(target!=0)func_002AA2E8(&work->motion[i].f[2],target,step);
        else func_002AA2E8(&work->motion[i].f[2],0,step+step);
        work->basis[i].f[2]+=work->motion[i].f[2];
        if(work->basis[i].f[2]>0.35f) {
            work->basis[i].f[2]=0.35f;
            if(work->motion[i].f[2]>0)work->motion[i].f[2]=0;
        }else if(work->basis[i].f[2]<-0.45f) {
            work->basis[i].f[2]=-0.45f;
            if(work->motion[i].f[2]<0)work->motion[i].f[2]=0;
        }
        if(func_00282948(&D_L00_001AEDD0[i]))FIELD(unsigned char,slots[i],0x23)=128;
        else {
            phase=func_00283CE0(D_L00_001B19C8%60);
            period=func_00283CE0(60);
            phase*=6.2831855f;
            phase/=period;
            alpha=func_00283CF0(func_00283198(phase-3.1415927f)*10)+20;
            FIELD(unsigned char,slots[i],0x23)=alpha;
        }
        if(func_00282948(&work->timers[i]) &&
           FIELD(unsigned char,slots[i],0x23)==128 && ((i?&D_00138180:&D_00138500)->buttons&0xE0)) {
            work->timers[i]=45;
            position.bits=D_L00_001AEDC0.bits;
            if(!i)position.f[1]-=0.05f;
            func_00283098(&position,&position,slots[i]+0xC0);
            func_00282CC0(&position,&position,slots[i]+0x10);
            func_00282F20(&velocity,slots[i]+0xC0,D_L00_001AEDB4*0.016666668f);
            func_L00_00384CA8(slots[i],&position,&velocity,i);
            func_002E5CF8(0,0,slots[i]);
        }
    }
}
