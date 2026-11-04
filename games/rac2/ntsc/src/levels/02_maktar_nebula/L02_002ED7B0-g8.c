#include "include_asm.h"

extern void func_002E37B0(void);

extern void func_002E3548(void);

extern void func_002E3820(void);

extern void func_002FC560(unsigned int, unsigned long);

extern void func_L02_00363D78(void);

extern int D_001A79F0;

extern int D_001A72E0;

extern char D_L02_00280A10[];

extern int D_L02_001B33D0;

/* func_L02_003CB090 needs a $gp member and an absolute one; the measured
   92-of-96-byte candidate is recorded in nonmatching/levels/func_L02_003CB090.c
   (residue: branch-likely + which register gets the absolute address). */

extern int func_002ACBC8(void *point, int index);

extern void func_00282CF0(float *out, const float *a, const float *b);

extern void func_00283098(float *out, const float *a, const float *b);

extern void func_L02_002DCEA0(int a0, float a1);

extern void func_L00_002B9D60(int a0, int a1, int a2, int a3);

extern char D_00189EA0[];

extern char D_L02_001BA500[];

extern char *D_L02_001B368C;

extern float D_L02_001B2FE0 __attribute__((sda));


void func_L02_00300738(void) {
    func_002E37B0();
    D_L02_001B33D0 = (int)D_L02_00280A10;

    if (D_001A79F0 == 2)
        func_L02_00363D78();
    else
        func_002E3548();
    func_002E3820();
    func_002FC560(0x47, 0x5360B);
    func_002FC560(0x4E, 0x01000000 | (D_001A72E0 >> 13));
}

/* Flags the actor's target when its record is on screen, and refreshes it afterwards. */
void func_L02_003D02F0(char *arg0) {
    char *p = *(char **)(arg0 + 0x68);
    float v[4];

    if (*(int *)(p + 0xDC) == 0) {
        if (func_002ACBC8(D_00189EA0, *(int *)(p + 0xC4)) != 0) {
            func_00282CF0(v, (float *)D_L02_001BA500,
                          (float *)(D_L02_001B368C + (*(int *)(p + 0xC4) << 7) + 0x30));
            func_00283098(v, v, (float *)(D_L02_001B368C + (*(int *)(p + 0xC4) << 7) + 0x40));
            if (v[0] > 0.0f) {
                *(int *)(p + 0xDC) = 1;
                func_L02_002DCEA0(1, 10.0f);
            }
        }
    } else {
        if (D_L02_001B2FE0 >= 1.0f) {
            func_L00_002B9D60((int)D_00189EA0,
                              (int)(D_L02_001B368C + (*(int *)(p + 0xC4) << 7) + 0x70), 0, 1);
            *(short *)(D_00189EA0 + 0x160) = 10;
        }
    }
}

extern unsigned short *D_L02_001FC060[];

extern unsigned char *D_L02_001B359C;

/* Answers 1 when the object's group holds an armed 0x9CA entity (class byte 6, state word
   +0x2F6 == 1) that is not being freed (state byte 0xFE or 0xFD). */
int func_L02_003D18D8(unsigned char *a0) {
    unsigned short *q;
    unsigned short *p;
    unsigned char *e;

    if (a0[0x21] == 0xFF)
        return 0;
    q = D_L02_001FC060[a0[0x21]];
    if (q == 0)
        return 0;
    p = q;
    do {
        e = D_L02_001B359C + ((*p & 0x7FFF) << 8);
        if (e != 0 && e[0x20] != 0xFE && e[0x20] != 0xFD && *(short *)(e + 0xAA) == 0x9CA &&
            e[0x95] == 6 && *(short *)(*(unsigned char **)(e + 0x68) + 0x2F6) == 1)
            return 1;
    } while (*(short *)p++ >= 0);
    return 0;
}

extern int D_L02_001B35B8 __attribute__((sda));

extern int func_002B3BA8(char *, int, float, float, int, int);

extern int func_L02_003FC0D0(int, short);

