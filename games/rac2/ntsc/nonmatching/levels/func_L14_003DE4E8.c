/* GPT-6.1 Sol (high effort), v0905, USA v1.01, pinned game -O2 -G0.
 * Best check: 1020/1024 bytes, 0 of 1 places match. No linked match.
 * A short-return inline (RAC1 pauseFlagState, src/game/pause.c) avoids
 * movn and restores the state-load scratch register, but emits a shorter
 * bnez/assignment/assignment sequence instead of retail's branch pair at
 * +0x180. The self argument move stays after selection rather than in the
 * branch delay slot. Other code matches after accounting for that word.
 * Duplicated calls give 1036 bytes; a 64-bit state gives 1028 bytes plus
 * narrowing shifts; short/char local selections still give movn.
 * Next: preserve the branch pair and earlier self argument move together.
 * Required declarations are included below.
 */
#include "common.h"
typedef unsigned int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vec;
extern void func_002A7750(void *, int, int);
extern void func_00282CF0(void *, void *, void *);
extern void func_002AD6F0(void *, void *, void *, void *);
typedef struct { char pad[0x80]; float pos[4]; char gap[0x2224]; unsigned char mode; } B1245Player;
extern B1245Player D_00189E20;
extern char D_00188660[];
extern int func_L00_00324120(void *);
extern int func_002B3868(char *);
extern int func_002E59B0(void *, int);
extern void func_002E59F8(int);
extern int func_002E5CF8(int, int, void *);
extern int func_L14_003DE908(void *);
extern int func_002ACBC8(void *, int);
extern void func_002AA548(void *, void *, float, float, float, float);
/* A short result preserves the branch, as in RAC1 pauseFlagState. */
static inline short nextMovementState(int open) {
    if (open == 0) return 3;
    return 2;
}

void func_L14_003DE4E8(char *self) {
    Vec oldpos, oldvel, delta;
    char *s = *(char **)(self + 0x68);
    qcopy(&oldpos, self + 0x10);
    qcopy(&oldvel, self + 0xF0);
    switch ((unsigned char)self[0x20]) {
    case 0:
        *(int *)(s+0x5C) |= 1;
        { float z=*(float *)(self+0x18);
        float mid=(z+*(float *)(s+0x64))*0.5f;
        *(int *)(s+0x74) = -1;
        *(float *)(s+0x7C) = 0.00116666674f;
        *(float *)(s+0x68) = z;
        *(int *)(s+0x70) = 0;
        *(float *)(s+0x80)=mid; }
        if (*(int *)(s+0x60)==0) *(int *)(s+0x6C)=0;
        else *(int *)(s+0x6C)=1;
        self[0x20] = 1;
        break;
    case 1:
        if (D_00189E20.mode == 1 && *(int *)(s+0x84)) {
            if (*(int *)(s+0x6C) == 1) {
                func_002A7750(self,2,-1);
                *(int *)(s+0x74) = func_002E5CF8(0,4,self);
            }
        } else if (func_L00_00324120(self) ||
                   ((*(float *)(self+0x18)-*(float *)(s+0x80)) *
                    (D_00189E20.pos[2]-*(float *)(s+0x80)) < 0.0f && *(int *)(s+0x70))) {
            func_002A7750(self,nextMovementState(*(int *)(s+0x6C)),-1);
            if (!func_002E59B0(self,*(int *)(s+0x74)))
                *(int *)(s+0x74) = func_002E5CF8(0,4,self);
        }
        break;
    case 2:
        if (func_002B3868(self)) { *(int *)(s+0x70) += 1; *(int *)(s+0x78)=0; }
        if (*(float *)(s+0x64)-*(float *)(self+0x18) < 0.03f) {
            int n=*(int *)(s+0x74);
            if(n!=-1) {
                unsigned char *e=(unsigned char *)D_00188660+n*0x70;
                if(*(char **)(e+0x88)==self && e[0x74]) func_002E59F8(n);
            }
            *(int *)(s+0x74)=-1;
            if(!func_L14_003DE908(self)) {
                *(int *)(s+0x6C)=0;
                *(float *)(self+0x18)=*(float *)(s+0x64);
                func_002A7750(self,1,-1);
            }
        } else func_002AA548(self+0x18,s+0x78,*(float *)(s+0x64),*(float *)(s+0x7C),*(float *)(s+0x7C),0.100000009f);
        break;
    case 3:
        if (func_002B3868(self)) { *(int *)(s+0x70) += 1; *(int *)(s+0x78)=0; }
        if (*(float *)(self+0x18)-*(float *)(s+0x68) < 0.03f) {
            int n=*(int *)(s+0x74);
            if(n!=-1) {
                unsigned char *e=(unsigned char *)D_00188660+n*0x70;
                if(*(char **)(e+0x88)==self && e[0x74]) func_002E59F8(n);
            }
            *(int *)(s+0x74)=-1;
            if(!func_L14_003DE908(self)) {
                *(int *)(s+0x6C)=1;
                *(float *)(self+0x18)=*(float *)(s+0x68);
                func_002A7750(self,1,-1);
            }
        } else {
            int active=func_L00_00324120(self)!=0;
            int inside=func_002ACBC8(D_00189E20.pos,*(int *)(s+0x88))==0;
            if(active || inside) func_002AA548(self+0x18,s+0x78,*(float *)(s+0x68),*(float *)(s+0x7C),*(float *)(s+0x7C),0.100000009f);
        }
        break;
    }
    func_00282CF0(&delta,self+0x10,&oldpos);
    func_002AD6F0(s+0x20,&delta,&oldvel,self+0xF0);
}


