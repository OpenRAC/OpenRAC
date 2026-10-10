/* NON_MATCHING func_L00_0029C070 -- src/overlays/shared/update_0029B6A0.c
 * Best so far: SIZE ours 620 / retail 628, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Weapon vendor demo start: sets up camera vectors in D_L00_001CA7C0, clears two buffers, then loops until state
 *   Difference: register choice for the D_0013E633+0xE1D base (ours $18 hi / $16 t, retail $16 / $18) and scheduli
 *   Budget spent (10 runs). Would need a source form that orders the pseudos differently for the allocator.
 *   (w05) Ported earlier best (p9) with the Rec table via an __asm__ alias (the file's later Rec typedef clashes),
 *   Those are CSE'd hi copies the allocator places differently; no source form found (pointer reuse, range-test fo
 */
#include "common.h"
#include "include_asm.h"

extern int D_L00_001CA7C0[];
extern unsigned char D_0014171B[] NOT_SDA;
extern unsigned char D_0013E633[] NOT_SDA;
extern char D_L00_00179BC0_t[] __asm__("D_L00_00179BC0") NOT_SDA;
extern int D_L00_0015F6BC MACRO_ADDR;
extern int D_L00_0015F4FC MACRO_ADDR;
extern short D_L00_0015F500;
extern short D_L00_001611B0;
extern int D_L00_0016128C MACRO_ADDR;
extern int D_L00_00173F00[];
extern int D_L00_001CA320[];
extern char D_L00_0016C960[];
extern char D_L00_0017C440[];
extern void func_L00_002EC0C8(int);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214358(void *, int, float);
extern void func_001F9BC0(void *);
extern float func_001FA748(float, float);
extern void func_001FA1F8(void *, void *);
extern void func_001F99B0(void *, int, int);
extern void func_00234AC8(int);
extern void func_002348B8(void);
extern void func_L00_00210340(int, int);
extern void func_00205270(int, int);
extern void func_L00_00245E98(int);
extern void func_00205220(int);
extern void func_0022DD68(void);
extern void func_00122598(int);
extern int func_00216960(void);

// start the weapon vendor demo: set up the camera, reset state and wait for the demo to finish
void func_L00_0029C070(void) {
    char *g;
    char *m;
    char *h;
    int sel;
    int t;
    int v;
    int cur;
    char *d;
    func_L00_002EC0C8(2);
    h = (char *)D_0014171B + 0x100B5;
    if (!(*(unsigned short *)(h + 0x5A) >= 6 && *(unsigned short *)(h + 0x5A) <= 7)) *(short *)(h + 0x5A) = 5;
    g = (char *)D_L00_001CA7C0;
    m = *(char **)(g + 0x1C);
    D_L00_0015F6BC = 1;
    m[0x20] = 1;
    D_L00_001CA7C0[0] = 3;
    *(int *)(g + 4) = 0;
    func_001F9EC0(g + 0xC0, &D_L00_001611B0, *(char **)(g + 0x1C) + 0xC0);
    func_001F9BD8(g + 0xC0, g + 0xC0, *(char **)(g + 0x1C) + 0x10);
    *(float *)(g + 0xC8) = *(float *)(g + 0xC8) + 2.0f;
    *(float *)(g + 0xC8) = func_00214358(g + 0xC0, 0, 0.5f);
    func_001F9BC0(g + 0xB0);
    *(float *)(g + 0xB8) = func_001FA748(*(float *)(*(char **)(g + 0x1C) + 0x48), 3.1415927f);
    func_001FA1F8(g + 0x80, g + 0xB0);
    func_001F99B0(D_L00_0016C960, 0, 0x1C0);
    func_001F99B0(D_L00_0017C440, 0, 0x40);
    func_00234AC8(1);
    func_002348B8();
    sel = *(int *)(g + 0x48);
    t = D_L00_001CA320[sel];
    d = (char *)D_0013E633 + 0xE1D;
    cur = *(int *)(d + 0x10B8);
    v = D_L00_0016128C - 0x40000;
    *(int *)(D_L00_0016C960 + 0x5C) = D_L00_00173F00[2] + v;
    *(int *)(D_L00_0016C960 + 0x58) = D_L00_00173F00[1] + v;
    D_L00_0016128C = v;
    D_L00_0015F4FC = 0;
    *(int *)&D_L00_0015F500 = 0;
    if (cur != 0 && sel != cur) {
        func_L00_00210340(0, 0);
    }
    d = (char *)D_0013E633 + 0xE1D;
    *(int *)(d + 0x20B8) = D_L00_001CA7C0[18];
    if (*(int *)(d + 0x20B8) == 0x18) *(int *)(d + 0x20B8) = 0;
    func_00205270(*(int *)(D_L00_00179BC0_t + D_L00_001CA7C0[18] * 0x4C + 0x10), -1);
    func_L00_00245E98(t);
    *(int *)(h + 0x1C) = 0x2734;
    func_00205220(0);
    while (*(short *)(h + 0x5A) != 3) {
        func_0022DD68();
        func_00122598(0);
    }
    func_00216960();
}
