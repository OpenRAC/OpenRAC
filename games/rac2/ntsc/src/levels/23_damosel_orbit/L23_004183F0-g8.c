#include "include_asm.h"

/* func_L23_004183F0 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern float D_L23_001B15B4 __attribute__((sda));

extern TI128 D_L23_001B3800 __attribute__((sda));

extern char D_L23_001B3810[16] __attribute__((sda));

extern volatile int D_L23_001B37F0;

extern volatile int D_L23_001B37F4;

extern volatile int D_L23_001B36AC;

extern volatile int D_L23_001B36B0;

extern char *D_L23_00259560[];

extern void func_0029FCC0(char *, int, char *);

extern void func_0029FC20(char *, int, char *);

extern void func_00282D30(char *, char *, float);

extern void func_00282CC0(char *, char *, char *);

extern int func_00282948(char *);

extern void func_0029F338(int);

extern int func_L23_003C19E0(char *, float *);

extern void func_L23_003C1AB0(char *);

extern void func_L23_00418160(char *);

void func_L23_004183F0(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    float v0[4];
    float v1[4];
    float m[16];
    float dist;
    char *best;
    float bestDist;
    char **p;
    int i;
    int j;
    int a;
    int b;
    int ok;

    if (*(int *)(state + 0x59C) != 0) {
        func_0029FCC0(arg0, *(int *)(state + 0x594) + 0x1E, (char *)v1);
        D_L23_001B3800 = *(TI128 *)v1;
        func_00282D30(D_L23_001B3810, arg0 + 0xE0, D_L23_001B15B4);
        func_00282CC0(D_L23_001B3810, D_L23_001B3810, *(char **)(state + 0x59C) + 0x10);
        func_0029FC20(arg0, *(int *)(state + 0x594) + 0x1E, (char *)m);
        func_00282D30((char *)v0, (char *)(m + 8), -16.0f);
        func_00282CC0((char *)v1, (char *)v1, (char *)v0);
        func_00282948(state + 0x598);
        if (*(int *)(state + 0x598) == 0) {
            *(int *)(state + 0x59C) = 0;
            *(int *)(state + 0x594) = (*(int *)(state + 0x594) + 1) % 4;
            a = D_L23_001B37F0;
            if (a != 0) {
                func_0029F338(a);
                D_L23_001B37F0 = 0;
            }
            b = D_L23_001B37F4;
            if (b != 0) {
                func_0029F338(b);
                D_L23_001B37F4 = 0;
            }
        }
    }
    D_L23_001B36AC = 0;
    p = D_L23_00259560;
    i = 24;
    do {
        i--;
        if (!(*(unsigned short *)(*p + 0x34) & 1))
            D_L23_001B36AC = D_L23_001B36AC + 1;
        p++;
    } while (i >= 0);
    best = 0;
    bestDist = 3000.0f;
    if (D_L23_001B36B0 == 0) {
        for (j = 0; j < 25; j++) {
            char *obj = D_L23_00259560[j];

            ok = !(*(unsigned short *)(obj + 0x34) & 1);
            if (ok && func_L23_003C19E0(obj, &dist) != 0) {
                if (best == 0) {
                    best = obj;
                    bestDist = dist;
                } else if (dist < bestDist) {
                    best = obj;
                    bestDist = dist;
                }
            }
        }
    }
    if (best != 0)
        func_L23_003C1AB0(best);
    if (*(int *)(state + 0x590) != 0 && *(int *)(state + 0x598) <= 0)
        func_L23_00418160(arg0);
}
