/* NON_MATCHING func_L16_002CFDB8 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 688 / retail 696, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   - Other substates refresh generation cache and resolve damage; skip status1/states10/11.
 *   - Fatal damage enters10; surviving damage stores prior state/animation, faces player, enters9 and animates hit
 *   - Update damage/visibility; attached child follows selected joint transform and three unit-scaled basis vector
 *   - Always repeat final damage update unless early respawn/delete path.
 *   1. SIZE688: early player alias suppresses branch-likely/load ordering, and damage current aliases coalesce. Mo
 *   2. SIZE688: delayed player view and cached coordinates reproduce all player branches/math. Remaining missing a
 *   3. SIZE688: named inner array does not break coalescing with the initial pointer. Move persistent damage alias
 *   4. SIZE688: third pointer lifetime wording preserves the same damage-pointer coalescing and saved-register per
 */
#include "common.h"
extern int func_001F9850(int);
extern void func_L00_002584A8(void*,int,int);
extern void func_0020D678(void*);
extern char *func_L00_0025B478(void*,int,int);
extern int func_L00_0025B4D0(void*,void*,void*,int,int*,float*,int,int);
extern float func_L00_001FF860(float,float);
extern void func_00213DE0(void*,int,int,int);
extern void func_L00_0025E4B0(void*,short*);
extern void func_L00_0025E590(void*,void*);
extern void func_L00_00250800(void*,int,void*);
extern void func_0020DAF8(void*,int,void*);
extern void func_001FA480(void*,void*);
extern void func_L00_001FF4B0(void*,void*,float);
extern void func_L00_00251E30(void*);
extern int D_L16_0015F6B0 MACRO_ADDR;
extern short D_L16_00161A88;
extern char D_0013E633[];
typedef struct {int status;float amount;} L16RespawnHit;
typedef struct {char pad[0x80];float position[4];} L16RespawnPlayer;
#define B(p,o) (*(unsigned char*)((char*)(p)+(o)))
#define W(p,o) (*(int*)((char*)(p)+(o)))
#define F(p,o) (*(float*)((char*)(p)+(o)))
/* Handle respawning, hit reactions and an attached joint object. */
void func_L16_002CFDB8(char *m) {
    float matrix[16],zero;
    L16RespawnHit hit;
    char *d;
    void *damage;
    char *record;
    if(B(m,0x20)==1) return;
    d=*(char**)(m+0x78);
    if(B(d,0x2E)==2) {
        B(d,0x2E)=1;F(d,0x20)=2.0f;W(d,0xF0)=func_001F9850(60);
        func_L00_002584A8(m,0,-1);
        *(unsigned short*)(m+0x34)&=0xEFFF;
        W(d,0xC8)--;
        if(W(d,0xC8)!=-1) {
            B(m,0x20)=11;qcopy(m+0x10,d+0xE0);W(d,0xF0)=func_001F9850(60);
        } else {
            if(*(void**)(d+0x104)) func_0020D678(*(void**)(d+0x104));
            func_0020D678(m);
        }
        return;
    }
    if(W(d,0x118)!=D_L16_0015F6B0) func_L16_002D0238(m);
    hit.amount=zero=0.0f;
    record=func_L00_0025B478(m,0x330000,0);
    func_L00_0025B4D0(m,record,d+0x20,0,&hit.status,&hit.amount,0,4);
    damage=d+0x60;
    if(hit.status!=1 && B(m,0x20)!=10 && B(m,0x20)!=11) {
        F(d,0x20)-=hit.amount;
        if(F(d,0x20)<=zero) B(m,0x20)=10;
        else {
            if(B(m,0x20)!=9) {W(d,0x110)=B(m,0x20);W(d,0x114)=B(m,0x53);}
            {float x=F(m,0x10),y=F(m,0x14);
            L16RespawnPlayer *player=(L16RespawnPlayer*)(D_0013E633+0xE1D);
            F(d,0x10C)=func_L00_001FF860(player->position[0]-x,player->position[1]-y);}
            B(m,0x20)=9;
            if(B(m,0x53)!=8) func_00213DE0(m,8,2,1);
            {short *current=(short*)(d+0x60);
            B(d,0x67)=120;func_L00_0025E4B0(m,current);damage=d+0x60;}
        }
    }
    B(m,0xA4)=255;func_L00_0025E590(m,damage);
    if(*(char**)(d+0x104)) {
        func_L00_00250800(m,*(int*)&D_L16_00161A88,*(char**)(d+0x104)+0x10);
        func_0020DAF8(m,*(int*)&D_L16_00161A88,matrix);
        func_001FA480(*(char**)(d+0x104)+0xC0,matrix);
        func_L00_001FF4B0(*(char**)(d+0x104)+0xC0,*(char**)(d+0x104)+0xC0,1.0f);
        func_L00_001FF4B0(*(char**)(d+0x104)+0xD0,*(char**)(d+0x104)+0xD0,1.0f);
        func_L00_001FF4B0(*(char**)(d+0x104)+0xE0,*(char**)(d+0x104)+0xE0,1.0f);
        func_L00_00251E30(*(char**)(d+0x104));
    }
    func_L00_0025E590(m,damage);
}
