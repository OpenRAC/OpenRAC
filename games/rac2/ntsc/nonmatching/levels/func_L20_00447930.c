/* GPT-6.1 Sol high worker v0899, USA v1.01, -O2 -G8 -ffunction-sections.
 * Best check.py: 1052/1052 bytes, 0 of 1 places match; first mismatch +0xF8.
 * Six instructions at 0x447A18-0x447A34 use different temporary registers
 * for the D_L20_0021EFB0 flag lookup; all other non-relocation words match.
 * This reproduces the sol-low draft from the read-only integration checkout.
 * An explicit base local and the compiler qcopy/barrier pattern for the two
 * state copies tie the baseline. Integer address arithmetic folds 0x464 into
 * the relocation and remains wrong; preloading the index worsens scheduling.
 * Replacing all copies with the compiler pattern worsens size/allocation;
 * replacing the entry copies lets register promotion change the stack layout.
 * -mno-split-addresses hits an assembler assertion (tc-mips.c:11922);
 * MACRO_ADDR on the flag array changes earlier allocation and the displacement.
 * Next useful input: an independently evidenced addressing/declaration profile.
 * qcopy_nc for entry-table position/rotation shares the index and base; these
 * source reads do not depend on the object's position/rotation destinations.
 * All required declarations follow. No linked build run.
 */
#include "common.h"
extern int D_001A79F0;
extern unsigned char D_0019B2E8[];
extern char D_00189EA0[];
extern unsigned char D_L20_0021EFB0[];
extern unsigned char *D_L20_001B2F9C;
typedef struct { char pad[0x30]; char position[0x10]; char pad2[0x30]; char rotation[0x10]; } Entry1386;
extern Entry1386 *D_L20_001B308C;
extern int func_0033E808(char *,int);
extern int func_002B38B8(char *);
extern void func_002A7750(char *,int,int);
extern void func_002A0430(int);
extern void func_0029F338(char *);
extern void func_L20_002C5A48(int);
extern float func_00283180(float);
extern void func_00282F20(void *,void *,float);
extern void func_00282CF0(void *,void *,void *);
extern void func_00282D30(void *,void *,float);
extern void func_00282CC0(void *,void *,void *);
extern void func_002AD6F0(void *,void *,void *,void *);
typedef int Q1386 __attribute__((mode(TI)));
void func_L20_00447930(unsigned char *obj) {
    Q1386 pos,rot,delta;
    unsigned char *state=*(unsigned char **)(obj+0x68);
    char *p=obj+0x10;
    float t;
    qcopy(&pos,p); qcopy(&rot,obj+0xF0);
    switch ((unsigned char)obj[0x20]) {
    case 0:
        if (*(int *)(state+0x80)==-1) {
            *(unsigned short *)(obj+0x34)|=2;
            func_002A7750(obj,4,-1); return;
        }
        *(int *)(state+0x88)=0;
        *(int *)(state+0x5C)|=1;
        qcopy(state+0x60,p); qcopy(state+0x70,obj+0xF0);
        *(unsigned short *)(obj+0x34)|=0x100;
        if (D_L20_0021EFB0[*(int *)(state+0x8C)+0x464] ||
            ((unsigned char)obj[0xB0]!=0xFF && (unsigned char)state[0x92] && (D_0019B2E8[(unsigned char)obj[0xB0]+D_001A79F0*16]&0x80)) ||
            (*(int *)(state+0x98)!=-1 && func_0033E808(D_00189EA0,*(int *)(state+0x98)))) {
            qcopy_nc(obj+0x10,D_L20_001B308C[*(int *)(state+0x80)].position);
            qcopy_nc(obj+0xF0,D_L20_001B308C[*(int *)(state+0x80)].rotation);
            func_0029F338(obj); return;
        } else func_002A7750(obj,1,-1);
        break;
    case 1:
        if ((unsigned char)obj[0xBC] || func_002B38B8(obj) || *(int *)(state+0x8C)==-1 ||
            (unsigned char)D_L20_001B2F9C[(*(int *)(state+0x8C)<<8)+0x20]==0xFE ||
            (unsigned char)D_L20_001B2F9C[(*(int *)(state+0x8C)<<8)+0x20]==0xFD ||
            (((unsigned char)obj[0xB0]!=0xFF && (unsigned char)state[0x92] && (D_0019B2E8[(unsigned char)obj[0xB0]+D_001A79F0*16]&0x80)) ||
             (*(int *)(state+0x98)!=-1 && func_0033E808(D_00189EA0,*(int *)(state+0x98))))) {
            if (*(int *)(state+0x94)!=-1) D_L20_001B2F9C[(*(int *)(state+0x94)<<8)+0xBC]=1;
            if ((unsigned char)state[0x93]) func_002A0430((unsigned char)obj[0xB0]);
            func_002A7750(obj,2,-1);
        }
        break;
    case 2:
        *(float *)(state+0x88)+=1.0f / *(float *)(state+0x84);
        if (*(float *)(state+0x88)>1.0f) *(float *)(state+0x88)=1.0f;
        t=(1.0f-func_00283180(*(float *)(state+0x88)*3.14159274f))*0.5f;
        func_00282F20(obj+0xE0,obj+0xE0,1.0f-t);
        func_00282CF0(&delta,D_L20_001B308C[*(int *)(state+0x80)].position,state+0x60);
        func_00282D30(&delta,&delta,t);
        func_00282CC0(p,state+0x60,&delta);
        if (*(float *)(state+0x88)>=1.0f) func_002A7750(obj,3,-1);
        break;
    case 3:
        if (*(int *)(state+0x94)==-1 || !(unsigned char)D_L20_001B2F9C[(*(int *)(state+0x94)<<8)+0xBC]) {
            if ((unsigned char)state[0x90]) func_L20_002C5A48(0);
            func_0029F338(obj); return;
        }
        break;
    }
    if ((unsigned char)obj[0x20]<3) {
        func_00282CF0(&delta,obj+0x10,&pos);
        func_002AD6F0(state+0x20,&delta,&rot,obj+0xF0);
    }
}