extern float func_00282E80(void *, void *);

/* Pairs the actor with a partner: keeps the one it already has (state +0x2E0, mode +0x2F6),
   or picks the nearest free 0x9CA entity of its group and links the two.
   The early exit writes its own `+0x2F6 = 0` (jump2's cross-jumping merges it with the last
   one, which gives retail's `b` to that store); a `goto` to the shared store instead leaves
   `state` one reference short, and global.c then gives `self` the lower saved register. The
   two early stores are in the reverse of retail's order (the scheduler swaps them). */
void func_L02_003D1998(char *self) {
    char *state = *(char **)(self + 0x68);
    unsigned char *other;
    unsigned short *p;
    unsigned char *e;
    unsigned char *best;
    float min;

    if (*(unsigned char *)(self + 0x20) < 4) {
        *(short *)(state + 0x2F6) = 0;
        *(int *)(state + 0x2E0) = 0;
        return;
    }
    if (*(short *)(state + 0x2F6) == 1) {
        *(short *)(state + 0x2FA) =
            func_002B3BA8(self, *(int *)(state + 0x2F0), 100.0f, 100.0f, 0, 0);
        *(int *)(state + 0x2E0) = *(int *)(state + 0x200);
    } else if (func_L02_003FC0D0(*(int *)(self + 0xB8), *(short *)(self + 0xAA)) != 0) {
        *(short *)(state + 0x2F6) = 1;
        *(short *)(state + 0x2FA) =
            func_002B3BA8(self, *(int *)(state + 0x2F0), 100.0f, 100.0f, 0, 0);
        *(int *)(state + 0x2E0) = *(int *)(state + 0x200);
    } else if (*(short *)(state + 0x2F6) == 2 && *(char **)(state + 0x2E0) != 0) {
        char *os;

        other = *(unsigned char **)(state + 0x2E0);
        os = *(char **)(other + 0x68);
        if (other[0x20] != 0xFE) {
            if (other[0x20] != 0xFD) {
                if (other[0x95] != 0) {
                    if (other[0x95] != 1 && *(short *)(os + 0x2F6) == 2 &&
                        *(char **)(os + 0x2E0) == self) {
                        *(short *)(state + 0x2FA) = 2;
                        *(short *)(os + 0x2FA) = 2;
                        return;
                    }
                }
            }
        }
        *(int *)(state + 0x2E0) = 0;
        *(short *)(state + 0x2F6) = 0;
    } else {
        min = 1000.0f;
        best = 0;
        for (p = D_L02_001FC060[*(unsigned char *)(self + 0x21)];
             *(unsigned char *)(self + 0x21) != 0xFF &&
             *(unsigned char *)(self + 0x21) <= D_L02_001B35B8 && p != 0 &&
             ((p != 0 && (short)p[-1] >= 0) ||
              p == D_L02_001FC060[*(unsigned char *)(self + 0x21)]);
             p++) {
            e = D_L02_001B359C + ((*p & 0x7FFF) << 8);
            if (e == 0)
                break;
            if (e[0x20] != 0xFE) {
                if (e[0x20] != 0xFD && e != (unsigned char *)self && e[0x20] >= 3 &&
                    *(short *)(e + 0xAA) == 0x9CA &&
                    *(short *)(*(char **)(e + 0x68) + 0x2F6) == 0) {
                    if (func_00282E80(self + 0x10, e + 0x10) < min) {
                        best = e;
                        min = func_00282E80(self + 0x10, e + 0x10);
                    }
                }
            }
        }
        if (best != 0) {
            char *bs;

            *(unsigned char **)(state + 0x2E0) = best;
            *(short *)(state + 0x2F6) = 2;
            *(short *)(state + 0x2FA) = 2;
            bs = *(char **)(best + 0x68);
            *(short *)(bs + 0x2F6) = 2;
            *(char **)(bs + 0x2E0) = self;
            *(short *)(bs + 0x2FA) = 2;
        } else {
            *(short *)(state + 0x2F6) = 0;
        }
    }
}
