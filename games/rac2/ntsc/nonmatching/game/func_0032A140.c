/* sol-high, task v1333: USA v1.01, pinned game -O2 -G8.
 * Recompiled sol-low b1199 draft: 1596/2104 bytes, 0/1 places match.
 * CATALOGUE blocker: current extent starts at 0032A140 with 512 bytes
 * containing 63 positive addiu sp,sp and 65 nops, after jr ra at 0032A138
 * and its nop delay slot. The actual -0xB0 prologue starts at 0032A340.
 * A boot scan found no direct branch/jump, aligned literal pointer, or
 * nearby lui/address-construction pair into the prefix or at 0032A340.
 * The read-only orch/main tools/levels.py already records (2104, 512)
 * for this function, but this checkout does not have that entry repair.
 * Stop here until the catalogue correction lands; do not pad C to 2104.
 * The prior shortened-target result was 1596/1592 bytes, 0/1 places;
 * GPR/FPR allocation and scheduling differ. Recompiled here only against
 * the current uncorrected extent, not a modified or masked target.
 * Needs an isolated -g8.c unit for the GP store and jump-table ownership.
 * Full required declarations follow; original src marker is restored.
 */
#include "common.h"
typedef struct { char pad[0x2290]; char *object; } Context;
extern Context D_L00_00189E20;
extern char D_L00_00189F00[], D_L00_0018A0C0[];
extern char D_00235D00[], D_001BAFA0[], D_001B2B40[], D_001B5340[];
extern char *D_001BF000[];
extern int D_001B0590;
extern float func_002A7940(void), func_00283CE0(int), func_002A7790(float);
extern float func_00283180(float), func_00283B98(float,float), func_00283BE0(float,float);
extern int func_00283CF0(float), func_00282948(void *);
extern void func_002AF000(void *,float,float,float), func_00282CC0(void *,void *,void *);
extern void func_00282CF0(void *,void *,void *), func_00282F20(void *,void *,float);
extern int func_002761F8(void *,void *,int,int,int), func_00277F48(void *,int,void *,int,float);
extern void func_0032B598(void *,void *,int), func_0032ABC0(void *), func_0032A978(void *);
extern void func_002A8C00(void *,void *,int,void *,float), func_002A8E20(void *,void *);
extern void func_0032AA30(void), func_0029F338(void *), func_00291450(int);
extern void func_002833F0(void *,void *), func_0032ACF0(void *), func_0032AF50(void *);
extern void func_0032B7D0(void *), func_0032B190(void), func_0027CFD0(void (*)(void),void *);
void func_0032A140(char *self)
{
    float v[4];
    char hit[48];
    char *state = *(char **)(self+0x68);
    char *p;
    char **list;
    int i,n;
    float t;
    qcopy(self+0x10, D_L00_00189E20.object+0x10);
    switch ((unsigned char)self[0x20]) {
    case 0:
        self[0x20]=1;
        *(int *)(state+4)=60;
        *(int *)(state+0x14)=0;
        { char *row=D_00235D00; int j;
        for(j=136;j>=0;j--) {
            *(float *)row=func_002A7940();
            *(float *)(row+4)=func_002A7940();
            row+=8;
        }
        }
    case 1:
        self[0x20]=2;
    case 2:
        t=func_00283CE0(*(int *)(state+4));
        t/=func_00283CE0(60);
        *(float *)(state+0x14)=func_002A7790(1.0f-t)*15.0f;
        if (*(int *)(state+4)%3==0) {
            float a=func_002A7940(); float b=func_002A7940();
            func_002AF000(v,*(float *)(state+0x14),a,b);
            func_00282CC0(v,v,D_L00_00189F00);
            if(func_002761F8(D_L00_00189F00,v,0,*(int *)(D_L00_00189F00+0x21B0),0))
                qcopy(v,D_001BAFA0);
            func_0032B598(self,v,10);
        }
        if (*(int *)(state+4)<10 && !(*(int *)(state+4)&7)) func_0032ABC0(self);
        if(func_00282948(state+4)) {
            *(int *)(state+4)=120; self[0x20]=3;
            n=func_00277F48(D_L00_00189F00,0,self,0,15.0f);
            if(n>0) {
                list=D_001BF000;
                do {
                    p=*list;
                    if ((*(unsigned long long *)(p+0x30)&0x500000000000ULL) &&
                        p!=D_L00_00189E20.object &&
                        *(short *)(p+0xAA)!=3303 && *(short *)(p+0xAA)!=3000) {
                        func_00282CF0(v,p+0x10,D_L00_00189F00);
                        func_00282F20(v,v,1.0f);
                        func_00282F20(hit,hit,1.0f);
                        func_002A8C00(hit,self,0x830000,v,30.0f);
                        qcopy(hit,v);
                        *(float *)(hit+12)=5627.9248046875f;
                        hit[0x19]=3; hit[0x18]=5;
                        *(int *)(hit+0x20)=1;
                        *(short *)(hit+0x1A)=*(unsigned short *)(*list+0xAA);
                        func_002A8E20(*list,hit);
                    }
                    list++;
                } while(--n);
            }
            func_0032A978(self);
        }
        break;
    case 3:
        if(!(*(int *)(state+4)&7)) func_0032ABC0(self);
        func_00282948(state+4); func_0032AA30();
        if(*(int *)(state+4)==40) self[0x20]=4;
        break;
    case 4:
        if(func_00282948(state+4) && !*(int *)state) {
            if(*(char **)(state+8)) func_0029F338(*(char **)(state+8));
            if(*(char **)(state+12)) func_0029F338(*(char **)(state+12));
            if(*(int *)(state+16)!=-1) func_00291450(*(int *)(state+16));
            D_001B0590=0;
            func_0029F338(self); return;
        }
        break;
    }
    t=func_00283CE0(*(int *)(state+4)); t/=func_00283CE0(120);
    t=(1.0f-t)*6.2831855f;
    t=(func_00283180(t-3.1415927f)+1.0f)*0.5f;
    if(*(char **)(state+8)) {
        i=func_00283CF0(t*32.0f);
        **(int **)(*(char **)(state+8)+0x68)=i;
        
        *(float *)((*(char **)(state+8))+0xF8)=func_00283B98(*(float *)((*(char **)(state+8))+0xF8),0.05235988646745682f);
         func_002833F0((*(char **)(state+8))+0xC0,(*(char **)(state+8))+0xF0);
         *(float *)((*(char **)(state+8))+0xC0)*=0.5f;
         *(float *)((*(char **)(state+8))+0xD4)*=0.5f;
         *(float *)((*(char **)(state+8))+0xE8)=t*1.5f;
        qcopy((*(char **)(state+8))+0x10,D_L00_00189E20.object+0x10);
    }
    if(*(char **)(state+12)) {
        i=func_00283CF0(t*32.0f);
        **(int **)(*(char **)(state+12)+0x68)=i;
        
        *(float *)((*(char **)(state+12))+0xF8)=func_00283BE0(*(float *)((*(char **)(state+12))+0xF8),0.05235988646745682f);
         func_002833F0((*(char **)(state+12))+0xC0,(*(char **)(state+12))+0xF0);
         *(float *)((*(char **)(state+12))+0xE8)=t;
        qcopy((*(char **)(state+12))+0x10,D_L00_00189E20.object+0x10);
    }
    if(*(int *)(state+16)!=-1) {
        p=D_001B2B40+*(int *)(state+16)*32;
        func_00282F20(v,D_L00_0018A0C0,0.5f);
        func_00282CC0(p+16,*(char **)(D_L00_0018A0C0+0x1FF0)+16,v);
        func_00282CF0(v,D_001B5340,p+16);
        func_00282F20(v,v,0.15f);
        func_00282CC0(p+16,p+16,v);
        *(float *)(p+8)=t; *(float *)(p+28)=5.0f; *(int *)(p+12)=0;
    }
    func_0032ACF0(self); func_0032AF50(self); func_0032B7D0(self);
    func_0027CFD0(func_0032B190,self);
}
