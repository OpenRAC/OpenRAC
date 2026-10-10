/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/level_planar_limit.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
float openrac_sqrt(float value) { return sqrtf(value); }
static void near(float a,float b) { CHECK(fabsf(a-b)<0.000001f); }
int main(void) {
    float in[4]={3,4,1000,0}, out[4]={91,92,93,94}, saved[4];
    unsigned z=0x80000000u, w=0x7FC12345u;
    memcpy(in+2,&z,4); memcpy(in+3,&w,4);
    memcpy(saved,out,16);
    CHECK(func_L00_001FF5B0(6,out,in)==0 && memcmp(out,saved,16)==0);
    CHECK(func_L00_001FF5B0(5,out,in)==1 && memcmp(out,in,16)==0);
    CHECK(func_L00_001FF5B0(2.5f,out,in)==1);
    near(out[0],1.5f); near(out[1],2); CHECK(memcmp(out+2,in+2,8)==0);
    CHECK(func_L00_001FF5B0(0,out,in)==1); near(out[0],0); near(out[1],0);
    CHECK(func_L00_001FF5B0(-5,out,in)==1); near(out[0],-3); near(out[1],-4);
    CHECK(func_L00_001FF5B0(-0.0f,out,in)==1);
    CHECK(signbit(out[0]) && signbit(out[1]));
    float zero[4]={0,-0.0f,90,91};
    memcpy(saved,out,16);
    CHECK(func_L00_001FF5B0(0,out,zero)==0 && memcmp(out,saved,16)==0);
    CHECK(func_L00_001FF5B0(-1,out,zero)==0 && memcmp(out,saved,16)==0);
    CHECK(func_L00_001FF5B0(100,out,zero)==0 && memcmp(out,saved,16)==0);
    CHECK(func_L00_001FF5B0(2.5f,in,in)==1); near(in[0],1.5f); near(in[1],2);
    CHECK(memcmp(in+2,&z,4)==0 && memcmp(in+3,&w,4)==0);
    /* All source lanes are loaded before any output write, as with lqc2. */
    float overlap[5]={3,4,0,0,99};
    memcpy(overlap+2,&z,4); memcpy(overlap+3,&w,4);
    CHECK(func_L00_001FF5B0(2.5f,overlap+1,overlap)==1);
    near(overlap[1],1.5f); near(overlap[2],2);
    CHECK(memcmp(overlap+3,&z,4)==0 && memcmp(overlap+4,&w,4)==0);
    /* Next representable limits distinguish strict rejection from equality. */
    float axis[4]={5,0,17,19};
    memcpy(saved,out,16);
    CHECK(func_L00_001FF5B0(nextafterf(5,INFINITY),out,axis)==0);
    CHECK(memcmp(out,saved,16)==0);
    CHECK(func_L00_001FF5B0(nextafterf(5,0),out,axis)==1);
    CHECK(out[0]<5 && out[2]==17 && out[3]==19);
    puts("planar limit: no-write paths, equality, aliases and copied lane bits passed");
    return 0;
}
