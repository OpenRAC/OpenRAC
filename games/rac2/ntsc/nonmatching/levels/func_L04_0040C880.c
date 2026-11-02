/* sol-high, USA v1.01, -O2 -G8 -ffunction-sections.
 * Best check.py: 1360/1360 bytes, 0/2 places, first mismatch +0x8.
 * Corrected the final angle multiplier to 0x3c0efa36 (was one ULP low).
 * Object stays in s7, state in fp; retail uses fp and sp+0xa0. Constants
 * 1.0, 0.0 and 0.8 remain live across the sprite loop instead of being
 * materialized at retail's individual draws/inner-loop entry. Color-loop
 * threshold allocation and scheduling also differ.
 * Reusing threshold integers as coordinates, per-draw coordinate assignment,
 * direct nested sprite calls, integer/FP argument interleaving, and const
 * tables do not fix the lifetime shape. A ProjectionState struct view gives
 * retail's explicit base+offset store but does not fix allocation. -fno-gcse
 * gives 1324 bytes, -fno-rerun-loop-opt 1356, -fno-strength-reduce 1396 on
 * the measured variants; -fno-expensive-optimizations keeps excess FP lives.
 * Next: independently establish the original coordinate/loop lifetime shape.
 * All declarations are below; integrate in a -g8.c unit (GP frame counter).
 */
#include "common.h"
typedef struct { int v[5]; } B1202Five;
extern B1202Five D_L04_001B09B0, D_L04_001B09C8, D_L04_001B09E0, D_L04_001B09F8;
extern int D_L04_001A8FE8 __attribute__((sda));
extern unsigned char *D_L04_001A904C;
extern unsigned char D_L04_001BDB80[];
extern float D_L04_001B2D44;
extern int D_001A7B90;
extern int func_00283D38(int,int,float);
extern float func_00283198(float),func_00283180(float);
extern void func_0027AB40(void);
extern void *func_00336D50(void *,int);
extern void func_00300A00(void *,float,float,float,float,float,unsigned int);
extern void func_002FC560(int,unsigned long long);
void func_L04_0040C880(unsigned char *m)
{
    B1202Five x = D_L04_001B09B0;
    B1202Five y = D_L04_001B09C8;
    B1202Five start = D_L04_001B09E0;
    B1202Five end = D_L04_001B09F8;
    int colors[5];
    unsigned char *p = *(unsigned char **)(m+0x68);
    int frame,i,side,left,right,a,b,c,d;
    void *sprite;
    float angle, sine, sign, fx, fy;
    if ((unsigned)(m[0xbc]-3)<2 || m[0xbc]==1 || m[0xbc]==2) frame=150-*(int *)(p+0x394);
    else frame=D_L04_001A8FE8+150;
    if (!D_L04_001A904C) return;
    for(i=0;i<5;i++) {
        if(i==4) { a=120;b=149;c=156;d=171; }
        else { a=i*30;b=a+30;c=b;d=a+45; }
        colors[i]=0;
        if(frame>=a && frame<b) colors[i]=func_00283D38(0,start.v[i],(float)(frame-a)*0.033333335f);
        else if(frame>=b && frame<c) { if(b!=c) colors[i]=start.v[i]; }
        else if(frame>=c && frame<d) colors[i]=func_00283D38(start.v[i],end.v[i],(float)(frame-c)*0.06666667f);
        else if(frame>=d) colors[i]=end.v[i];
    }
    angle=0.56217057f;
    sine=func_00283198(angle);
    *(float *)(D_L04_001BDB80+0xb0)=sine/func_00283180(angle);
    func_0027AB40();
    for(side=0;side<2;side++) {
        if(side==0) {left=10;right=31;sign=1.0f;} else {left=498;right=478;sign=-1.0f;}
        fx=(float)left;
        sprite=func_00336D50(D_L04_001A904C+0x8710,58);
        func_00300A00(sprite,fx,(float)(int)(D_L04_001B2D44*120.0f+0.5f),sign,1.0f,0.0f,0x60442d00);
        sprite=func_00336D50(D_L04_001A904C+0x8710,59);
        func_00300A00(sprite,fx,(float)(int)(D_L04_001B2D44*120.0f+0.5f),sign,1.0f,0.0f,0x55f0c070);
        sprite=func_00336D50(D_L04_001A904C+0x8710,60);
        func_00300A00(sprite,fx,(float)(int)(D_L04_001B2D44*120.0f+0.5f),sign,1.0f,0.0f,0x55f0c070);
        fy=(float)right;
        for(i=0;i<5;i++) {
            sprite=func_00336D50(D_L04_001A904C+0x8710,61);
            func_00300A00(sprite,fy,(float)(D_001A7B90?y.v[i]:x.v[i]),0.8f,0.8f,0.0f,colors[i]);
        }
        func_002FC560(71,0x5360b);
    }
    if(m[0x20]!=3) {
        angle=*(float *)(p+0x28c)*0.008726647f;
        sine=func_00283198(angle);
        *(float *)(D_L04_001BDB80+0xb0)=sine/func_00283180(angle);
        func_0027AB40();
    }
}
