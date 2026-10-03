/* NON_MATCHING func_L00_0026EFF0 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: BYTES 13/592 (97.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   PartType28Update: particle update; size p[1] grows/shrinks by D_0015EE60*0.007*p[3] (three branches), kills th
 *   Best candidate p6.c (BYTES 13/592, size equal): only the neg-branch float registers differ (retail D in $f1 an
 *   Tried locals n/d, (D*0.007) hoisted, multiply operand swap, p[3] = -p[3] inside the expression: same allocatio
 *   Second session (lb1/p02): p11 (D*0.007 hoisted into a local), p12 (d before n, n in the multiply), p14 (size r
 *   all keep the same 13-byte diff as p6 (flip branch: retail D in $f1 and p[3] in $f0, ours swapped). The Lombyte
 *   (n in the multiply with D inline) grow to 600 bytes (an extra reload). Stopped: same allocation every wording;
 */
extern int func_001F9938(void *);
extern void func_L00_002688A8(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9BD8(void *, void *, void *);
extern float D_0015EE60 MACRO_ADDR;

/* updates a fading, drifting particle: grows its size, fades its alpha and colour phase, kills it when small */
void func_L00_0026EFF0(char *m) {
    float *p = (float *)(m + 0x20);
    int a;
    float t;
    float sz;
    if (0.0f < p[3]) {
        p[1] += D_0015EE60 * 0.007f * p[3];
    } else if (p[1] > 0.03f) {
        p[1] += D_0015EE60 * 0.007f * p[3];
    } else {
        float d = D_0015EE60;
        p[1] += d * 0.007f * p[3] * 0.2f;
        *(float *)(m + 0xC) += d * 5460.0f;
    }
    if (p[1] <= 0.0244f || func_001F9938(m + 0xA)) {
        func_L00_002688A8(m);
        return;
    }
    sz = p[1];
    if (sz >= 0.12f) {
        float n = -p[3];
        float d = D_0015EE60;
        p[3] = n;
        p[1] = sz + d * 0.007f * p[3];
    }
    *(float *)(m + 0xC) += D_0015EE60 * 5460.0f;
    a = func_001FA898_r(p[1] * 255.0f);
    t = D_0015EE60;
    *(int *)(m + 4) = (*(int *)(m + 4) & 0xFFFFFF) | (a << 24);
    p[2] += t * 0.002f;
    if (p[2] > 1.0f) p[2] -= 1.0f;
    m[8] = (int)(p[2] * 255.0f);
    func_001F9BD8(m + 0x10, m + 0x10, p + 4);
}
