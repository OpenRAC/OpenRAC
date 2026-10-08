/* NON_MATCHING func_L03_002ECBA8 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: BYTES 4/404 (99.0% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L03_002ECBA8 (404 B)
 *   Best: c1.c BYTES 4/404. Only difference: before the first call retail moves a1 (g) before a0 (v1 ptr);
 *   ours moves a0 first. Tried: g position (c2/c3), inline g (c5), v1 pointer local (c6), float temp (c7),
 *   v1/v3 swap (c4, layout breaks), flags -fno-force-mem/-fno-regmove (no change), -fno-schedule-insns(2) worse.
 */
#include "common.h"

extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern char D_0013E633[];

// Places a camera-like moby behind the hero and builds its look-at basis.
void func_L03_002ECBA8(void *arg) {
    char *m = arg;
    char *d = *(char **)(m + 0x70);
    char *g = D_0013E633 + 0x10AD;
    char *at = m + 0x30;
    char *dir = m + 0x40;
    char *b = d + 0xF0;
    char *t = d + 0x100;
    char *c = d + 0xB0;
    float v3[4];
    float v1[4];
    float v2[4];
    float v4[4];
    float v5[4];
    func_L00_001FF4B0(v1, g, -*(float *)(d + 0xE0));
    func_L00_001FF4B0(v3, b, -*(float *)(d + 0xD0));
    func_001F9BD8(v2, D_0013E633 + 0xE9D, v1);
    func_001F9BD8(at, v2, v3);
    func_L00_001FF4B0(v4, g, -*(float *)(c + 0x14));
    func_001F9BD8(v4, D_0013E633 + 0xE9D, v4);
    func_001F9BF0(v5, v4, at);
    func_L00_001FF4B0(t, v5, 1.0f);
    func_L00_001FF4B0(m, t, 1.0f);
    func_L00_001FF4B0(dir, m, 1.0f);
    qcopy(m, dir);
    func_001F9CA0(m + 0x10, m, g);
    func_L00_001FF4B0(m + 0x10, m + 0x10, -1.0f);
    func_001F9CA0(m + 0x20, m + 0x10, m);
}
