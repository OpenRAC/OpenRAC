/* NON_MATCHING func_L00_002709A0 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: SIZE ours 304 / retail 312, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   PartType38Update: particle rises and fades (alpha = v5*255, v5 -= 0.01*D_0015EE60 clamped at 0), killed when
 *   9938(m+0xA) fires or v5 <= 0, else FastVecAdd + FastVecScale by 1 - 0.02*D_0015EE60.
 *   Best: p6.c (size 312 matches). Remaining diff is layout/scheduling: retail puts the KillPart block first
 *   (if (kill) {Kill} else {add, scale}) and loads v[4] before m[0xC]; the f20 zero is created after the first cal
 *   comma-expression form (p8.c) could not be tested (budget spent). Unblock: try p8.c shape.
 */
extern int func_001F9938(void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_002688A8(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float D_0015EE60 MACRO_ADDR;

/* Particle update: rise and fade out, kill once fully transparent. */

void func_L00_002709A0(char *m) {
    char *v = m + 0x20;
    float t;
    float z = 0.0f;
    if (!func_001F9938(m + 0xA)) {
        m[8] += 1;
        *(float *)(m + 0xC) += *(float *)(v + 0x10);
        t = *(float *)(v + 0x14) - D_0015EE60 * 0.01f;
        *(float *)(v + 0x14) = t;
        *(float *)(v + 0x14) = (t < z) ? z : t;
        *(int *)(m + 4) &= 0xFFFFFF;
        *(int *)(m + 4) |= func_001FA898_r(*(float *)(v + 0x14) * 255.0f) << 24;
        if (*(float *)(v + 0x14) > z) {
            func_001F9BD8(m + 0x10, m + 0x10, v);
            func_001F9C30(v, v, D_0015EE60 * -0.0199999809f + 1.0f);
            return;
        }
    }
    func_L00_002688A8(m);
}
