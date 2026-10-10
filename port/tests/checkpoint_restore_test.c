/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NOT_SDA
#define MACRO_ADDR
#include "../game/rac1-pal/hand/level_checkpoint_restore.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while(0)
_Alignas(16) char D_L00_001BA960[0xC70], D_0013F450[0x2400];
_Alignas(16) char D_0013E650[0x80], D_001517D0[0x50];
_Alignas(16) unsigned char D_L00_001BB5C0[0xC70];
char *D_L00_00160098, *D_L00_0016009C;
static _Alignas(16) char moby[0x100], replacement[0x100], objects[0x300];
static char expected_hero[0x2400], expected_status[0x80], expected_global[0x50];
static char expected_moby[0x100], expected_current[0xC70];
static unsigned char saved[0xC70];
static char calls[16];
static int ncalls, expected_mode, expected_value, replace_moby;
static char *expected_object;
static void record(char c) { CHECK(ncalls < 15); calls[ncalls++]=c; }
void func_001F99B0(void *out,int value,int size) {
    record('Z'); CHECK(out==D_L00_001BA960 && value==0 && size==0xC60);
    memset(out,value,(size_t)size);
}
void checkpoint_copy(void *out,void *in,unsigned short size) {
    record('C'); CHECK(out==D_L00_001BA960 && in==D_L00_001BB5C0 && size==0xC60);
    memcpy(out,in,size);
}
void func_001F9978(void) { record('A'); }
void func_L00_002110C0(int mode,int value,void *object) {
    record('M'); CHECK(mode==expected_mode && value==expected_value);
    CHECK(object==expected_object);
}
void func_L00_00251E30(void *object) {
    record('H'); CHECK(object==moby);
    CHECK(memcmp(moby,expected_moby,sizeof moby)==0);
    CHECK(memcmp(D_0013F450,expected_hero,sizeof expected_hero)==0);
    CHECK(memcmp(D_0013E650,expected_status,sizeof expected_status)==0);
    CHECK(memcmp(D_001517D0,expected_global,sizeof expected_global)==0);
    if(replace_moby) {
        *(char **)(D_0013F450+0x2080)=replacement;
        *(char **)(expected_hero+0x2080)=replacement;
    }
}
void func_001FA1F8(void *out,void *in) {
    record('R'); CHECK(out==(replace_moby?replacement:moby)+0xC0 && in==D_0013F450+0x90);
}
void func_L00_001ED600(void) { record('E'); }
void func_00216270(void) { record('S'); }
static void word(void *base,int offset,int value) { memcpy((char *)base+offset,&value,4); }
static void object_class(int slot,short value) { memcpy(objects+slot*0x100+0xA6,&value,2); }
static void setup(int mode,int valid) {
    ncalls=replace_moby=0; expected_object=objects; memset(calls,0,sizeof calls);
    memset(D_L00_001BA960,0xA5,sizeof D_L00_001BA960);
    memset(D_0013F450,0xB6,sizeof D_0013F450);
    memset(D_0013E650,0xC8,sizeof D_0013E650);
    memset(D_001517D0,0xD9,sizeof D_001517D0);
    memset(moby,0xEA,sizeof moby); memset(replacement,0xFB,sizeof replacement);
    memset(objects,0xFF,sizeof objects);
    D_L00_00160098=objects; D_L00_0016009C=objects+sizeof objects;
    *(char **)(D_0013F450+0x2080)=moby;
    for(unsigned i=0;i<sizeof saved;++i) D_L00_001BB5C0[i]=(unsigned char)(i*37+11);
    word(D_L00_001BB5C0,0,valid);
    word(D_L00_001BB5C0,0x44,mode);
    expected_mode=mode; expected_value=0x12345678;
    word(D_L00_001BB5C0,0x4C,expected_value);
    /* Vector restoration must copy bits, including NaN and signed zero. */
    word(D_L00_001BB5C0,0x10,(int)0x80000000u);
    word(D_L00_001BB5C0,0x24,0x7FC12345);
    memcpy(saved,D_L00_001BB5C0,sizeof saved);
    memcpy(expected_hero,D_0013F450,sizeof expected_hero);
    memcpy(expected_status,D_0013E650,sizeof expected_status);
    memcpy(expected_global,D_001517D0,sizeof expected_global);
    memcpy(expected_moby,moby,sizeof expected_moby);
    memcpy(expected_current,D_L00_001BA960,sizeof expected_current);
    if(!valid) { memset(expected_current,0,0xC60); return; }
    memcpy(expected_current,saved,0xC60);
    memcpy(expected_hero+0x80,saved+0x10,0x20);
    memcpy(expected_moby+0x38,saved+0x30,8);
    memcpy(expected_moby+0x80,saved+0x38,4);
    expected_status[0x6B]=(char)0xCF;
    memcpy(expected_status+0x64,saved+0x3C,7);
    memcpy(expected_global+0x38,saved+0xC54,2);
}
static void run(const char *sequence) {
    char saved_objects[sizeof objects]; memcpy(saved_objects,objects,sizeof objects);
    func_L00_002862E0();
    CHECK(strcmp(calls,sequence)==0);
    CHECK(memcmp(D_L00_001BA960,expected_current,sizeof expected_current)==0);
    CHECK(memcmp(D_L00_001BB5C0,saved,sizeof saved)==0);
    CHECK(memcmp(D_0013F450,expected_hero,sizeof expected_hero)==0);
    CHECK(memcmp(D_0013E650,expected_status,sizeof expected_status)==0);
    CHECK(memcmp(D_001517D0,expected_global,sizeof expected_global)==0);
    CHECK(memcmp(moby,expected_moby,sizeof moby)==0);
    CHECK(memcmp(objects,saved_objects,sizeof objects)==0);
}
int main(void) {
    setup(1,0); run("Z");
    /* A valid word with a zero low byte must still take the restore path. */
    setup(0,0x100); run("CHRES");
    setup(3,-1); run("CHRES");
    setup(1,1); object_class(0,0x57); object_class(1,0x57); run("CMHRES");
    setup(2,1); object_class(0,0x57); object_class(2,0x1A3);
    expected_object=objects+0x200; run("CMHRES");
    setup(1,1); object_class(2,0x1A3); run("CHRES");
    setup(2,1); object_class(0,(short)0xFFA3); run("CHRES");
    setup(1,1); D_L00_0016009C=objects; object_class(0,0x57); run("CHRES");
    setup(2,1); D_L00_00160098=objects+0x200; D_L00_0016009C=objects; run("CHRES");
    setup(0x101,1); run("CAHRES");
    setup(-1,1); object_class(1,0); expected_object=objects+0x100; run("CAMHRES");
    setup(3,1); replace_moby=1; run("CHRES");
    puts("checkpoint restore: word flags, full copy, state fields, search modes and callback order passed");
    return 0;
}
