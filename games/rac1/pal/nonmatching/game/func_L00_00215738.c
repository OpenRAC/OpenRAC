/* Veldin level code: clamps the camera's angle limits and writes the per-mode tables at D_L00_0017A780
 * (retail func_L00_00215738, 0x354 bytes). Reads the camera block at D_0013E633 + 0xE1D: 0x308 (short) gates
 * the whole pass; 0x2084 (mode 0x3F, 0x70, 0x71 or 4) picks a limit; 0x2FC (int) tightens it to 0.157.
 * Builds a vector (0, 0, -1, 0) and transforms it with func_001F9EE8 into the block at +0x40. */
extern char D_L00_0017A780[];
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_0020A858(float, float);

void func_L00_00215738(void) {
    char *h;
    char *g;
    float vec[4];
    float f0, f1, f2, f3, f4, f5, f6;
    s32 mode;

    h = D_0013E633 + 0xE1D;
    if (*(short *)(h + 0x308) == 0)
        return;

    vec[0] = 0.0f;
    vec[1] = 0.0f;
    vec[2] = -1.0f;
    vec[3] = 0.0f;
    func_001F9EE8(vec, vec, h + 0x40);

    f0 = D_0015EE64;
    func_L00_0020A858(f0 * 0.013f, f0 * 0.3f);

    mode = *(s32 *)(h + 0x2084);
    f5 = 0.43633232f;
    if (mode == 0x3F) {
        f5 = 0.29670596f;
    } else if (mode == 0x70) {
        f5 = 0.08726646f;
    } else if (mode == 0x71 || mode == 4) {
        f5 = 0.19198622f;
    }
    if (*(s32 *)(h + 0x2FC) != 0) {
        f0 = 0.15707964f;
        if (f0 < f5)
            f5 = f0;
    }

    f1 = vec[0];
    f0 = 0.6981317f;
    f4 = f1 * f0;
    if (f5 < f4) {
        f4 = f5;
    } else {
        f0 = -f5;
        if (f4 < f0)
            f4 = f0;
    }

    f0 = vec[1];
    f1 = -f4;
    f3 = 0.6981317f;
    g = D_L00_0017A780;
    f0 = -f0;
    f2 = 0.5f;
    f1 = f1 * f2;
    f3 = f0 * f3;
    *(float *)(g + 0x114) = f4;
    *(float *)(g + 0x274) = f1;
    if (f5 < f3) {
        f3 = f5;
    } else {
        f0 = -f5;
        if (f3 < f0)
            f3 = f0;
    }

    f1 = vec[0];
    f2 = -f3;
    f6 = 1.2217305f;
    f1 = -f1;
    f0 = 0.25f;
    f5 = 0.99483764f;
    f2 = f2 * f0;
    *(float *)(g + 0x110) = f3;
    f4 = f1 * f6;
    *(float *)(g + 0x270) = f2;
    if (f5 < f4)
        f4 = f5;

    f0 = -0.12217305f;
    if (f4 < f0)
        f4 = f0;

    f1 = vec[2];
    f0 = 0.85f;
    if (f0 < f1) {
        f0 = 1.3962634f;
        if (f4 < f0)
            f4 = f0;
    } else if (0.5f < f1) {
        if (f4 < f6)
            f4 = f6;
    }
    f0 = vec[1];

    f5 = 1.2217305f;
    f3 = f0 * f5;
    f1 = 0.5f;
    f0 = 0.35f;
    f2 = 0.57f * f4;
    f1 = f4 * f1;
    f0 = f4 * f0;
    *(float *)(g + 0x954) = f2;
    *(float *)(g + 0xAB4) = f1;
    *(float *)(g + 0xB64) = f0;
    *(float *)(g + 0xA04) = f1;
    if (f5 < f3) {
        f3 = f5;
    } else {
        f0 = -1.2217305f;
        if (f3 < f0)
            f3 = f0;
    }

    f0 = 0.7f;
    *(float *)(g + 0xB60) = f3;
    f0 = f3 * f0;
    *(float *)(g + 0xAB0) = f0;
    *(float *)(g + 0x950) = f0;
    *(float *)(g + 0xA00) = f0;
}
