/* NON_MATCHING func_L06_002F4CF8 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: BYTES 58/524 (88.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tint pulse moby update: picks an ARGB base per state, scales rgb by sin-based factor (clamped to 255 via movn)
 *   p4 is the best (structure matches); left: clamp compiles to `slt 255,v; movz` instead of retail's `slti v,0x10
 *   D_L06_0015F6B0 read via $gp (retail lui+lw: wants int MACRO_ADDR alias, untried), and an s3/s4/s5 register rot
 */
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001FA8A8(int, int, float);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F49B0(void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern short D_L06_0015F6B0_s __asm__("D_L06_0015F6B0");

// Pulses a moby's tint with a sine, applies it, and refreshes its child mobys and draw callback.
void func_L06_002F4CF8(unsigned char *m)
{
    char *d;
    int c;
    int s;
    float x;
    float f;
    int r, g, b, a;
    int mx;
    int i;
    int t;
    int *p;
    int col;

    if (m[0x21] == 0xFF) return;
    d = *(char **)(m + 0x78);
    s = m[0x20];
    c = 0x3228AAAA;
    if (s == 3 || s == 6) {
        c = 0x3228AA28;
    } else if (s == 5 || s == 9) {
        c = 0x322828AA;
    } else if (s == 7) {
        c = 0x32AA2828;
    }
    x = func_001FA748(*(float *)(d + 0x28C), D_0015EE6C * 2.96705961227417f);
    *(float *)(d + 0x28C) = x;
    r = c & 0xFF;
    a = (unsigned)c >> 24;
    mx = 255;
    f = func_001F9FA8(x) * 0.5f + 1.0f;
    g = (c >> 8) & 0xFF;
    b = (c >> 16) & 0xFF;
    {
        float ff = (float)r;
        int v;
        r = 255;
        v = func_001FA898_r(ff * f);
        if (v <= 255) r = v;
    }
    {
        float ff = (float)g;
        int v;
        g = 255;
        v = func_001FA898_r(ff * f);
        if (v <= 255) g = v;
    }
    {
        float ff = (float)b;
        int v;
        b = 255;
        v = func_001FA898_r(ff * f);
        if (v <= 255) b = v;
    }
    col = (a << 24) | (b << 16) | (g << 8) | r;
    *(int *)(m + 0x90) = func_001FA8A8(*(int *)(m + 0x90), col, 0.1f);
    for (i = 0; i < 3; i++) {
        func_L00_00250800(m, i + 1, d + i * 16 + 0x140);
    }
    *(int *)(d + 0x290) = *(int *)(m + 0x90);
    t = *(int *)&D_L06_0015F6B0_s;
    if (t != 0) {
        p = *(int **)(d + 0x294);
        if (t != *p) {
            *p = t;
            func_001F49B0((void *)func_L06_002F4C00, m);
        }
    }
}
