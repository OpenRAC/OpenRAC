/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/level_particle53.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
#define F(o) (*(float *)(particle+(o)))
#define S(o) (*(short *)(particle+(o)))
#define U(o) (*(unsigned *)(particle+(o)))
_Alignas(16) static unsigned char particle[0x40];
static int kills, ticks, converts, conversion_input;
float func_001FA888(int n) { ++converts; conversion_input=n; return (float)n; }
int func_001FA898(float f) { return (int)f; }
void func_001F9BD8(void *o,void *a,void *b) {
    float *out=o,*x=a,*y=b;
    CHECK(out==(float *)(particle+0x10) && x==out && y[3]==0);
    for(int i=0;i<3;++i) out[i]=x[i]+y[i];
    memcpy(out+3,x+3,4);
}
void func_L00_002688A8(void *p) { CHECK(p==particle); ++kills; }
int func_001F9938(void *p) {
    CHECK(p==particle+0xA); ++ticks;
    short *v=p;
    if(*v==0) return 1;
    *v=(short)(*v<1?0:*v-1);
    return *v>0?0:2;
}
static void near(float a,float b) { CHECK(fabsf(a-b)<0.00001f); }
static void reset(int initial,int remaining) {
    memset(particle,0,sizeof particle);
    U(0)=0x12345678; U(4)=0xAB123456; particle[8]=250; particle[0x2A]=10;
    S(0x28)=(short)initial; S(0xA)=(short)remaining;
    F(0x20)=2; F(0x24)=4; particle[0x2B]=255;
    F(0x10)=100; F(0x14)=200; F(0x18)=300; F(0x1C)=7;
    F(0x2C)=1; F(0x30)=-2; F(0x34)=3; F(0x38)=0.5f;
    kills=ticks=converts=0; conversion_input=-999;
}
static void run(void) {
    unsigned char before[sizeof particle]; memcpy(before,particle,sizeof particle);
    func_L00_00273090((char *)particle);
    /* Only the retail update fields may change. */
    for(unsigned i=0;i<sizeof particle;++i) {
        if(i==7 || i==8 || (i>=0xA && i<0x1C) || (i>=0x34 && i<0x38)) continue;
        CHECK(particle[i]==before[i]);
    }
    CHECK((U(4)&0xFFFFFF)==0x123456 && U(0)==0x12345678);
}
int main(void) {
    reset(20,20); run(); near(F(0xC),4); CHECK((U(4)>>24)==255);
    CHECK(particle[8]==4 && S(0xA)==19 && ticks==1 && kills==0 && converts==0);
    near(F(0x10),101); near(F(0x14),198); near(F(0x18),303); near(F(0x34),2.5f);
    run(); near(F(0x18),305.5f); near(F(0x34),2); near(F(0x1C),7);
    reset(20,15); run(); near(F(0xC),3); CHECK((U(4)>>24)==127 && converts==0);
    reset(20,14); run(); near(F(0xC),3); CHECK((U(4)>>24)==127 && converts==1 && conversion_input==14);
    reset(20,7); run(); near(F(0xC),2.5f); CHECK((U(4)>>24)==63 && kills==0);
    reset(20,1); run(); near(F(0xC),2+1.0f/14); CHECK((U(4)>>24)==9 && kills==1 && S(0xA)==0);
    reset(20,0); run(); near(F(0xC),2); CHECK((U(4)>>24)==0 && kills==1 && ticks==1);
    reset(-2,-4); run(); near(F(0xC),3.6f); CHECK(kills==1 && S(0xA)==0 && converts==0);
    reset(20,18); particle[0x2B]=199; run(); CHECK((U(4)>>24)==159);
    reset(20,20); particle[8]=1; particle[0x2A]=255; run(); CHECK(particle[8]==0);
    for(int axis=0;axis<3;++axis) {
        float values[]={2,1021,1.999f,1021.001f};
        for(int k=0;k<4;++k) {
            reset(20,20); F(0x2C)=F(0x30)=F(0x34)=0;
            F(0x10+axis*4)=values[k]; run();
            CHECK(kills==(k>=2) && ticks==(k<2));
            CHECK(S(0xA)==(k<2?19:20)); near(F(0x34),-0.5f);
        }
    }
    reset(20,20); F(0x10)=2; F(0x2C)=-0.25f; run(); CHECK(kills==1 && ticks==0);
    reset(20,20); F(0x10)=NAN; run(); CHECK(kills==0 && ticks==1);
    puts("particle 53: fade phases, byte wrap, drift, bounds and lifetime passed");
    return 0;
}
