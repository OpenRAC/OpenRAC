#include "include_asm.h"

#include "common.h"
extern float func_00282C48(float a);
extern void func_00283698(void *dst, void *src);
extern void func_00282CF0(void *dst, void *a, void *b);
extern void func_002AFDD8(void *a, void *b, void *c, void *d);
extern void func_00283098(void *dst, void *a, void *mtx);
extern void func_00282CC0(void *dst, void *a, void *b);
extern void func_L14_003C9658(char *e, void *v);
extern unsigned short *D_L14_001FB060[];
extern unsigned char *D_L14_001B259C __attribute__((sda));
extern int D_L14_001B25B8 __attribute__((sda));

/* For every entity of the owner's group (state word 0x68), other than the two states 0xFE and
   0xFD, that func_L14_003C9548 accepts: takes its offset from the owner in the owner's frame;
   when that is beyond 1.0 on the first axis or 1.5 on the second it is pulled back to +-0.5 or
   +-1.25, and the entity is handed the corrected world position (func_L14_003C9658). */
void func_PAL_L14_003E3370(char *self) {
    float mtx[16];
    float off[4];
    char *state = *(char **)(self + 0x68);
    unsigned short *p;
    unsigned char *e;
    int clamped;

    func_00283698(mtx, self + 0xC0);
    for (p = D_L14_001FB060[*(int *)(state + 0x68)];
         (*(int *)(state + 0x68) & 0xFF) != 0xFF && *(int *)(state + 0x68) <= D_L14_001B25B8 &&
         p != 0 && ((p != 0 && (short)p[-1] >= 0) || p == D_L14_001FB060[*(int *)(state + 0x68)]);
         p++) {
        e = D_L14_001B259C + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (e[0x20] != 0xFE) {
            if (e[0x20] != 0xFD) {
                if (func_L14_003C9548((char *)e) != 0) {
                    func_00282CF0(off, e + 0x10, self + 0x10);
                    clamped = 0;
                    func_002AFDD8(self, off, off, mtx);
                    if (func_00282C48(off[0]) > 1.0f) {
                        clamped = 1;
                        if (off[0] > 0.5f)
                            off[0] = 0.5f;
                        else if (off[0] < -0.5f)
                            off[0] = -0.5f;
                    }
                    if (func_00282C48(off[1]) > 1.5f) {
                        clamped = 1;
                        if (off[1] > 1.25f)
                            off[1] = 1.25f;
                        else if (off[1] < -1.25f)
                            off[1] = -1.25f;
                    }
                    if (clamped) {
                        func_00283098(off, off, self + 0xC0);
                        func_00282CC0(off, off, self + 0x10);
                        func_L14_003C9658((char *)e, off);
                    }
                }
            }
        }
    }
}

