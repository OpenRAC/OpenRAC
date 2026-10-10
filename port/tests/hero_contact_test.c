/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MACRO_ADDR
#include "../game/rac1-pal/hand/level_hero_contact.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
_Alignas(16) unsigned char D_0013F450[0x2400];
static HeroContactEntry entries[3];
HeroContactEntry *D_L00_0015F7EC=entries;
int D_L00_0015F7F0;
static int surfaces[3], calls, distance_calls, fail_first, mutate_count;
static float dx, dz, origin[4], point[4];
static int *surface_out, segment_out, index_out;
static float fraction_out;
float func_001F9D10(void *a,void *b) {
    ++distance_calls;
    float *x=a,*y=b;
    float dx_=x[0]-y[0],dy_=x[1]-y[1],dz_=x[2]-y[2];
    return sqrtf(dx_*dx_+dy_*dy_+dz_*dz_);
}
float func_001F9D48(void *a,void *b) {
    float *x=a,*y=b;
    return sqrtf((x[0]-y[0])*(x[0]-y[0])+(x[1]-y[1])*(x[1]-y[1]));
}
float func_001F9B88(float f) { return fabsf(f); }
int func_L00_0025EFC0(void *s,void *p,void *out,void *seg,void *frac,
                     int index,float a,float b,float c) {
    int n=(int)((int *)s-surfaces);
    CHECK(n>=0 && n<3 && index==40+n);
    CHECK(a==12 && b==10 && c==0 && p!=origin);
    CHECK(memcmp(p,origin,16)==0);
    ++calls;
    if(mutate_count) D_L00_0015F7F0=0;
    float *v=out;
    v[0]=dx; v[1]=0; v[2]=dz; v[3]=7;
    *(int *)seg=70+n; *(float *)frac=0.25f;
    return !(fail_first && n==0);
}
static void reset(void) {
    memset(D_0013F450,0,sizeof D_0013F450);
    memset(entries,0,sizeof entries); memset(origin,0,sizeof origin);
    for(int i=0;i<3;++i) {
        surfaces[i]=1; entries[i].surface=surfaces+i;
        entries[i].radius=2; entries[i].index=40+i;
    }
    D_L00_0015F7F0=1; calls=distance_calls=fail_first=mutate_count=0;
    dx=0.1f; dz=0.2f;
    surface_out=NULL; segment_out=-1; fraction_out=-1; index_out=-1;
}
static int run(int *exclude,int *require) {
    float saved[4]; memcpy(saved,origin,16);
    int hit=func_L00_0020D3A0(origin,&surface_out,point,&segment_out,
                            &fraction_out,&index_out,(int)(uintptr_t)exclude,(int)(uintptr_t)require);
    CHECK(memcmp(origin,saved,16)==0);
    if(!hit) CHECK(surface_out==NULL && segment_out==-1 && fraction_out==-1 && index_out==-1);
    return hit;
}
int main(void) {
    _Static_assert(offsetof(HeroContactEntry,surface)==16,"surface offset");
    if(sizeof(void *)==4) {
        CHECK(sizeof(HeroContactEntry)==32);
        CHECK(offsetof(HeroContactEntry,index)==20);
    }
    reset(); CHECK(run(NULL,NULL));
    CHECK(surface_out==surfaces && segment_out==70 && fraction_out==0.25f && index_out==40);
    reset(); D_L00_0015F7F0=0; CHECK(!run(NULL,NULL) && calls==0);
    reset(); D_L00_0015F7F0=-1; CHECK(!run(NULL,NULL) && calls==0);
    reset(); CHECK(!run(surfaces,NULL) && distance_calls==0);
    reset(); CHECK(!run(NULL,surfaces+1) && distance_calls==0);
    reset(); surfaces[0]=0; CHECK(!run(NULL,NULL) && distance_calls==0);
    reset(); entries[0].position[2]=2; CHECK(run(NULL,NULL)); /* radius equality */
    reset(); entries[0].position[2]=2.01f; CHECK(!run(NULL,NULL) && calls==0);
    reset(); fail_first=1; CHECK(!run(NULL,NULL) && calls==1);
    reset(); fail_first=1; D_L00_0015F7F0=3;
    CHECK(run(NULL,NULL) && calls==2 && surface_out==surfaces+1 && segment_out==71 && index_out==41);
    reset(); D_L00_0015F7F0=3;
    CHECK(run(surfaces,surfaces+2) && calls==1 && surface_out==surfaces+2);
    reset(); D_L00_0015F7F0=3; fail_first=mutate_count=1;
    CHECK(!run(NULL,NULL) && calls==1); /* Count is reloaded after callbacks. */
    for(int mode=0;mode<4;++mode) {
        reset(); *(unsigned *)(D_0013F450+0x208C)=(unsigned)mode;
        dx=mode<2?0.3f:0.9f; CHECK(!run(NULL,NULL));
        dx-=0.0001f; CHECK(run(NULL,NULL));
    }
    reset(); *(unsigned *)(D_0013F450+0x208C)=UINT32_MAX;
    dx=0.8f; CHECK(run(NULL,NULL)); /* Unsigned state comparison. */
    for(int sign=-1;sign<=1;sign+=2) {
        reset(); dz=sign*1.5f; CHECK(!run(NULL,NULL));
        dz=sign*1.499f; CHECK(run(NULL,NULL));
    }
    for(int mode=0;mode<3;mode+=2) {
        reset(); *(unsigned *)(D_0013F450+0x208C)=(unsigned)mode;
        *(short *)(D_0013F450+0x1CA)=-1;
        dx=(mode==0?0.3f:0.9f)+0.5f; CHECK(!run(NULL,NULL));
        dx-=0.0001f; dz=10; CHECK(!run(NULL,NULL));
        dz=9.999f; CHECK(run(NULL,NULL));
    }
    reset(); dx=NAN; CHECK(!run(NULL,NULL));
    reset(); dz=NAN; CHECK(!run(NULL,NULL));
    puts("hero contact: filters, table iteration, strict limits and outputs passed");
    return 0;
}
