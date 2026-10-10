/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MACRO_ADDR
#include "../game/rac1-pal/hand/level_hero_lean.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
#define HF(o) (*(float *)(D_0013F450 + (o)))
#define HI(o) (*(int *)(D_0013F450 + (o)))
#define MF(o) (*(float *)(D_L00_0017A780 + (o)))
_Alignas(16) char D_0013F450[0x2400];
_Alignas(16) char D_L00_0017A780[0xC00];
float D_0015EE6C;
static int setters, trig_calls;
static float settings[4][2];
static void setting(int i, float a, float b) {
    CHECK(i == setters++);
    settings[i][0] = a; settings[i][1] = b;
}
/* Catalogue names fold all four setters. Hostgen recovers each destination
 * from the retail call ordinal; this fixture checks the four parameter pairs. */
void func_L00_0020A858(float a, float b) { CHECK(setters<4); setting(setters,a,b); }
float func_001F9B88(float a) { return fabsf(a); }
float func_L00_001FF860(float a, float b) { ++trig_calls; return atan2f(a,b); }
static float wrap(float a) {
    if (a > 3.14159265f) a -= 6.2831853f;
    else if (a < -3.14159265f) a += 6.2831853f;
    return a;
}
float func_001FA790(float a, float b) { return wrap(a-b); }
float func_001FA748(float a, float b) { return wrap(a+b); }
float func_001F9FA8(float a) { return sinf(a); }
float func_001F9F90(float a) { return cosf(a); }
static void near(float a, float b) { CHECK(fabsf(a-b) < 0.000001f); }
static void reset(int primary, int secondary, int movement, float lean) {
    memset(D_0013F450,0,sizeof D_0013F450);
    for (unsigned i=0;i<sizeof D_L00_0017A780;i+=4) MF(i)=42.0f;
    HI(0x2084)=primary; HI(0x2088)=secondary; HI(0x208C)=movement;
    HF(0x188)=lean; D_0015EE6C=1.0f; setters=trig_calls=0;
}
static void common_settings(int airborne) {
    CHECK(setters==4);
    near(settings[0][0],airborne?0.008f:0.04f);
    near(settings[1][0],airborne?0.015f:0.04f);
    near(settings[2][0],airborne?0.015f:0.04f);
    near(settings[3][0],airborne?0.008f:0.02f);
    for(int i=0;i<4;i++) near(settings[i][1],airborne?(i==3?0.1f:0.08f):0.2f);
    near(MF(0),42); near(MF(0xBFC),42); /* untouched boundaries */
}
int main(void) {
    /* Primary 4 alone must not select the fallback: its selector is +208C. */
    reset(4,0,0,2); func_L00_00215A90(); CHECK(setters==0);
    for(unsigned i=0;i<sizeof D_L00_0017A780;i+=4) near(MF(i),42);
    for(int movement=2;movement<=4;movement+=2) {
        reset(0,0,movement,10); func_L00_00215A90(); common_settings(0);
        near(MF(0x118),0.406f); near(MF(0x278),1.218f);
        near(MF(0x958),-0.49f); near(MF(0x1C0),-0.378f);
        near(MF(0xA08),-0.42f); near(MF(0x60),42);
    }
    reset(0,0,4,-10); func_L00_00215A90(); near(MF(0x118),-0.406f);
    /* Walking branch: direction clamp, speed factor bounds, and state priority. */
    for(int sign=-1;sign<=1;sign+=2) {
        reset(2,1,4,sign*10.0f); HF(0x164)=2.85f;
        func_L00_00215A90(); common_settings(0);
        near(MF(0x60),sign*-0.098f); near(MF(0x118),sign*0.224f);
        near(MF(0x274),-0.21f); near(MF(0x270),0.224f);
        near(MF(0x954),0.49f); near(MF(0xA04),0.28f);
        near(MF(0xB68),sign*-0.42f); near(MF(0xAB8),MF(0xB68));
    }
    reset(2,1,0,1); HF(0x164)=-1; func_L00_00215A90(); near(MF(0x60),0);
    reset(2,1,0,1); HF(0x164)=20; func_L00_00215A90(); near(MF(0x60),-0.14f);
    /* Retail nonlinear gain is 1.25, not the candidate's 2.0. */
    reset(2,0,0,1); func_L00_00215A90(); common_settings(0);
    near(MF(0x118),0.3125f); near(MF(0x274),0.078125f); near(MF(0x60),42);
    reset(2,0,0,-10); func_L00_00215A90(); near(MF(0x118),-0.8f);
    /* Airborne lean caps several independent destinations. */
    for(int sign=-1;sign<=1;sign+=2) {
        reset(8,0,0,sign*10.0f); func_L00_00215A90(); common_settings(1);
        near(MF(0x60),sign*-0.28f); near(MF(0x278),sign*0.6f);
        near(MF(0x118),sign*0.725f); near(MF(0x274),0.2f);
        near(MF(0x270),0.2f); near(MF(0x958),sign*-0.4375f); CHECK(trig_calls==0);
    }
    /* 0x81 applies the angular offset even below/equal the speed threshold. */
    reset(0x81,0,0,10); HF(0x164)=0.5f; MF(0x64)=0.3f;
    func_L00_00215A90(); common_settings(1); CHECK(trig_calls==0);
    near(MF(0x118),0.638f); near(MF(0x958),-0.385f); near(MF(0x64),0.17782695f);
    /* Above the threshold, velocity heading overrides the torso lean. */
    reset(0x81,0,0,0); HF(0x164)=1; HF(0x100)=1; HF(0x104)=0;
    func_L00_00215A90(); CHECK(trig_calls==1);
    near(MF(0x60),-0.436332315f); near(MF(0x64),-0.12217305f);
    { uint32_t bits; memcpy(&bits,D_L00_0017A780+0x60,4); CHECK(bits==0xBEDF66F3u); }
    reset(0x81,0,0,0); HF(0x164)=1; HF(0x104)=-1;
    func_L00_00215A90(); near(MF(0x64),-0.436332315f-0.12217305f);
    reset(8,0,0,0); func_L00_00215A90(); near(MF(0x60),0); near(MF(0x278),0);
    puts("hero lean: all branch, clamp, selector and setter checks passed");
    return 0;
}
