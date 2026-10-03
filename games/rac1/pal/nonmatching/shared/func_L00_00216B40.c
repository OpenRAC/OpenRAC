/* NON_MATCHING func_L00_00216B40 -- src/overlays/shared/help_00214D60.c
 * Best so far: BYTES 41/512 (92.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds three view vectors (z = 1.0, 0.8, 0.8), adjusts by player state (g+0x20B3), calls func_L00_00216880, th
 *   Left: float register assignment (ours 1.0 lands in f22 and the 9.6 product in f23; retail the reverse), lq/lbu
 *   Unblock: a source order that makes the 1.0f constant allocate after the three products; budget spent.
 */
typedef int U128 __attribute__((mode(TI)));
extern float D_0015EE64 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern void func_001F9C30(void *, void *, float);
extern int func_001F9850(int);

/* builds the three view vectors and applies the camera offset to the moby */
void func_L00_00216B40(void) {
    unsigned char *g = (unsigned char *)D_0013E633 + 0xE1D;
    unsigned char *h;
    M_c m;
    V_c a, b, c;
    float f20, f21, f22;
    U128 z = 0;
    f21 = D_0015EE64 * 0.05f;
    f20 = D_0015EE64 * 0.3f;
    f22 = D_0015EE6C * 9.6f;
    *(U128 *)&a = z; *(U128 *)&b = z; *(U128 *)&c = z;
    a.f[2] = 1.0f;
    b.f[2] = 0.8f;
    c.f[2] = 0.8f;
    func_001F9EE8(&b, &b, g);
    *(U128 *)&a = *(U128 *)(g + 0x270);
    if (g[0x20B3] == 2) func_001F9C30(&a, g + 0x290, -1.0f);
    if (g[0x20B3] == 1) {
        if (*(short *)(g + 0x30E) < func_001F9850(10) || *(float *)(g + 0x2DC) < 1.0f)
            *(U128 *)&a = *(U128 *)(g + 0x270);
    }
    h = (unsigned char *)D_0013E633 + 0xE1D;
    if (*(short *)(h + 0x30E) != 0 && *(short *)(h + 0x1F8) != 0) {
        f20 = D_0015EE64 * 0.001f;
        f21 = f20;
    }
    func_L00_00216880(&a, &m, 0, f21, f20, f22);
    g = (unsigned char *)D_0013E633 + 0xE1D;
    if (g[0x20B3] == 1 && *(short *)(g + 0x30E) != 0) {
        func_001F9EE8(&c, &c, &m);
        func_001F9BF0(g + 0x80, g + 0x80, &c);
        func_001F9BD8(g + 0x80, g + 0x80, &b);
    }
}
