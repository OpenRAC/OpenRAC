/* NON_MATCHING func_L00_002472D0 -- src/overlays/shared/map_002465F8.c
 * Best so far: SIZE ours 420 / retail 424, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stash/map-section loader: clears a level struct, advances the two stash cursors at D_L00_001BA070+0xFC/0x100, 
 *   Best p6.c: structure exact (goto-shared clear, struct D_00137C80 entry at +0x8B8), only 27 bytes differ: the t
 *   Store order of the g stores in source has no effect on the schedule; would need a wording that lengthens b's l
 */
#include "common.h"
extern void func_001E9768(void);
extern void func_0020CDE0(int, int);
extern void func_00217628_w(int, int, int) __asm__("func_00217628");
extern void func_00217748(int);
extern void func_L00_00295010(int, int, int, int);
extern int func_00234238(void *dest, unsigned int slot, int offset, int size, int mode);
extern char D_L00_001842F0[];
extern char D_L00_001BA070[] NOT_SDA;
extern unsigned char D_0013D5E9 NOT_SDA;
extern int D_00137C80[];
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;

/* Sets up the level stash area and starts loading the selected map section. */
void func_L00_002472D0(void) {
    char *g = D_L00_001842F0;
    char *p;
    int a, b, c, sz, e, one, v;
    char *z;
    func_001E9768();
    func_0020CDE0(*(int *)(g + 0x224), 1);
    p = D_L00_001BA070;
    a = *(int *)(p + 0xFC);
    b = *(int *)(p + 0x100);
    c = a + 0x9A800;
    *(int *)(p + 0xFC) = c + 0x48000;
    *(int *)(p + 0x100) = b + 0x48000;
    *(int *)(g + 0x23C) = a;
    *(int *)(g + 0x278) = 0;
    *(int *)(g + 0x27C) = c;
    *(int *)(g + 0x280) = b;
    *(int *)(g + 0x284) = 0;
    *(int *)(g + 0x288) = 0;
    *(int *)(g + 0x28C) = -1;
    *(int *)(g + 0x290) = -1;
    *(int *)(g + 0x294) = -1;
    *(int *)(g + 0x298) = -1;
    *(int *)(g + 0x29C) = -1;
    *(int *)(g + 0x2A0) = -1;
    if (*(int *)(g + 0x22C) == -1) {
        goto clear;
    }
    if (*(int *)(g + 0x230) == 0 && D_0013D5E9 != 0) {
        char *q = (char *)D_00137C80 + *(int *)(g + 0x224) * 8;
        one = 1;
        sz = *(int *)(q + 0x8BC);
        func_00217628_w(c, *(int *)(q + 0x8B8), sz);
        sz = sz << 7;
        func_00217748(1);
        *(int *)(g + 0x230) = one;
        *(int *)(g + 0x234) = sz;
        func_L00_00295010(*(int *)(g + 0x27C), *(int *)(g + 0x22C), 0, sz);
        *(int *)(g + 0x228) = -2;
        *(int *)(g + 0x2A8) = sz;
        *(int *)(g + 0x290) = D_0015EE84_m + 0x100;
        *(int *)(g + 0x24) = one;
    } else {
        char *h = D_L00_001842F0;
        if (*(int *)(h + 0x22C) == -1) {
            goto clear;
        }
        v = *(int *)(h + 0x234);
        *(int *)(h + 0x2A8) = v;
        func_00234238(*(void **)(h + 0x27C), *(int *)(h + 0x22C), 0, v, 0);
        e = D_0015EE84_m;
        if (D_0013D5E9) e += 0x100;
        *(int *)(h + 0x228) = -2;
        *(int *)(h + 0x290) = e;
        *(int *)(h + 0x24) = 1;
    }
    return;
clear:
    z = D_L00_001842F0;
    *(int *)(z + 0x24) = 0;
}
