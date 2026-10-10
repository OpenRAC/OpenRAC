/* NON_MATCHING func_L05_00317CD8 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: BYTES 4/384 (99.0% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L05_00317CD8 (384 B)
 *   Best: c5.c BYTES 4/384. Only difference: the phase temp `(fr & 0x7F) * 0.0078125f` is in $f12 in ours,
 *   $f0 in retail (then mul.s $f12, $f0, $f23 for the sin argument).
 *   Levers that got here: callers' (char *, int) prototype, the frame counter read into an int local
 *   after the clamp (retail fills the bc1f delay slot with it), 0x3F1C61AA = 0.61086524f.
 *   Tried without effect: phase local (c6), x / 128.0f (c7), PI * ph (c8), reuse w (c9, worse),
 *   ph at function scope (d1), split mul (d2), float ph = fr & 0x7F (d3).
 */
#include "common.h"

extern float D_0015EE64 MACRO_ADDR;
extern int D_L05_0015F6B0 MACRO_ADDR;
extern short D_L05_00161F90;
extern float func_001F9FA8(float);
extern void func_L00_00263950(char *, char *, int, float, float);

/* Updates the two trails of the moby; with the cheat counter on, they get wider and pulse. */
void func_L05_00317CD8(char *m, int t_) {
    char *t = (char *)t_;
    int i;
    for (i = 0; i < 2; i++) {
        char *e = t + i * 0x80;
        if (*(int *)&D_L05_00161F90 != 0) {
            float w = *(int *)&D_L05_00161F90 * 0.15f + 1.0f;
            int fr;
            if (3.7f < w) w = 3.7f;
            fr = D_L05_0015F6B0;
            *(float *)(e + 0x1B0) = w;
            *(float *)(e + 0x1A4) = func_001F9FA8((fr & 0x7F) * 0.0078125f * 3.1415927f) * 0.61086524f;
        }
        func_L00_00263950(m, e + 0x140, i + 2, D_0015EE64 * 0.05f, D_0015EE64 * 0.3f);
    }
}
