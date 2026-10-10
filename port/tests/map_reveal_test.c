/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../game/rac1-pal/hand/level_map_reveal.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(MapRevealZone) == 16, "EE zone");
_Static_assert(offsetof(MapRevealHero, active) == 0x308, "EE active");
_Static_assert(offsetof(MapRevealHero, water) == 0x12E4, "EE water");
_Static_assert(offsetof(MapRevealHero, moby) == 0x2080, "EE moby");
_Static_assert(offsetof(MapRevealHero, mode) == 0x208C, "EE mode");
_Static_assert(offsetof(MapRevealState, fog) == 0xC, "EE fog");
_Static_assert(offsetof(MapRevealState, tile) == 0x38, "EE cache head");
_Static_assert(offsetof(MapRevealState, brush) == 0x1A4, "EE brush");
_Static_assert(sizeof(MapRevealPredicate[8]) == 32, "EE callbacks");
#endif
int mr_ready, mr_alternate, mr_level, mr_switches[16];
unsigned char mr_hero[0x5000];
MapRevealZone mr_zones[19][16];
MapRevealPredicate mr_predicates[20][8];
MapRevealState mr_map;
static MapRevealHero* hero;
static MapRevealMoby moby;
static unsigned char fog_storage[32768 + 32], tiles[256][512];
static float point[3] = {12.5f, -23.25f, 42.0f}, projection[2];
static int projects, projected_level, loads, callback_count, callback_order[8], reject;
static int center_x, center_y;
void mr_project(float x, float y, float* ox, float* oy, int level) {
    CHECK(x == point[0] && y == point[1]); ++projects; projected_level = level;
    *ox = projection[0]; *oy = projection[1];
}
int mr_load_tile(int cell) {
    CHECK(cell >= 0 && cell < 256); ++loads;
    mr_map.tile = tiles[cell];
    return 0; /* A cache hit still supplies the tile; zero is not failure. */
}
static int predicate_result(int n, float x, float y, float z, int gx, int gy) {
    CHECK(x == point[0] && y == point[1] && z == point[2]);
    CHECK(gx == center_x && gy == center_y && callback_count < 8);
    callback_order[callback_count++] = n;
    return n != reject;
}
#define PREDICATE(n) static int cb##n(int gx, int gy, float x, float y, float z) { return predicate_result(n,x,y,z,gx,gy); }
PREDICATE(0) PREDICATE(1) PREDICATE(2) PREDICATE(3)
PREDICATE(4) PREDICATE(5) PREDICATE(6) PREDICATE(7)
static MapRevealPredicate callbacks[8] = {cb0,cb1,cb2,cb3,cb4,cb5,cb6,cb7};
int mr_height_a(int gx, float x, float y, float z) { return predicate_result(0,x,y,z,gx,center_y); }
int mr_height_b(int gx, float x, float y, float z) { return predicate_result(1,x,y,z,gx,center_y); }
int mr_height_c(int gx, float x, float y, float z) { return predicate_result(2,x,y,z,gx,center_y); }
int mr_height_d(int gx, float x, float y, float z) { return predicate_result(3,x,y,z,gx,center_y); }
static void reset(int gx, int gy, int zone) {
    memset(mr_hero, 0, sizeof(mr_hero)); memset(&mr_map, 0, sizeof(mr_map));
    memset(mr_zones, 0, sizeof(mr_zones)); memset(mr_predicates, 0, sizeof(mr_predicates));
    memset(mr_switches, 0, sizeof(mr_switches)); memset(&moby, 0, sizeof(moby));
    memset(fog_storage, 0xFF, sizeof(fog_storage));
    memset(tiles, zone * 17, sizeof(tiles));
    hero = (MapRevealHero*)(mr_hero + 0xE1D); hero->moby = &moby;
    mr_map.fog = fog_storage + 16; mr_map.tile = NULL;
    mr_ready = 1; mr_level = 2; mr_alternate = 0;
    projects = loads = callback_count = 0; reject = -1;
    center_x = gx; center_y = gy;
    projection[0] = (float)(gx - 1) / 512.0f;
    projection[1] = (float)(gy - 1) / 512.0f;
}
static int hidden(int x, int y) { return !!(mr_map.fog[y*64+x/8] & (1 << (x&7))); }
static void check_guards(void) {
    for (int i=0; i<16; ++i) CHECK(fog_storage[i] == 255 && fog_storage[32784+i] == 255);
}
static void single_pixel(void) {
    memset(mr_map.brush, 255, sizeof(mr_map.brush));
    mr_map.brush[16*4+2] &= (unsigned char)~1u;
}
static void run_expect(int reveal) {
    single_pixel(); func_L00_00248EF8(point);
    CHECK(hidden(center_x, center_y) == !reveal); CHECK(loads == 1); check_guards();
}
static void filters(void) {
    static const unsigned int bits[] = {1,2,4,8,16,32,64,128};
    for (int i=0; i<8; ++i) for (int state=0; state<2; ++state) {
        reset(32,32,1); mr_zones[2][1].flags = bits[i];
        if (i < 2) hero->mode = state ? 0x11 : 0;
        if (i == 2 || i == 3) hero->mode = state ? 0x10 : 0;
        if (i == 4 || i == 5) hero->mode = state ? 0xF : 0;
        if (i == 6) hero->active = state ? -1 : 0;
        if (i == 7) { mr_zones[2][1].switch_index = 5; mr_switches[5] = state; }
        run_expect((i == 1 || i == 3 || i == 5) ? !state : state);
    }
    reset(32,32,1); mr_zones[2][1].flags=1; hero->mode=0x12; run_expect(1);
    reset(32,32,1); mr_zones[2][1].flags=1; hero->water=1; run_expect(1);
    reset(32,32,1); mr_zones[2][1].flags=1; hero->water=2; run_expect(0);
    reset(32,32,1); mr_zones[2][1].flags=3; hero->water=1; run_expect(0);
    for (int h=9; h<=21; ++h) {
        reset(32,32,1); mr_zones[2][1].low=10; mr_zones[2][1].high=20;
        moby.height=(float)h; point[2]=500; run_expect(h>=10 && h<=20);
    }
    point[2]=42;
    reset(32,32,1); mr_zones[2][1].low=mr_zones[2][1].high=100; hero->moby=NULL; run_expect(1);
    reset(32,32,0); mr_zones[2][0].flags=0xFFFF; hero->moby=NULL; run_expect(1);
    /* Only descriptors are clamped; projection receives the real level. */
    reset(32,32,1); mr_level=-1; mr_zones[0][1].flags=1; run_expect(0); CHECK(projected_level==-1);
    reset(32,32,1); mr_level=19; mr_zones[0][1].flags=1; run_expect(0); CHECK(projected_level==19);
}
static void predicates(void) {
    MapRevealPredicate height_callbacks[4] = {(MapRevealPredicate)mr_height_a,
        (MapRevealPredicate)mr_height_b, (MapRevealPredicate)mr_height_c, (MapRevealPredicate)mr_height_d};
    for (int i=0; i<4; ++i) for (int deny=0; deny<2; ++deny) {
        reset(32,33,1); mr_zones[2][1].flags=0x100;
        mr_predicates[2][0]=height_callbacks[i]; reject=deny?i:-1; run_expect(!deny);
        CHECK(callback_count==1 && callback_order[0]==i);
    }
    for (int i=0; i<8; ++i) for (int deny=0; deny<2; ++deny) {
        reset(32,32,1); mr_zones[2][1].flags=0x100u<<i;
        mr_predicates[2][i]=callbacks[i]; reject=deny?i:-1; run_expect(!deny);
        CHECK(callback_count==1 && callback_order[0]==i);
    }
    for (int stop=-1; stop<8; ++stop) {
        reset(32,32,1); mr_zones[2][1].flags=0xFF00;
        memcpy(mr_predicates[2],callbacks,sizeof(callbacks)); reject=stop; run_expect(stop<0);
        CHECK(callback_count==(stop<0?8:stop+1));
        for (int i=0; i<callback_count; ++i) CHECK(callback_order[i]==i);
    }
    reset(32,32,1); mr_zones[2][1].flags=0xFF00; run_expect(1); CHECK(!callback_count);
    reset(32,32,1); mr_level=19; mr_zones[0][1].flags=0x100;
    mr_predicates[19][0]=cb0; reject=0; run_expect(0); CHECK(callback_count==1);
    reset(32,32,1); mr_zones[2][1].flags=0x100; mr_zones[2][1].low=5;
    mr_zones[2][1].high=6; mr_predicates[2][0]=cb0; run_expect(0); CHECK(!callback_count);
}
static void geometry(void) {
    static const int centers[][2]={{0,0},{1,1},{16,16},{31,32},{32,31},{256,256},{495,495},{510,510},{511,511}};
    for (unsigned int c=0; c<sizeof(centers)/sizeof(centers[0]); ++c) {
        int gx=centers[c][0], gy=centers[c][1], expected_loads=0;
        reset(gx,gy,0);
        /* Every zone and both packed nibbles occur, with different tile data. */
        for (int cell=0; cell<256; ++cell) for (int y=0; y<32; ++y) for (int x=0; x<32; x+=2) {
            int a=(x+3*y+cell)%16, b=(x+1+3*y+cell)%16;
            tiles[cell][y*16+x/2]=(unsigned char)(a | b<<4);
        }
        for (int z=1; z<16; ++z) mr_zones[2][z].flags=(z%3)?1:0;
        for (int y=0; y<32; ++y) for (int x=0; x<32; ++x)
            if ((x-15)*(x-15)+(y-15)*(y-15)>200) mr_map.brush[y*4+x/8] |= (unsigned char)(1<<(x&7));
        /* An already revealed bit must neither be reset nor load a tile. */
        mr_map.fog[gy*64+gx/8] &= (unsigned char)~(1u<<(gx&7));
        func_L00_00248EF8(point);
        for (int y=0; y<512; ++y) for (int x=0; x<512; ++x) {
            int bx=x-gx+16, by=y-gy+16, reveal=(x==gx && y==gy);
            if (x<511 && y<511 && bx>=0 && bx<32 && by>=0 && by<32 &&
                (bx-15)*(bx-15)+(by-15)*(by-15)<=200 && !reveal) {
                int cell=x/32+(y/32)*16, zone=(x%32+3*(y%32)+cell)%16;
                ++expected_loads; reveal=(zone%3==0);
            }
            CHECK(hidden(x,y)==!reveal);
        }
        CHECK(loads==expected_loads); check_guards();
    }
}
static void projection_bounds(void) {
    static const float invalid[]={-2.0f,511.0f,-10000.0f,10000.0f,INFINITY,-INFINITY,NAN};
    reset(32,32,0); mr_ready=0; func_L00_00248EF8(NULL); CHECK(!projects && !loads);
    for (unsigned int i=0; i<sizeof(invalid)/sizeof(invalid[0]); ++i) for (int axis=0; axis<2; ++axis) {
        reset(32,32,0); projection[axis]=invalid[i]/512; func_L00_00248EF8(point);
        CHECK(projects==1 && !loads); check_guards();
    }
    reset(0,0,0); projection[0]=-1.75f/512; projection[1]=-1.25f/512;
    mr_alternate=0x10000; func_L00_00248EF8(point); CHECK(projected_level==102 && loads==256);
    reset(511,511,0); projection[0]=510.75f/512; projection[1]=510.5f/512;
    func_L00_00248EF8(point); CHECK(loads==256 && hidden(511,510) && hidden(510,511));
}
int main(void) {
    filters(); predicates(); geometry(); projection_bounds();
    puts("map reveal: OK"); return 0;
}
