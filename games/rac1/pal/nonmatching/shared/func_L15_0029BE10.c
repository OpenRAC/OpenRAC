/* NON_MATCHING func_L15_0029BE10 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: BYTES 33/484 (93.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_49: plays a sound when near the player, then (state 0) builds a swept segment, calls the collision 
 */
#include "common.h"
extern char D_0013E633[];
extern char D_L15_00167440[];
extern int D_L15_001744D8;
extern float D_0015EE6C MACRO_ADDR;
extern float func_001F9D10(void *, void *);
extern int func_L00_0028EF68(int i, int a1, int v, int k);
extern float func_001FA748(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern int func_002140B0(int);
extern void func_L00_0025A8C0(char *arg, void *a, int b, void *src, float scale);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern int func_001F9908(int *arg0);
extern void func_L15_0029C168(void *);
extern void func_0020D678(void *);
extern void func_L15_0029BFF8(char *moby);

// Update for moby class 49: plays a sound near the player, then moves toward the target until it hits something.
void func_L15_0029BE10(unsigned char *moby) {
    char *d = *(char **)(moby + 0x78);
    float v[16];
    char *pos;
    float s;
    char *pv;
    char *pos2;
    char *p1;
    char *vec2;
    if (*(int *)(d + 0x10) == 0) {
        if (func_001F9D10(moby + 0x10, D_L15_00167440) < 15.0f) {
            func_L00_0028EF68(9, 0, (int)moby, 0x27E);
            *(int *)(d + 0x10) = 1;
        }
    }
    if (moby[0x20] == 0) {
        pos = (char *)moby + 0x10;
        s = 1.0f;
        *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40), D_0015EE6C * 6.2831855f);
        func_001F9BD8(v, pos, d);
        p1 = (char *)(v + 4);
        func_001F9C30(p1, d, -2.0f);
        func_001F9BD8(p1, p1, pos);
        pv = p1;
        if (*(unsigned char *)(D_0013E633 + 0x2EC1) == 2 && func_002140B0(5) != 0) {
            s = 0.0f;
        }
        p1 = (char *)(v + 8);
        vec2 = p1;
        func_L00_0025A8C0(vec2, moby, 0x10003, d, s);
        *(short *)((char *)v + 0x3A) = *(unsigned short *)(moby + 0xA6);
        if (func_L00_001EFFF0(pv, v, 0x10, moby, vec2) != 0 || func_001F9908((int *)(d + 0x14)) != 0) {
            if (D_L15_001744D8 != *(int *)(d + 0x18)) {
                func_L00_0028EF68(8, 0, (int)moby, 0x27E);
                func_L15_0029C168(moby);
                func_0020D678(moby);
                return;
            }
        }
        qcopy(moby + 0x10, v);
        func_L15_0029BFF8((char *)moby);
    }
}
