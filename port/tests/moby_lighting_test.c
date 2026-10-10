/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/level_moby_lighting.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
_Static_assert(sizeof(MobyLightRegion)==128, "region stride");
_Static_assert(offsetof(MobyLightRegion, colors)==0x40, "region colors");
_Static_assert(offsetof(MobyLightRegion, flags)==0x50, "region flags");
_Static_assert(sizeof(MobyPointLight)==32, "point stride");
_Static_assert(offsetof(MobyLightObject, position)==0x10, "moby position");
_Static_assert(offsetof(MobyLightObject, lights)==0x38, "moby lights");
_Static_assert(offsetof(MobyLightObject, color)==0x3C, "moby color");
_Static_assert(offsetof(MobyLightObject, ambient)==0x80, "moby ambient");
int ml_last;
float ml_bounds[4][4];
MobyLightRegion ml_regions[4];
static unsigned int grid[4200];
static MobyPointLight points[4];
unsigned int* ml_grid=grid;
MobyPointLight* ml_points=points;
static MobyLightObject object;
static void reset(void) {
    memset(ml_bounds,0,sizeof(ml_bounds)); memset(ml_regions,0,sizeof(ml_regions));
    memset(grid,0,sizeof(grid)); memset(points,0,sizeof(points)); memset(&object,0xA5,sizeof(object));
    ml_last=0; object.position[0]=object.position[1]=object.position[2]=0;
    object.position[3]=99; object.ambient=0x80402010; object.color=0; object.lights=0x12345678;
}
static void region(int i) {
    ml_bounds[i][3]=100;
    ml_regions[i].matrix[0]=ml_regions[i].matrix[5]=ml_regions[i].matrix[10]=1;
    ml_regions[i].flags=1; ml_regions[i].colors[0]=ml_regions[i].colors[1]=0xFFFFFFFF;
    ml_regions[i].lights[0]=3; ml_regions[i].lights[1]=7;
}
static void list(int cell, int count, int a, int b) {
    grid[cell]=4096*4; grid[4096]=(unsigned int)count;
    grid[4097]=(unsigned int)a; grid[4098]=(unsigned int)b;
}
static void regions(void) {
    float blend=123; int index=99;
    reset(); CHECK(!func_L00_00257E18(object.position,&blend,&index)); CHECK(blend==123 && index==99);
    /* The scan's end is inclusive, even when the count global is zero. */
    region(0); CHECK(func_L00_00257E18(object.position,&blend,&index)); CHECK(index==0 && blend==0.5f);
    reset(); ml_last=2; region(2);
    CHECK(func_L00_00257E18(object.position,&blend,&index)); CHECK(index==2);
    for (int axis=0; axis<3; ++axis) for (int side=-1; side<=1; side+=2) {
        reset(); region(0); object.position[axis]=(float)side;
        CHECK(func_L00_00257E18(object.position,&blend,&index));
        if (!axis) CHECK(blend==(side<0?0:1));
        object.position[axis]=(float)side*1.01f; blend=123; index=99;
        CHECK(!func_L00_00257E18(object.position,&blend,&index)); CHECK(blend==123 && index==99);
    }
    reset(); region(0); region(1); ml_last=1; ml_regions[0].matrix[12]=2;
    CHECK(!func_L00_00257E18(object.position,&blend,&index)); /* no retry after a clipped broad hit */
    reset(); region(0); object.position[0]=10;
    ml_regions[0].matrix[0]=0; CHECK(!func_L00_00257E18(object.position,&blend,&index)); /* strict radius */
    reset(); region(0); object.position[0]=0.25f; object.position[1]=0.5f; object.position[2]=-0.25f;
    ml_regions[0].matrix[0]=2; ml_regions[0].matrix[4]=0.5f; ml_regions[0].matrix[8]=1;
    ml_regions[0].matrix[12]=0.25f;
    CHECK(func_L00_00257E18(object.position,&blend,&index)); CHECK(blend==0.875f);
}
static void point_queries(void) {
    float d=123;
    reset(); CHECK(!func_L00_00257F4C(object.position,&d)); CHECK(d==123);
    list(0,1,2,0); points[2].radius_squared=100; points[2].position[2]=10000;
    CHECK(func_L00_00257F4C(object.position,&d)==&points[2]); CHECK(d==0); /* XY only */
    points[2].position[0]=10; d=123;
    CHECK(!func_L00_00257F4C(object.position,&d)); CHECK(d==123); /* equal radius is outside */
    list(0,2,2,1); points[1].radius_squared=30; points[1].position[0]=3; points[1].position[1]=4;
    CHECK(func_L00_00257F4C(object.position,&d)==&points[1] && d==25);
    points[2].radius_squared=101;
    CHECK(func_L00_00257F4C(object.position,&d)==&points[2] && d==100); /* first, not nearest */
    reset(); object.position[0]=31.9f; object.position[1]=47.9f;
    list(129,1,1,0); points[1].position[0]=31.9f; points[1].position[1]=47.9f; points[1].radius_squared=1;
    CHECK(func_L00_00257F4C(object.position,&d)==&points[1]);
    reset(); object.position[0]=-0.9f; list(0,1,0,0); points[0].radius_squared=2;
    CHECK(func_L00_00257F4C(object.position,&d)==&points[0]); /* truncate before logical shift */
}
static void colors(void) {
    unsigned char original[sizeof(object)];
    reset(); memcpy(original,&object,sizeof(object)); func_L00_0025805C(&object);
    CHECK(object.color==0x80402010 && object.ambient==0x80402010 && object.lights==0x12345678);
    for (unsigned int i=0; i<sizeof(object); ++i)
        if (!(i>=0x3C && i<0x40)) CHECK(((unsigned char*)&object)[i]==original[i]);
    reset(); region(0); func_L00_0025805C(&object);
    CHECK(object.ambient==0xFDFDFDFD && object.color==0xFDFDFDFD && object.lights==0x007F0703);
    /* PADDUB does not carry between product bytes: ordinary lerp gives 254. */
    reset(); region(0); ml_regions[0].matrix[12]=-1; func_L00_0025805C(&object);
    CHECK(object.color==0xFEFEFEFE && object.lights==0x00000703);
    reset(); region(0); object.position[0]=1; func_L00_0025805C(&object);
    CHECK(object.color==0xFEFEFEFE && object.lights==0x00FF0703);
    reset(); region(0); ml_regions[0].flags=2; func_L00_0025805C(&object);
    CHECK(object.ambient==0x80402010 && object.color==0x80402010 && object.lights==0x12345678);
    reset(); region(0); ml_regions[0].colors[0]=0xFF804020; ml_regions[0].colors[1]=0x204080FF;
    func_L00_0025805C(&object); CHECK(object.color==0x8E5F5F8E);
    reset(); list(0,1,0,0); points[0].radius_squared=100; points[0].color=0x80808080;
    object.ambient=0x01010101; func_L00_0025805C(&object); CHECK(object.color==0x80808080);
    CHECK(object.ambient==0x01010101 && object.lights==0x12345678);
    points[0].position[0]=5; func_L00_0025805C(&object); CHECK(object.color==0x61616161);
    points[0].position[0]=10; func_L00_0025805C(&object); CHECK(object.color==0x01010101);
    reset(); list(0,1,0,0); points[0].radius_squared=100; points[0].color=0xC8C8C8C8;
    object.ambient=0xC8C8C8C8; func_L00_0025805C(&object); CHECK(object.color==0xFFFFFFFF);
    reset(); region(0); list(0,1,0,0); points[0].radius_squared=100; points[0].color=0x01020304;
    func_L00_0025805C(&object); CHECK(object.ambient==0xFDFDFDFD && object.color==0xFDFEFFFF);
}
int main(void) {
    regions(); point_queries(); colors(); puts("moby lighting: OK"); return 0;
}
