/* NON_MATCHING func_L05_0032ABF0 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: BYTES 55/500 (89.0% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L05_0032ABF0 (500 B)
 *   Best: c3.c BYTES 55/500, size and register assignment right (one temp per sub-block pointer, not a
 *   reused `p`; func_L05_0032AA50 takes char *). Left: sched order inside the long store runs (retail loads
 *   all block constants into $f1-$f5 up front and keeps stores in source order). Float-typed zero stores
 *   (c4) change nothing.
 */
#include "common.h"

extern char *D_L05_0015F050 MACRO_ADDR;
extern char D_0013E633[];
extern void func_001F9BC0(void *);
extern void func_L05_0032A9D0(float, float, float, float, float, float, float, float);
extern void func_L05_0032AA50(char *m);

/* Resets the camera moby to the camera point of its spline entry and clears all its smoothing state. */
void func_L05_0032ABF0(void *arg) {
    char *m = arg;
    char *c = *(char **)(D_L05_0015F050 + *(short *)(m + 0x84) * 32 + 0x1C);
    char *d;
    char *p80;
    char *pE0;
    char *ps;
    char *t;
    char *u;
    char *w;
    *(int *)(c + 0x48) = 0;
    d = *(char **)(m + 0x70);
    *(float *)(d + 0x184) = *(float *)(c + 0x28);
    *(float *)(d + 0x188) = *(float *)(c + 0x2C);
    *(float *)(d + 0x180) = *(float *)(c + 0x30);
    *(float *)(d + 0x190) = *(float *)(c + 0x34);
    *(float *)(d + 0x194) = *(float *)(c + 0x38);
    *(float *)(d + 0x18C) = *(float *)(c + 0x3C);
    func_001F9BC0(d + 0x1B0);
    func_001F9BC0(d + 0x1C0);
    p80 = *(char **)(m + 0x70) + 0x80;
    *(int *)(p80 + 0x40) = 5;
    *(int *)(p80 + 0x44) = *(int *)(D_0013E633 + 0x2E9D);
    *(float *)(p80 + 0x48) = 0.209439516f;
    *(float *)(p80 + 0x58) = 0.00174532924f;
    *(float *)(p80 + 0x5C) = -0.0043633231f;
    *(float *)(p80 + 0x4C) = *(float *)(d + 0x180);
    pE0 = *(char **)(m + 0x70) + 0xE0;
    *(float *)(pE0 + 0x0) = *(float *)(d + 0x184);
    *(float *)(pE0 + 0x4) = 0.5f;
    *(float *)(pE0 + 0x10) = *(float *)(d + 0x188);
    ps = *(char **)(m + 0x70);
    *(float *)(ps + 0x10) = 0.01f;
    *(float *)(ps + 0x14) = 0.2f;
    *(int *)(ps + 0x18) = 0;
    *(short *)(ps + 0x1C) = 0;
    func_001F9BC0(ps);
    t = *(char **)(m + 0x70);
    u = t + 0x100;
    func_001F9BC0(u);
    func_001F9BC0(t + 0x120);
    *(int *)(u + 0x40) = 0;
    *(float *)(u + 0x10) = *(float *)(c + 0x0);
    *(float *)(u + 0x34) = *(float *)(c + 0x20);
    *(int *)(u + 0x3C) = 0;
    *(float *)(u + 0x38) = *(float *)(c + 0x24);
    w = *(char **)(m + 0x70) + 0x30;
    *(float *)(w + 0x20) = 0.8f;
    *(float *)(w + 0x24) = 0.3f;
    *(float *)(w + 0x40) = 1.33333337f;
    *(int *)(w + 0x28) = 0;
    *(short *)(w + 0x2C) = 0;
    func_001F9BC0(w);
    func_L05_0032A9D0(*(float *)(c + 0x28), *(float *)(c + 0x2C), *(float *)(c + 0x30), *(float *)(c + 0x34),
                      *(float *)(c + 0x38), *(float *)(c + 0x3C), *(float *)(c + 0x40), *(float *)(c + 0x44));
    func_L05_0032AA50(m);
    *(short *)(m + 0x7E) = 0;
}
