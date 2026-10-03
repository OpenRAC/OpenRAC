/* NON_MATCHING func_L15_002E5770 -- src/overlays/shared/vendor_002D7C00.c
 * Best so far: SIZE ours 496 / retail 508, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_934 (projectile): moves, hit-tests, deletes on impact; p0.c is structurally right (496 vs 508 bytes
 *   Only difference: retail keeps moby+0x10 in two saved regs ($s0 and a copy `daddu $s4,$s0,$zero`, used for F10E
 */
#include "common.h"
extern void func_0020D678(void *);
extern float func_001FA748(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_0025A8C0(char *arg, int a, int b, void *src, float scale);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern int func_001F9908(int *arg0);
extern void func_L00_0025A8E8(int, float, void *, int, float, float, int, int, int);
extern void func_L00_00260108(void *, void *, int, float, float);
extern int func_0022ED80(int, int, int);
extern void func_L07_0029C6A8(void *, int, float, float);
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L15_001744D8;
extern int D_L15_0015F6B0 MACRO_ADDR;

// updates a projectile moby: moves it, tests for hits, and deletes it on impact
void func_L15_002E5770(char *moby) {
    char *d = *(char **)(moby + 0x78);
    if (d == 0) {
        func_0020D678(moby);
        return;
    }
    if (*(unsigned char *)(moby + 0x20) == 0) {
        float a[4];
        float b[4];
        char *mp = moby + 0x10;
        float *q = (float *)mp;
        *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40), D_0015EE6C * 6.2831855f);
        func_001F9BD8(a, mp, d);
        func_L00_0025A8C0((char *)b, (int)moby, 1, d, 2.0f);
        *(unsigned short *)((char *)a + 0x2A) = *(unsigned short *)(moby + 0xA6);
        if ((func_L00_001EFFF0(mp, a, 0, moby, b) != 0 || func_L00_001F10E0(0.3f, q, 0, moby) != 0 || func_001F9908((int *)(d + 0x38)) != 0)
            && D_L15_001744D8 != 0 && D_L15_001744D8 != *(char **)(d + 0x3C)) {
            if (*(short *)(D_L15_001744D8 + 0xA6) != 0x4E) {
                func_L00_0025A8E8((int)moby, 0.5f, q, 1, 2.0f, 0.0f, 0, 1, 0);
            }
            func_L00_00260108(moby, q, -1, 0.5f, 13.0f);
            func_0022ED80(0, 0, (int)moby);
            func_0020D678(moby);
            return;
        }
        qcopy((char *)q, a);
        if (D_L15_0015F6B0 % 3 == 0) {
            func_L07_0029C6A8(moby, 0, 30000.0f, 0.1f);
        }
    }
}
