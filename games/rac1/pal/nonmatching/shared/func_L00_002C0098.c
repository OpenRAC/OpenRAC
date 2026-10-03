/* NON_MATCHING func_L00_002C0098 -- src/overlays/shared/vendor_002BA7C8.c
 * Best so far: SIZE ours 700 / retail 704, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Picks the best-scoring object in the D_L00_001ABD80 list for a target position (distance, z-gap and line-of-si
 *   Difference: 4 bytes. Retail has `bc1f; nop` before `lui/mtc1 15.0f` (the `if (dz > 3.25f) e += 15.0f` arm); ou
 *   Would unblock: some wording that stops reorg from hoisting the constant load into the bc1f slot (maybe a diffe
 */
#include "common.h"
extern char *D_L00_001ABD80[];
extern char D_0013E633[] NOT_SDA;
extern float func_001F9D10(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9B88(float);
extern int func_L00_001EFFF0_v(void *, void *, int, void *, int) __asm__("func_L00_001EFFF0");
extern float func_001F9D48(void *, void *);

/* Picks the nearest usable object in the global list for a target position, falling back to a default. */
char *func_L00_002C0098(char *a, float *b, float *c, int unused, int flag) {
    char **pp;
    char *o;
    char *best = 0;
    int ok = 1;
    float bestd = 1.0e9f;
    Vx v;

    for (pp = D_L00_001ABD80; (o = *pp) != 0; pp++) {
        char *u = *(char **)(o + 0x24);
        int h = 0;
        float d;
        float x;
        float y;
        float z;
        float w;
        int r;
        float e;
        float tot;

        if (u != 0) {
            h = *(short *)(u + 0x46);
        }
        if (h != 5) {
            continue;
        }
        d = func_001F9D10(b, o + 0x10);
        if (d > 40.0f) {
            continue;
        }
        if (d > 30.0f) {
            ok = 0;
        }
        qcopy(&v, o + 0x10);
        v.z = v.z + 0.4f;
        x = func_L00_001FF860(v.x - b[0], v.y - b[1]);
        y = func_001FA850(c[2], x);
        y = y * y;
        z = func_L00_001FF860(d, v.z - b[2]);
        w = func_001FA850(c[1], z);
        w = w * w;
        if (ok) {
            if (func_001F9B88(*(float *)(o + 0x18) - *(float *)(a + 0x18)) > 8.0f) {
                r = 1;
            } else {
                r = func_L00_001EFFF0_v(b, &v, 6, o, 0) != 0;
            }
        } else {
            r = *(unsigned char *)(o + 0x31) == 0;
        }
        if (flag == 0 && r != 0) {
            continue;
        }
        e = func_001F9D10(b, &v);
        d = 0.0f;
        e = *(float *)(o + 0x18) - *(float *)(a + 0x18) > 3.25f ? e + 15.0f : e;
        if (r != 0) {
            d = 5.0f;
        }
        tot = e + d + w + y;
        if (tot < bestd) {
            bestd = tot;
            best = o;
        }
    }
    if (best == 0) {
        char *g = D_0013E633 + 0xE9D;
        if (func_001F9D48(a + 0x10, g) > 3.2f) {
            best = *(char **)(g + 0x2000);
        }
    }
    return best;
}
