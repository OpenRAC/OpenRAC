/*
 * Tier: GPT-6.1 Sol, high effort; USA v1.01 game compiler, -O2 -G8
 * -ffunction-sections (a filename ending in -g8.c supplies the -G8 profile).
 * Best check.py: 1448/1476 bytes, 0/27 places match; nine checks this task.
 * Frame size 0x110 and vector/list/count/timer offsets match. Saved-register
 * allocation, initial loop guard, reaction-state branching and scheduling differ.
 * Typed resident-global fields recover the retained high-half base register.
 * Byte-offset macros must cast to char *: the earlier draft's F(out,8) read
 * offset 0x20 of an int array instead of its actual vector z at offset 8.
 * Interleaved integer/FP arguments preserve the callee's physical EE interface
 * and reproduce the query call's f16-before-t2 ordering.
 * Three reaction-condition wordings (mixed signedness, separate goto conditions,
 * and an explicit local byte value) emitted identical code in the 1440-byte
 * qcopy_nc variant; stopped that line of investigation on the tie.
 * TImode qcopy/barrier variants were 1432 bytes; moving index initialization
 * before the calls lost the correct frame size. No linked check for this body.
 * Next useful experiment: recover typed component/state access and real loop
 * induction structure to retain separate comparisons and the two constant
 * registers (retail saves fp and holds reaction states 1 and 2 across calls).
 * All required declarations follow.
 */
#include "common.h"
extern void func_L00_00409420(char *);
extern void ref_L00_00408E58_00(int,int);
extern void func_002A75E0(char *,int,int,int,int);
extern void func_00282CC0(void *,void *,void *);
extern void func_002829D8(void *);
extern void func_00282A88(void *,int);
extern int func_00282948(void *);
extern int func_L00_002D4040(int);
extern void func_L00_002AF568(void);
extern int func_L00_002D6998(void *);
extern void func_002E5CF8(int,int,char *);
extern void func_00282F20(void *,void *,float);
extern void func_L00_00416F68(void *,int,char *);
extern void func_002E6EF8(void (*)(char *),char *);
extern void func_L00_00409480(char *);
extern void func_L00_0037E000(char *);
extern void func_002AF580(void *,void *,int *,void *,int,int,float,float,float,float,float,int,int);
extern char *func_002AB1F0(char *);
extern void func_002A6C58(char *,int,int);
extern void func_00310440(char *);
extern void func_L00_002D3F30(int,int);
extern struct {
    char pad0[0x1230]; int mask;
    char pad1234[0xc]; int level;
    char pad1244[0x104c]; char *object; int state;
    char pad2298[0x24]; unsigned char disabled;
} D_00189E20;
extern int D_L00_001A8F00 __attribute__((nosda));
extern int D_00138324 __attribute__((nosda));
extern int D_L00_001B0F90 __attribute__((sda));
extern unsigned char D_00139593 __attribute__((nosda));
extern char D_L00_00262570[];
extern float D_L00_0025C64C __attribute__((nosda));
extern char D_L00_0029FBE0[];
extern char D_L00_001B8FF0[];
#define I(p,n) (*(int *)((char *)(p)+(n)))
#define F(p,n) (*(float *)((char *)(p)+(n)))
#define B(p,n) (*(unsigned char *)((char *)(p)+(n)))
void func_L00_00408E58(char *o)
{
    char *q = *(char **)(o+104);
    struct {
        int base[4] __attribute__((aligned(16)));
        int v[4] __attribute__((aligned(16)));
        int out[4] __attribute__((aligned(16)));
        char *list[32];
        int count;
        int *timer;
    } frame;
#define base frame.base
#define v frame.v
#define out frame.out
#define list frame.list
#define count frame.count
#define timer frame.timer
    int i;
    func_L00_00409420(o);
    if (D_00189E20.state==1) ref_L00_00408E58_00(29,1);
    if(B(o,0x31)) D_L00_0025C64C=0.1f;
    if(B(o,0x42)==B(o,0x43) && (B(o,0x60)&2)) func_002A75E0(o,1,0,2,0);
    func_00282CC0(base,base,o+16);
    func_002829D8(q);
    if(B(o,32)==1) {
        if(D_L00_001A8F00 || D_00189E20.disabled || !(D_00138324&D_00189E20.mask) || D_00189E20.level<23) return;
        if(func_L00_002D4040(-1)) {
            func_L00_002AF568();
            I(q,24)=func_L00_002D6998(D_L00_0029FBE0);
            B(o,32)=2;
            I(q,4)=D_L00_001B0F90;
        } else func_002E5CF8(4,0,o);
        F(q,0)=1.0f/F(D_L00_00262570+D_00139593*224,0x54);
    } else if(B(o,32)==2) {
        if(func_00282948(q+16)) { B(o,32)=4; I(q,16)=30; I(q,20)=0; }
        if(I(q,16)==10) {
            func_002A75E0(o,2,0,3,0);
            func_002E5CF8(1,0,o);
        }
    } else if(B(o,32)==4) {
        if(func_00282948(q+16)) {
            B(o,32)=5; I(q,20)=0; I(q,16)=30;
            func_002A75E0(o,3,0,3,0);
        } else if(!I(q,20)) {
            qcopy(v,D_00189E20.object+0xc0);
            func_00282F20(v,v,1.0f);
            func_00282CC0(out,base,v);
            func_L00_00416F68(out,1,o);
            I(q,20)=1;
        }
    } else if(B(o,32)==5) {
        if(func_00282948(q+8)) {
            B(o,32)=3; I(q,8)=64;
            func_002A75E0(o,4,0,3,0);
        } else func_002E6EF8(func_L00_00409480,o);
    } else if(B(o,32)==3) {
        qcopy(v,D_L00_001B8FF0);
        timer=(int *)(q+4);
        func_00282A88(list,128);
        func_L00_0037E000(o);
        func_002AF580(o+16,v,&count,list,32,0,3.1415929794311523f,1.047197699546814f,40.0f,0.0f,0.0f,0,0);
        for(i=0;i<count;i++) {
            char *p;
            int active=1;
            qcopy(out,list[i]+16);
            p=func_002AB1F0(list[i]);
            if(p) {
                F(out,8)+=F(p,16)*1.5f;
                if(F(p,0)>100.0f) active=0;
            }
            if(list[i] && B(list[i],32)!=254 && B(list[i],32)!=253) {
                if(*(short *)(list[i]+0xaa)==0xa8d) B(list[i],0x95)=9;
                if(*(short *)(list[i]+0xaa)==0xa8e) B(list[i],0x95)=7;
                if(*(short *)(list[i]+0xaa)==0x1002) B(list[i],32)=7;
            }
            if(active) {
                if(p && ((unsigned int)(B(p,14)-1)<2)) {
                    func_L00_00416F68(out,0,0);
                    I(q,28)++;
                    func_002A6C58(list[i],0,3);
                    B(p,14)=2;
                } else if(p && B(p,14)!=3) {
                    func_L00_00416F68(out,0,0);
                    I(q,28)++;
                    func_002A6C58(list[i],0,3);
                    func_00310440(list[i]);
                }
            } else {
                F(p,0)-=20.0f;
                if(F(p,0)<0.0f) F(p,0)=0.0f;
            }
        }
        if(func_00282948(timer)) {
            if(I(q,28)) func_002E5CF8(2,0,o);
            else func_002E5CF8(3,0,o);
            I(q,28)=0;
            B(o,32)=0;
            func_L00_002D3F30(-1,1);
        }
    }
}
