/* NON_MATCHING func_L14_002D7880 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: BYTES 115/632 (81.8% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L14_002D7880 (632 B)
 *   Best: d4.c, size exact, BYTES 115/632. Levers used: the moby base needs an incomplete-array MACRO_ADDR
 *   alias (lui/lw each iteration, not $gp, not a hoisted explicit %hi); the second loop's base is spelled
 *   through D_0013F450 so gcc does not keep the first %hi alive; path + (idx * 16 + 0x40) index-first;
 *   idx copied to a local before the G->564 store. Left: $s4/$s5 swapped between g and the hoisted table base
 *   (and the loop-2 counter), plus the resulting argument-setup order. Table-local variants (e1/e2) don't help.
 */
#include "common.h"

extern char D_0013E633[];
extern char D_0013F450_d78[] __asm__("D_0013F450");
extern short *D_L14_001AC2C0_d78[] __asm__("D_L14_001AC2C0");
extern char *D_L14_00160098_d78[] __asm__("D_L14_00160098") MACRO_ADDR;
extern char *D_L14_0015F7EC_d78 __asm__("D_L14_0015F7EC") MACRO_ADDR;
extern char *D_L14_001B0F30_d78[] __asm__("D_L14_001B0F30");
extern short D_L14_00161B08;
extern int func_L00_0025EFC0(void *, void *, float *, int *, float *, int, float, float, float);
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern void func_L00_00217718(void *, void *, int, int);
extern int func_L00_0025E860_d78(void *, void *, void *, void *, int, float) __asm__("func_L00_0025E860");
extern void func_001F9BC0(void *);
extern void func_L00_0023F1D0(int);

/* Puts the three trailing mobys of the list on the hero's current path, at the hero's point, facing the moby. */
void func_L14_002D7880(char *m) {
    char *list[4];
    float pos[4];
    float v[4];
    float rot[4];
    int idx;
    float t;
    short *p = D_L14_001AC2C0_d78[((unsigned char *)m)[0x21]];
    char **lp;
    char *g;
    int i;
    if (p == 0) return;
    g = D_0013E633 + 0xE1D;
    lp = list;
    do {
        char *o = D_L14_00160098_d78[0] + ((*(unsigned short *)p & 0x7FFF) << 8);
        char *path = *(char **)(g + 0x560);
        char *od = *(char **)(o + 0x78);
        if (path == *(char **)(D_L14_0015F7EC_d78 + *(int *)(od + 0x60) * 32 + 0x10)) {
            t = 0.0f;
            func_L00_0025EFC0(path, D_L14_001B0F30_d78[*(int *)(od + 0x88)] + 0x10, pos, &idx, &t, 0, 20.0f, 5.0f, 0.0f);
            func_001F9BF0(v, path + (idx * 16 + 0x40), path + (idx * 16 + 0x10));
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[2] = func_001FA748(func_L00_001FF860(v[0], v[1]), -1.5707964f);
            rot[3] = 0.0f;
            {
                int k = idx;
                *(int *)(g + 0x564) = k;
                *(float *)(g + 0x568) = 0.0f;
                func_L00_00217718(path + (k * 16 + 0x10), rot, 0x28, 0);
            }
        }
        *lp++ = o;
    } while (*p++ >= 0);
    g = D_0013F450_d78;
    for (i = 0; i < 3; i++) {
        char *o = list[i];
        char *od = *(char **)(o + 0x78);
        char *path = *(char **)(D_L14_0015F7EC_d78 + *(int *)(od + 0x60) * 32 + 0x10);
        o[0x20] = 6;
        *(float *)(o + 0x48) = func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(m + 0x10), *(float *)(g + 0x84) - *(float *)(m + 0x14));
        *(int *)(od + 0x64) = *(int *)(g + 0x564);
        *(float *)(od + 0x68) = *(float *)(g + 0x568);
        func_L00_0025E860_d78(path, o + 0x10, od + 0x64, od + 0x68, 0, -*(float *)&D_L14_00161B08);
        func_001F9BC0(od + 0x90);
        *(int *)(od + 0xF8) = i * 2;
        *(int *)(od + 0xFC) = i * 2 + 1;
        if (*(int *)(od + 0xF0) != -1) {
            func_L00_0023F1D0(*(int *)(od + 0xF0));
            *(int *)(od + 0xF0) = -1;
        }
    }
}
