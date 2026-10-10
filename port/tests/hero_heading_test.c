/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MACRO_ADDR
#include "../game/rac1-pal/hand/level_hero_heading.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
#define OFFSET(f,o) _Static_assert(offsetof(HeroHeading,f)==o,"hero " #f)
OFFSET(velocity,0xE0); OFFSET(previous,0x130); OFFSET(height,0x168);
OFFSET(angle,0x180); OFFSET(speed,0x190); OFFSET(heightDelta,0x194);
OFFSET(boost,0x257); OFFSET(airborne,0x30A); OFFSET(modeFlag,0x4A4);
OFFSET(projection,0x920); OFFSET(state,0x2084); OFFSET(movement,0x208C);
OFFSET(mode,0x20B3);
HeroHeading D_0013F450;
float D_0015EE6C;
static int scaled, projected, damping_calls, trig_calls;
static float damping_target, damping_amount;
static void near(float a,float b) { CHECK(fabsf(a-b)<0.00001f); }
float func_001F9F90(float a) { ++trig_calls; return cosf(a); }
float func_001F9FA8(float a) { ++trig_calls; return sinf(a); }
float func_001F9CE8(void *p) {
    float *v=p; return sqrtf(v[0]*v[0]+v[1]*v[1]);
}
void func_001F9BF0(void *p,void *a,void *b) {
    float *out=p,*x=a,*y=b;
    for(int i=0;i<3;++i) out[i]=x[i]-y[i];
    memcpy(out+3,x+3,4);
}
void func_001F9BD8(void *p,void *a,void *b) {
    float *out=p,*x=a,*y=b;
    for(int i=0;i<3;++i) out[i]=x[i]+y[i];
    memcpy(out+3,x+3,4);
}
void func_L00_001FF500(float *out,float *a,float length) {
    ++scaled;
    float n=func_001F9CE8(a), factor=n==0?0:length/n;
    out[0]=a[0]*factor; out[1]=a[1]*factor;
    memcpy(out+2,a+2,8);
}
float func_L00_00213A08(HeroHeadingVector *p) {
    ++projected;
    CHECK(p!=&D_0013F450.projection);
    CHECK(memcmp(p,&D_0013F450.projection,sizeof *p)==0);
    CHECK(memcmp(D_0013F450.previous,D_0013F450.velocity,16)==0);
    p->v[0]=99; /* Must be a private copy. */
    return 2;
}
float func_L00_00234250(float *p) {
    CHECK(damping_calls++==0 && p==D_0013F450.velocity);
    return 5;
}
float func_00214D28(float *p,float target,float amount) {
    CHECK(damping_calls++==1); near(*p,5);
    damping_target=target; damping_amount=amount;
    *p=4.75f; return 0;
}
void func_L00_00234420(float *out,float *in,float z) {
    CHECK(damping_calls++==2 && out==D_0013F450.velocity && in==out);
    near(z,0); out[2]=0;
}
void func_L00_00233F88(void *out,void *in,float value) {
    CHECK(damping_calls++==3 && out==D_0013F450.velocity && in==out);
    near(value,4.75f); near(((float *)in)[2],0);
}
static void reset(float vx,float vy,float speed) {
    memset(&D_0013F450,0,sizeof D_0013F450);
    D_0013F450.velocity[0]=vx; D_0013F450.velocity[1]=vy;
    D_0013F450.velocity[2]=7; D_0013F450.velocity[3]=123;
    D_0013F450.speed=speed; D_0013F450.height=10;
    D_0013F450.heightDelta=-9;
    D_0013F450.projection=(HeroHeadingVector){{1,2,3,4}};
    D_0015EE6C=2;
    scaled=projected=damping_calls=trig_calls=0;
}
static void result(float x,float y,int scales) {
    near(D_0013F450.velocity[0],x); near(D_0013F450.velocity[1],y);
    near(D_0013F450.velocity[2],7); near(D_0013F450.velocity[3],123);
    near(D_0013F450.heightDelta,8); near(D_0013F450.projection.v[0],1);
    CHECK(scaled==scales && projected==1 && damping_calls==0 && trig_calls==2);
}
int main(void) {
    /* Same, opposite, orthogonal and zero horizontal velocities exercise
     * the angular gain limits without including vertical speed in lengths. */
    reset(1,0,2); func_L00_00214520(0.1f); result(1.1f,0,1);
    reset(-1,0,1); func_L00_00214520(0.1f); result(-0.7f,0,1);
    reset(-1,0,1); D_0013F450.state=0x81;
    func_L00_00214520(0.1f); result(-0.83f,0,1);
    reset(0,1,1); func_L00_00214520(0.1f);
    result(0.10606602f,0.89393398f,1);
    reset(0,0,2); func_L00_00214520(0.1f); result(0.15f,0,1);
    reset(1,0,0); func_L00_00214520(0.1f); result(0.85f,0,1);
    reset(0,0,0); func_L00_00214520(0.1f); result(0,0,0);
    reset(1,0,2); func_L00_00214520(1); result(2,0,0); /* equality */
    reset(1,0,2); func_L00_00214520(0); result(1,0,1);
    reset(0,0,2); D_0013F450.angle=1.570796327f;
    func_L00_00214520(100); result(0,2,0);
    /* Each boost gate independently matters. */
    for(int gate=0;gate<5;++gate) {
        reset(0,0,1); D_0013F450.movement=4;
        D_0013F450.modeFlag=1; D_0013F450.boost=1;
        if(gate==1) D_0013F450.movement=3;
        if(gate==2) D_0013F450.airborne=1;
        if(gate==3) D_0013F450.modeFlag=0;
        if(gate==4) D_0013F450.boost=0;
        func_L00_00214520(100); result(gate==0?4.2f:1,0,0);
    }
    for(int gate=0;gate<4;++gate) {
        reset(0,0,3); D_0013F450.airborne=1; D_0013F450.state=14;
        if(gate==1) D_0013F450.airborne=0;
        if(gate==2) D_0013F450.state=13;
        if(gate==3) D_0013F450.speed=1;
        func_L00_00214520(100); result(gate==0?2:gate==3?1:3,0,0);
    }
    for(int mode=1;mode<=2;++mode) {
        reset(3,4,2); D_0013F450.mode=(unsigned char)mode;
        func_L00_00214520(0.25f);
        CHECK(damping_calls==4 && projected==0 && scaled==0 && trig_calls==0);
        near(damping_target,2); near(damping_amount,0.25f);
        near(D_0013F450.heightDelta,-9); near(D_0013F450.previous[0],0);
    }
    for(int mode=3;mode<=255;mode+=252) {
        reset(3,4,2); D_0013F450.mode=(unsigned char)mode;
        HeroHeading before=D_0013F450; func_L00_00214520(0.25f);
        CHECK(memcmp(&before,&D_0013F450,sizeof before)==0);
        CHECK(damping_calls==0 && projected==0 && scaled==0 && trig_calls==0);
    }
    puts("hero heading: offsets, turn gain, caps, projection and damping passed");
    return 0;
}
