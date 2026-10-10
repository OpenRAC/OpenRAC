/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MACRO_ADDR
#include "../game/rac1-pal/hand/level_hero_vertical.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
#define HF(o) (*(float *)(D_0013F450+(o)))
#define HI(o) (*(int *)(D_0013F450+(o)))
#define HS(o) (*(short *)(D_0013F450+(o)))
_Alignas(16) char D_0013F450[0x2400], D_0013A5E0[0x2700];
unsigned char D_0013D5CA;
float D_0015EE6C, D_0015EE70;
int D_0015EE84;
static int classification, classify_calls, transitions, sets, adds, subtracts, eases, axis;
static float ease_target, ease_amount;
static HeroVerticalStep steps[4];
static void near(float a,float b) { CHECK(fabsf(a-b)<0.00001f); }
int func_L00_0020DB30(int a) { CHECK(a==3); ++classify_calls; return classification; }
int func_001F9850(int a) { return a; }
int func_L00_00217570(int a,int b) { CHECK(a==1 && b==0); ++transitions; return 0; }
void func_001F9BF0(void *o,void *a,void *b) {
    float *out=o,*x=a,*y=b;
    for(int i=0;i<3;++i) out[i]=x[i]-y[i];
    out[3]=x[3];
}
float func_L00_002342F8(float *v) { return v[axis]; }
float func_00214D28(float *v,float goal,float step) {
    ++eases; ease_target=goal; ease_amount=step;
    if(*v<goal) *v=fminf(*v+step,goal);
    else *v=fmaxf(*v-step,goal);
    return *v;
}
void func_L00_002343A0(float *out,float *in,float z) {
    ++sets; memmove(out,in,16); out[axis]=z;
}
float func_001F9B50(float x) { return sqrtf(x); }
void func_L00_00234090(float *out,float *in,float dz) {
    ++adds; memmove(out,in,16); out[axis]+=dz;
}
void func_L00_00233D50(float *out,float *in,float dz) {
    ++subtracts; memmove(out,in,16); out[axis]-=dz;
}
static void reset(int state,int ticks) {
    memset(D_0013F450,0,sizeof D_0013F450);
    memset(D_0013A5E0,0,sizeof D_0013A5E0);
    memset(steps,0,sizeof steps);
    HI(0x2084)=state; HI(0x198)=ticks;
    HF(0xE0)=3; HF(0xE4)=4; HF(0xE8)=2; HF(0xEC)=123;
    HF(0x118)=-100; HF(0x430)=HF(0x48C)=1;
    D_0015EE6C=0.02f; D_0015EE70=0.01f;
    D_0015EE84=14; D_0013D5CA=0;
    classification=classify_calls=transitions=sets=adds=subtracts=eases=0;
    axis=2;
}
static void result(float z) {
    near(HF(0xE0),3); near(HF(0xE4),4); near(HF(0xE8),z); near(HF(0xEC),123);
}
static HeroVerticalSequence *sequence(void) { return (HeroVerticalSequence *)(D_0013F450+0x3D0); }
int main(void) {
    _Static_assert(sizeof(HeroVerticalStep)==12,"retail step stride");
    if(sizeof(void *)==4) {
        CHECK(offsetof(HeroVerticalSequence,arr)==0x14);
        CHECK(offsetof(HeroVerticalSequence,idx)==0x18);
        CHECK(offsetof(HeroVerticalSequence,cnt)==0x1C);
        CHECK(offsetof(HeroVerticalSequence,val)==0x20);
    }
    reset(0,20); HF(0x4A0)=0.1f; func_L00_002147C0(); result(1.9f);
    reset(0,20); HF(0x4A0)=2; HF(0x118)=1;
    func_L00_002147C0(); result(0.9f); /* Previous vertical speed minus 0.1. */
    reset(0,20); HF(0xE8)=-10; func_L00_002147C0(); result(-1); /* Terminal limit. */
    reset(0,2); HI(0x420)=3; func_L00_002147C0(); result(-0.48f);
    CHECK(sets==1 && subtracts==1);
    for(int state=0x1C;state<=0x4C;state+=0x30) {
        reset(state,9); HF(0x4A0)=0.1f;
        func_L00_002147C0(); result(2); CHECK(sets==0 && subtracts==0);
        HI(0x198)=10; func_L00_002147C0(); result(1.9f);
    }
    reset(0,20); HF(0x428)=3; HF(0x42C)=2; HF(0x4A0)=0.1f;
    func_L00_002147C0(); result(4.9f); near(HF(0x428),0); near(HF(0x42C),5);
    CHECK(adds==1);
    /* Ramp activation: input bit, initially empty impulse, or state 18. */
    for(int gate=0;gate<4;++gate) {
        reset(gate==2?18:0,15); HF(0x430)=1; HF(0x48C)=2;
        HF(0x488)=0; HS(0x498)=2; HF(0x4A0)=0.5f;
        HF(0x428)=0.25f; HF(0x42C)=0.5f;
        if(gate==0) *(int *)(D_0013A5E0+0x2600)=0x40;
        if(gate==1) HF(0x428)=HF(0x42C)=0;
        func_L00_002147C0();
        near(HF(0x430),gate==3?1:2);
        result(gate==3?1.75f:2+sqrtf(2)-(gate==1?0:0.5f)-0.5f);
    }
    reset(0,16); HF(0x48C)=2; HS(0x498)=1;
    func_L00_002147C0(); near(HF(0x430),1); /* Ramp timeout. */
    reset(0,0); HF(0x48C)=2; HS(0x498)=1; HF(0x430)=1.5f;
    func_L00_002147C0(); near(HF(0x430),2); /* Overshoot clamp. */
    /* Timed entries: initial index, duration equality, sentinel and slope. */
    reset(0,5); HS(0x41C)=1; HI(0x3D0)=5; HI(0x3D4)=10;
    HeroVerticalSequence *seq=sequence(); seq->arr=steps; seq->idx=-1;
    steps[0]=(HeroVerticalStep){4,2,2};
    steps[1]=(HeroVerticalStep){-999999.0f,-1,3};
    func_L00_002147C0(); CHECK(seq->idx==0 && seq->cnt==0); near(seq->val,0.04f); result(2.04f);
    func_L00_002147C0(); CHECK(seq->idx==0 && seq->cnt==1); near(seq->val,0.06f);
    func_L00_002147C0(); CHECK(seq->idx==1 && seq->cnt==0); near(seq->val,0.06f);
    func_L00_002147C0(); CHECK(seq->cnt==1); near(seq->val,0.05f);
    HI(0x198)=10; func_L00_002147C0(); CHECK(seq->cnt==1);
    HI(0x198)=4; func_L00_002147C0(); CHECK(seq->cnt==1);
    /* Hover/helipack state: thresholds and classification multiplier. */
    float heights[]={1.7f,1.8f,2.1f,2.2f,2.7f,2.8f};
    for(int heli=0;heli<2;++heli) for(int i=0;i<6;++i) {
        reset(14,0); D_0013D5CA=(unsigned char)heli; classification=heli?3:0;
        HF(0x88)=heights[i]; HF(0xE8)=0;
        func_L00_002147C0();
        float factor=heli?(heights[i]>2.7f?0.47f:heights[i]>2.1f?0.7f:1):
                          (heights[i]>2.1f?0.45f:heights[i]>1.7f?0.7f:1);
        near(ease_target,0.14f*factor*(heli?1.25f:1));
        near(ease_amount,1.5f); CHECK(eases==1);
        result(ease_target);
    }
    reset(14,5); D_0015EE84=0; classification=3;
    func_L00_002147C0(); CHECK(transitions==1 && eases==0); result(2);
    reset(14,6); D_0015EE84=0; classification=3;
    func_L00_002147C0(); CHECK(transitions==0);
    reset(14,8); HF(0xE8)=0; func_L00_002147C0(); CHECK(eases==0 && sets==1);
    reset(14,0); D_0013D5CA=1; classification=3; HS(0x22D8)=1; HF(0xE8)=0;
    func_L00_002147C0(); near(ease_target,0.14f); CHECK(classify_calls==1);
    reset(0,20); axis=0; HF(0x110)=-100; HF(0x4A0)=0.25f;
    func_L00_002147C0(); near(HF(0xE0),2.75f); near(HF(0xE8),2);
    puts("hero vertical: impulse, sequence, hover gates and gravity passed");
    return 0;
}
