/* NON_MATCHING func_L03_002ECD40 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: BYTES 40/372 (89.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ActivateCamera_14 (level 3): seeds the camera's smoothing state (three sub-blocks of the moby's data at +0xB0/
 *   Best p8.c (43 bytes differ of 372, same size): final D_L03_0015F050 load is scheduled after `lw $s0,0x70(m)` i
 *   Pointer forms (char+, float*, index-first, struct array) all compile to the same bytes or worse; needs a diffe
 *   t03/q28: p9 = p8 with D_L03_0015F050 declared int MACRO_ADDR (packet's type; the old char* clashed with the fi
 */
#include "common.h"
extern char D_0013E633[];
extern int D_L03_0015F050 MACRO_ADDR;
extern int D_L03_001670F4;
extern void func_001F9BC0(void *);
extern int func_001F9850(int);
extern void func_001FA1F8(void *, void *);
extern void func_L03_002ECBA8(void *);

/* ActivateCamera_14: seed the camera's smoothing state from the current camera table entry */
void func_L03_002ECD40(char *m) {
    char *g = D_0013E633 + 0xE1D;
    char *e = *(char **)((char *)D_L03_0015F050 + *(short *)(m + 0x84) * 32 + 0x1C);
    char *d1;
    char *v;
    char *a;
    char *b;
    char *c;
    float *e2;
    int dd;
    float t;
    float buf[16];
    d1 = *(char **)(m + 0x70) + 0xB0;
    *(float *)(d1 + 0x14) = 1.5f;
    *(int *)(d1 + 0x10) = *(int *)(g + 0x2080);
    qcopy(d1, g + 0x80);
    *(int *)(d1 + 0xC) = 0;
    v = *(char **)(m + 0x70) + 0xD0;
    t = *(float *)(e + 0x20);
    *(float *)(v + 8) = t;
    *(float *)(v + 0) = t;
    *(float *)(v + 0x10) = *(float *)(e + 0x24);
    a = *(char **)(m + 0x70);
    *(int *)(a + 0x28) = 0;
    *(float *)(a + 0x20) = 0.01f;
    *(float *)(a + 0x24) = 0.2f;
    func_001F9BC0(a + 0x10);
    qcopy(a, g + 0x80);
    b = *(char **)(m + 0x70);
    *(int *)(b + 0x30) = func_001F9850(0x78);
    dd = D_L03_0015F050;
    c = *(char **)(m + 0x70) + 0xF0;
    *(int *)(c + 0x30) = 0;
    *(int *)(c + 0x28) = 0;
    *(int *)(c + 0x2C) = 0;
    *(int *)(c + 0x20) = 0;
    *(float *)(c + 0x34) = *(float *)(g + 0x88);
    *(int *)(c + 0x24) = 0;
    e2 = (float *)(*(short *)(m + 0x84) * 32 + dd) + 4;
    buf[12] = e2[0];
    buf[13] = e2[1];
    buf[15] = 0.0f;
    buf[14] = e2[2];
    func_001FA1F8(buf, buf + 12);
    qcopy(c, buf);
    func_L03_002ECBA8(m);
    *(short *)(m + 0x7E) = 0;
    D_L03_001670F4 = 0x78;
}
