/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MACRO_ADDR
#include "../game/rac1-pal/hand/level_hero_gait.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
#define HF(o) (*(float *)(D_0013F450+(o)))
#define HI(o) (*(int *)(D_0013F450+(o)))
_Alignas(16) unsigned char D_0013F450[0x2400];
struct HeroGaitRange D_L00_0017BEB8[2];
float D_0015EE6C;
int D_L00_0015F7A8[4];
_Alignas(16) static unsigned char moby[0x100], cls[0x100], walk[0x20], run[0x20];
static int ticks, changes, sequence, phase, interrupt_gate;
static float blend;
int func_001F9850(int n) {
    ++ticks;
    if (interrupt_gate && ticks==2) HI(0x1A0)=0;
    return n+1;
}
void func_L00_00232C10(int seq, int frame, float duration) {
    ++changes; sequence=seq; phase=frame; blend=duration;
}
static void near(float a,float b) { CHECK(fabsf(a-b)<0.00001f); }
static void reset(int mode,int gait,float speed) {
    memset(D_0013F450,0,sizeof D_0013F450);
    memset(moby,0,sizeof moby); memset(cls,0,sizeof cls);
    D_0013F450[0x20A4]=(unsigned char)mode; HI(0x2088)=gait;
    HF(0x160)=HF(0x194)=speed; HF(0xA90)=42; HI(0x1A0)=6;
    D_0015EE6C=1;
    D_L00_0017BEB8[0]=(struct HeroGaitRange){-FLT_MAX,0.1f,0,0};
    D_L00_0017BEB8[1]=(struct HeroGaitRange){0.08f,FLT_MAX,0,0};
    D_L00_0015F7A8[1]=27; D_L00_0015F7A8[2]=-3;
    char *p=(char *)moby; memcpy(D_0013F450+0x2080,&p,sizeof p);
    p=(char *)cls; memcpy(moby+0x24,&p,sizeof p);
    unsigned char **table=(unsigned char **)(cls+0x48);
    table[3]=walk; table[4]=run;
    walk[0x10]=20; run[0x10]=30;
    moby[0x51]=7;
    ticks=changes=interrupt_gate=0;
}
int main(void) {
    _Static_assert(sizeof(struct HeroGaitRange)==16,"retail threshold stride");
    reset(3,0,0); func_L00_002293E8(); near(HF(0xA90),0.7f); CHECK(ticks==0);
    reset(3,0,0.2f); func_L00_002293E8(); near(HF(0xA90),10.8f);
    reset(1,0,0); func_L00_002293E8(); near(HF(0xA90),0.5f);
    reset(1,0,0.2f); func_L00_002293E8(); near(HF(0xA90),18);
    reset(0,0,0.2f); func_L00_002293E8();
    CHECK(HI(0x2088)==1 && changes==1 && sequence==4 && phase==7 && ticks==3);
    near(blend,9); near(HF(0xA90),2.2f);
    reset(0,1,0.04f); moby[0x51]=23; func_L00_002293E8();
    CHECK(HI(0x2088)==0 && changes==1 && sequence==3 && phase==12); near(HF(0xA90),4);
    /* The two thresholds provide hysteresis, including equality. */
    reset(0,0,0.1f); func_L00_002293E8(); CHECK(changes==0 && HI(0x2088)==0);
    reset(0,1,0.08f); func_L00_002293E8(); CHECK(changes==0 && HI(0x2088)==1);
    reset(0,0,0.09f); func_L00_002293E8(); CHECK(changes==0);
    reset(0,1,0.09f); func_L00_002293E8(); CHECK(changes==0);
    reset(0,0,0.15f); D_0015EE6C=2; func_L00_002293E8(); CHECK(changes==0);
    reset(0,0,0.25f); D_0015EE6C=2; func_L00_002293E8(); CHECK(changes==1);
    /* Both gate checks must run; animation speed still updates when blocked. */
    reset(0,0,0.2f); HI(0x1A0)=5; func_L00_002293E8(); CHECK(changes==0); near(HF(0xA90),4);
    reset(0,0,0.2f); *(short *)(D_0013F450+0x3BC)=1;
    func_L00_002293E8(); CHECK(changes==0);
    reset(0,0,0.2f); interrupt_gate=1; func_L00_002293E8(); CHECK(changes==0 && ticks==2);
    reset(0,0,0); func_L00_002293E8(); near(HF(0xA90),0.6f);
    reset(0,0,0.01f); func_L00_002293E8(); near(HF(0xA90),1.57f);
    reset(0,1,0); HI(0x1A0)=0; func_L00_002293E8(); near(HF(0xA90),0.6f);
    reset(0,1,0.1f); func_L00_002293E8(); near(HF(0xA90),1.4f);
    puts("hero gait: phase transfer, gates, hysteresis and rates passed");
    return 0;
}
