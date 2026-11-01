/*
 * Tier: sol-high, task v1155, 29 October 2026.
 * USA v1.01, current pinned game compiler: 1524/1524 bytes, 0/27 places.
 * Profile: -O2 -G8 -ffunction-sections; declarations included below.
 * The 0x160 frame and four saved FPRs now agree. Direct TImode packet copies
 * remove the old saved sentinel; collision targets must reload after calls.
 * Computing nextIndex in each completion path keeps the initial loop and its
 * branch topology aligned. Remaining differences include object/constant
 * register allocation, temp-pointer lifetime, table madd versus mult/addu,
 * scheduling, and the final signed wrap comparison. First mismatch is +0x8.
 * Twenty bounded trials: qcopy_nc does not improve this baseline; -g ties,
 * -fno-gcse and -mno-split-addresses regress. MACRO_ADDR on the class table
 * preserves mult/addu but hoists its base and changes allocation; NOT_SDA
 * retains the better outer-loop roles. Next: recover the class-table address
 * expression and the temp pointer's phase lifetime together. No linked match.
 */
#include "common.h"
typedef unsigned int Q __attribute__((mode(TI)));
typedef union { Q q; float f[4]; } V;
typedef struct { V pos,vel,accel; int next,active,handle,pad; } Entry;
extern Entry *D_L00_001B144C __attribute__((sda));
typedef struct { char pad[0x18]; char *target; int count; V point; char gap[0x10]; V normal; } Collision;
extern Collision D_L00_001BF200;
extern char D_L00_001C3280[];
extern unsigned char D_00139568[];
typedef union { char b[0x2600]; char *p[0x980]; } HeroState;
extern HeroState D_00189E20;
typedef struct { char pad[0x70]; float direct, splash; char rest[0x68]; } ClassEntry;
extern ClassEntry D_L00_00262570[] NOT_SDA;
extern void func_00300190(void*,void*),func_002FEE70(int,void*,void*),func_00282CC0(void*,void*,void*),func_00282C88(void*),func_00282F20(void*,void*,float),func_002FEE50(int),func_00283410(void*,void*),func_002830C0(void*,void*,void*),func_002BE278(void*,void*,int,float),func_00282CF0(void*,void*,void*),func_002A8C00(void*,void*,int,void*,float),func_002A8E20(void*,void*),func_0032D758(void*,int,void*),func_002E5CF8(int,int,void*),func_002FE880(void*,void*,float),func_L00_0041B840(void*,void*,void*,void*),func_L00_0041AD40(void*,void*,void*);
extern int func_002761F8(void*,void*,int,void*,int),func_00277F48(void*,int,void*,int,float),func_001163B0(void);
extern float func_002A7878(float,float);
void func_L00_0041A5C0(char *object)
{
    V next,temp,small,angles,rot[4],direction;
    struct { V direction; char pad[8]; unsigned char type,flag; unsigned short id; int pad2,enabled; char tail[12]; } hit;
    char *state=*(char **)(object+0x68);
    int i,index=*(int *)(state+0x48);
    Entry *e;
    char *target;
    int n,j,nextIndex;
    for(i=0;i<*(int *)(state+0x44);) {
        e=D_L00_001B144C+index;
        if(i) func_00300190(e,e);
        if(e->handle>=0) func_002FEE70(e->handle,&e->vel,e);
        func_00282CC0(&next,e,&e->vel);
        if(!e->active) {
            if(func_002761F8(e,&next,0x10,object,0)) {
                qcopy(e,&D_L00_001BF200.point);
                func_00282C88(&e->vel);
                func_00282F20(&temp,&D_L00_001BF200.normal,0.1f);
                func_00282CC0(e,e,&temp);
                e->active=1;
                func_002FEE50(e->handle);e->handle=-1;
                qcopy(&e->accel,&temp);
                func_00282F20(&small,&temp,0.01f);
                angles.f[0]=func_002A7878(-0.7853981853f,0.7853981853f);
                angles.f[1]=func_002A7878(-0.7853981853f,0.7853981853f);
                angles.f[2]=func_002A7878(-0.7853981853f,0.7853981853f);
                func_00283410(rot,&angles);
                rot[3].f[0]=0;rot[3].f[1]=0;rot[3].f[2]=0;
                func_002830C0(&small,&small,rot);
                func_002BE278(e,&small,0,-1);
                func_00282C88(&angles);
                target=D_L00_001BF200.target;
                if(target && target!=D_00189E20.p[0x2290/4] && ((*(unsigned short *)(target+0x34)&0x1000) || *(short *)(*(char **)(target+0x24)+0x46)==5)) {
                    nextIndex=index-1;
                    func_00282CF0(&direction,target+0x10,object+0x10);
                    func_00282F20(&direction,&direction,1);func_00282F20(&hit,&hit,1);
                    func_002A8C00(&hit,object,0x910000,&direction,((ClassEntry *)((unsigned int)D_L00_00262570+D_00139568[0x1d]*0xe0))->direct);
                    hit.direction.q=direction.q;hit.direction.f[3]=5627.9248046875f;
                    hit.type=9;hit.flag=1;hit.enabled=1;hit.id=*(unsigned short *)(D_L00_001BF200.target+0xaa);
                    func_002A8E20(D_L00_001BF200.target,&hit);func_0032D758(D_L00_001BF200.target,0x1d,object);func_002E5CF8(1,0,object);
                    n=func_00277F48(D_L00_001BF200.target+0x10,0x10,object,0,1.5f);
                    for(j=0;j<n;j++) {
                        target=((char **)D_L00_001C3280)[j];
                        if(target==D_00189E20.p[0x2290/4]) continue;
                        func_00282CF0(&direction,target+0x10,object+0x10);
                        func_00282F20(&direction,&direction,1);func_00282F20(&hit,&hit,1);
                        func_002A8C00(&hit,object,0x910000,&direction,((ClassEntry *)((unsigned int)D_L00_00262570+D_00139568[0x1d]*0xe0))->splash);
                        hit.direction.q=direction.q;hit.direction.f[3]=5627.9248046875f;
                        hit.type=9;hit.enabled=1;hit.flag=1;hit.id=*(unsigned short *)(((char **)D_L00_001C3280)[j]+0xaa);
                        func_002A8E20(((char **)D_L00_001C3280)[j],&hit);func_0032D758(((char **)D_L00_001C3280)[j],0x1d,object);func_002E5CF8(1,0,object);
                    }
                } else {
                    nextIndex=index-1;
                    if(func_001163B0()%11==0) func_L00_0041B840(object,&D_L00_001BF200.point,&temp,&angles);
                }
            } else {
                func_002FE880(e,&e->vel,0.006f);
                nextIndex=index-1;
            }
        } else {
            nextIndex=index-1;
        }
        ++i;
        func_00282CC0(e,e,&e->vel);
        index=0x22;
        if(nextIndex>=0)index=nextIndex;
        if(func_001163B0()%7==0) {func_00282F20(&temp,&e->vel,0.05f);func_L00_0041AD40(object,e,&e->vel);}
        if(func_001163B0()%22==0) {func_00282F20(&temp,&e->vel,0.05f);func_002BE278(e,&temp,0,-1);}
    }
}
