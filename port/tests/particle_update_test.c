/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/game_particle_update.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while (0)
_Static_assert(sizeof(NativeParticleUpdate)==64,"EE particle stride");
_Static_assert(offsetof(NativeParticleUpdate,active)==1,"EE active flag");
NativeParticleUpdate* pu_pool;
NativeParticleUpdate* pu_cursor;
NativeParticleUpdate* pu_end;
int pu_high;
void (*pu_callbacks[81])(NativeParticleUpdate*);
static NativeParticleUpdate particles[9];
static int count, seen[16], kinds[16], mode;
static void record(NativeParticleUpdate* p, int kind) {
    CHECK(count<16 && p>=particles && p<particles+9);
    CHECK(pu_cursor==p+1);
    seen[count]=(int)(p-particles); kinds[count++]=kind;
    ++p->payload[0];
}
static void ordinary(NativeParticleUpdate* p) { record(p,0); }
static void other(NativeParticleUpdate* p) { record(p,1); }
static void change(NativeParticleUpdate* p) {
    record(p,2);
    if (mode==1) { pu_cursor=particles+3; pu_end=particles+4; }
    if (mode==2) { pu_high=7; pu_pool=particles+6; }
    if (mode==3) { pu_end=particles+4; particles[3].active=0; }
    if (mode==4) { particles[1].active=-1; particles[2].active=127; }
    if (mode==5) { pu_cursor=pu_end=particles+8; }
}
static void reset(void) {
    memset(particles,0,sizeof(particles));
    for (int i=0;i<81;++i) pu_callbacks[i]=ordinary;
    pu_callbacks[1]=other; pu_callbacks[80]=change;
    pu_pool=particles; pu_high=3; pu_cursor=particles+8; pu_end=particles+7;
    count=mode=0;
}
int main(void) {
    reset(); pu_high=-1; func_00218A80();
    CHECK(!count && pu_end==particles && pu_cursor==particles+8);
    reset(); pu_high=0; func_00218A80(); CHECK(count==1 && seen[0]==0 && pu_end==particles+1);
    reset(); particles[0].active=-128; particles[0].type=-1; particles[2].active=-1;
    particles[1].type=1; particles[1].active=127;
    func_00218A80(); CHECK(count==2 && seen[0]==1 && kinds[0]==1 && seen[1]==3);
    CHECK(particles[0].payload[0]==0 && particles[1].payload[0]==1 && particles[2].payload[0]==0);
    reset(); particles[3].active=-1; func_00218A80(); CHECK(count==3 && pu_cursor==particles+3);
    reset(); memset(particles,255,sizeof(particles)); func_00218A80();
    CHECK(!count && pu_cursor==particles+8 && pu_end==particles+4);
    /* Callback changes saved cursor and end; a local for-loop would be wrong. */
    reset(); pu_high=6; mode=1; particles[0].type=80; func_00218A80();
    CHECK(count==2 && seen[0]==0 && seen[1]==3 && pu_cursor==particles+4);
    /* Pool and high are snapshots, unlike the saved pointers. */
    reset(); pu_high=1; mode=2; particles[0].type=80; func_00218A80();
    CHECK(count==2 && seen[1]==1 && pu_end==particles+2 && pu_pool==particles+6 && pu_high==7);
    reset(); pu_high=0; mode=3; particles[0].type=80; particles[3].active=-1; func_00218A80();
    CHECK(count==4 && seen[3]==3 && particles[3].payload[0]==1);
    reset(); mode=4; particles[0].type=80; particles[2].active=-1; func_00218A80();
    CHECK(count==3 && seen[1]==2 && seen[2]==3);
    reset(); mode=5; particles[0].type=80; func_00218A80(); CHECK(count==1 && pu_cursor==particles+8);
    puts("particle update: OK"); return 0;
}
