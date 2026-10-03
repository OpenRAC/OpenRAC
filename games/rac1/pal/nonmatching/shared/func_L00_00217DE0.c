/* NON_MATCHING func_L00_00217DE0 -- src/overlays/shared/help_00214D60.c
 * Best so far: SIZE ours 780 / retail 792, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hero spawn setup: derives the camera heading from the nearest target, builds an orientation from it and starts
 *   Best candidate p5.c (780 vs 792 bytes): frame and local layout match (Vx,Vx,48-byte,48-byte,Mx), but the flag 
 *   Tried if/!/ternary/temp-int wordings of the flag: all give slt. Unblock: a wording that keeps flag=1 un-simpli
 */
#include "common.h"
typedef struct { float a[12]; } M3 __attribute__((aligned(16)));
extern char D_0013E633[] NOT_SDA;
extern char D_L00_00166D80[];
extern float D_0015EE6C MACRO_ADDR;
extern int func_001F9850(int);
extern int func_L00_00211A18(void *);
extern void func_L00_00233EE0(float *, float, float, float);
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9CE8(void *);
extern float func_001FA748(float, float);
extern int func_L00_00217570(int a, int b);
extern void func_001F9BC0(void *);
extern void func_L00_00233F88(float *dst, float *src, float r);
extern void func_00215C00(void *, float, float, float);
extern void func_001FA1F8(void *, void *);
extern void func_001FA4F0(void *, void *, void *);
extern void func_001FA460(void *, void *);
extern void func_002153E8(void *, void *);
extern void func_L00_00263578(int a, char *b);
extern void func_L00_002635A0(char *o, int a, int b);
extern void func_00213DE0(void *, int, int, int);

/* sets up the hero's spawn moby: heading, camera-relative orientation and initial anim */
void func_L00_00217DE0(void) {
    Vx A;
    Vx B;
    M3 C;
    M3 D;
    Mx E;
    int flag = 0;
    int got;
    char *m;
    char *d;
    char *h;
    char *g4;
    char *g = D_0013E633 + 0xE1D;
    if (*(int *)(g + 0x2084) == 1 || *(int *)(g + 0x2084) == 0x1E) {
        flag = 1;
        flag = (func_001F9850(0x14) < *(int *)(g + 0x198)) ? flag : 0;
    }
    if (flag) {
        char *g2 = D_0013E633 + 0xE1D;
        m = (char *)*(int *)(g2 + 0x1090);
        got = 0;
        if (m != 0 && func_L00_00211A18(m)) {
            d = (char *)*(int *)(m + 0x78);
            if (*(short *)(d + 0x7E) != 0) {
                got = 1;
                func_L00_00233EE0((float *)&A, 0.5f, 0.0f, 0.5f);
                func_001F9BF0(&B, d + 0x50, &A);
                *(float *)(g2 + 0xA74) = func_L00_001FF860(B.x, B.y);
                *(float *)(g2 + 0xA78) = func_L00_001FF860(func_001F9CE8(&B), B.z);
            }
        }
        if (!got) {
            char *g3 = D_0013E633 + 0xE1D;
            *(float *)(g3 + 0xA74) = *(float *)(D_L00_00166D80 + 0x158);
            *(float *)(g3 + 0xA78) = func_001FA748(-*(float *)(D_L00_00166D80 + 0x154), 0.12f);
        }
    }
    g4 = D_0013E633 + 0xE1D;
    m = (char *)*(int *)(g4 + 0x1090);
    if (func_L00_00211A18(m)) {
        *(char *)(g4 + 0x10AA) = 1;
        if (flag) func_L00_00217570(0x1B, 0);
        *(char *)(m + 0x20) = 10;
        d = (char *)*(int *)(m + 0x78);
        if (*(float *)(g4 + 0x248) < 1.2f
            && 0.66f < *(float *)(g4 + 0x250)
            && *(unsigned char *)(g4 + 0x254) == 0)
            *(short *)(d + 0x76) = 1;
        func_001F9BC0(d + 0x40);
        if (!flag) func_L00_00233F88((float *)(d + 0x40), (float *)(d + 0x40), 1.0f);
        else func_00215C00(d + 0x40, 1.0f, *(float *)(g4 + 0xA74), *(float *)(g4 + 0xA78));
        func_001F9BC0(&A);
        func_001FA1F8(&C, &A);
        if (flag) {
            A.x = *(float *)(D_L00_00166D80 + 0x150);
            A.y = *(float *)(D_L00_00166D80 + 0x154);
            A.z = *(float *)(D_L00_00166D80 + 0x158);
            func_001FA1F8(&D, &A);
            func_001FA4F0(&C, &D, &C);
        } else {
            func_001FA4F0(&C, (char *)*(int *)(g4 + 0x2080) + 0xC0, &C);
        }
        func_001FA460(&E, &C);
        func_002153E8(&E, m + 0x40);
        *(int *)(d + 0x64) = 0;
        *(float *)(d + 0x60) = D_0015EE6C * 23.0f;
        h = D_0013E633 + 0x25CD;
        func_L00_00263578((int)m, h);
        func_L00_002635A0(h, 0x30, 3);
        func_L00_002635A0(h, 0x17, 5);
        func_00213DE0(m, 6, 0, func_001F9850(5));
    }
}
