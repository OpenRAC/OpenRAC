/* NON_MATCHING func_L00_00215738 -- src/overlays/shared/help_00214D60.c
 * Best so far: SIZE ours 848 / retail 852, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Head-look clamp: reads rotated vector v (func_001F9EE8), clamps components to limits chosen by state at b+0x20
 *   Left (p2.c 860 vs 852 / p5.c 836): retail keeps the hi part of D_L00_0017A780 in a copy ($v1=daddu of lui) and
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_0020A858(float a, float b);
extern float D_0015EE64 MACRO_ADDR;
extern char D_L00_0017A780[];
extern char D_0013E633[];

// Clamps and applies the camera tilt/pan offsets into the head-look tables.
void func_L00_00215738(void) {
    float v[4];
    char *b = D_0013E633 + 0xE1D;
    float lim, x, y, z, m;
    int k;
    if (*(short *)(b + 0x308) == 0) return;
    *(u128 *)v = 0;
    v[2] = -1.0f;
    func_001F9EE8(v, v, b + 0x40);
    func_L00_0020A858(D_0015EE64 * 0.013f, D_0015EE64 * 0.3f);
    k = *(int *)(b + 0x2084);
    lim = 0.4363323f;
    if (k == 0x3F) {
        lim = 0.2967060f;
    } else if (k == 0x70) {
        lim = 0.0872665f;
    } else if (k == 0x71 || k == 4) {
        lim = 0.1919862f;
    }
    if (*(int *)(D_0013E633 + 0xE1D + 0x2FC) != 0) {
        if (0.1570796f < lim) lim = 0.1570796f;
    }
    x = v[0] * 0.6981317f;
    if (lim < x) x = lim;
    else if (x < -lim) x = -lim;
    y = -v[1] * 0.6981317f;
    *(float *)(D_L00_0017A780 + 0x114) = x;
    *(float *)(D_L00_0017A780 + 0x274) = -x * 0.5f;
    if (lim < y) y = lim;
    else if (y < -lim) y = -lim;
    z = -v[0] * 1.2217305f;
    *(float *)(D_L00_0017A780 + 0x110) = y;
    *(float *)(D_L00_0017A780 + 0x270) = -y * 0.25f;
    if (0.9948376f < z) z = 0.9948376f;
    if (z < -0.1221730f) z = -0.1221730f;
    if (0.85f < v[2]) {
        if (z < 1.3962634f) z = 1.3962634f;
    } else if (0.5f < v[2]) {
        if (z < 1.2217305f) z = 1.2217305f;
    }
    m = v[1] * 1.2217305f;
    *(float *)(D_L00_0017A780 + 0x954) = z * 0.57f;
    *(float *)(D_L00_0017A780 + 0xAB4) = z * 0.5f;
    *(float *)(D_L00_0017A780 + 0xB64) = z * 0.35f;
    *(float *)(D_L00_0017A780 + 0xA04) = z * 0.5f;
    if (1.2217305f < m) m = 1.2217305f;
    else if (m < -1.2217305f) m = -1.2217305f;
    *(float *)(D_L00_0017A780 + 0xB60) = m;
    *(float *)(D_L00_0017A780 + 0xAB0) = m * 0.7f;
    *(float *)(D_L00_0017A780 + 0x950) = m * 0.7f;
    *(float *)(D_L00_0017A780 + 0xA00) = m * 0.7f;
}
