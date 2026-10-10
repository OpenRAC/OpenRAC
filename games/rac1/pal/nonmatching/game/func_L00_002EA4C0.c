/* Follow-camera rows (US level 01 0x3141e8): builds the camera moby's
   forward / left / up rows (m+0x00, +0x10, +0x20). Mode 11 (the fall)
   aims the forward straight at the hero. Otherwise the forward is the
   horizontal look direction, the pivot and look heights are smoothed
   (springs at d+0x28 / d+0x24), and the forward is pitched toward the
   smoothed look point (minus an elevation bias, d+0x34) within +-70
   degrees. A blend timer (d+0x20) mixes in the saved forward (m+0x40).
   func_L00_001EB6A8 is used for its float result (retail returns
   FastAddRots(a, *p) in $f0 by a tail call), as the matched callers do.
   Adapted from ReRAC (crates/rc-game/src/follow_camera.rs FollowCamera::rows;
   ISC License, Copyright (c) 2026 ReRAC contributors). */
extern char D_0013E633_A4C0[] __asm__("D_0013E633");
extern char D_L00_00166F10_A4C0[] __asm__("D_L00_00166F10");
extern char D_L00_00166F30_A4C0[] __asm__("D_L00_00166F30");
extern int D_L00_00161DF4_A4C0 __asm__("D_L00_00161DF4");
extern void func_001F9BF0_A4C0(void *, void *, void *) __asm__("func_001F9BF0");
extern float func_001F9C78_A4C0(void *, void *) __asm__("func_001F9C78");
extern void func_001F9C30_A4C0(void *, void *, float) __asm__("func_001F9C30");
extern float func_001F9CB8_A4C0(void *) __asm__("func_001F9CB8");
extern void func_001F9CA0_A4C0(void *, void *, void *) __asm__("func_001F9CA0");
extern void func_001F9C08_A4C0(void *, void *, void *, float) __asm__("func_001F9C08");
extern void func_L00_001FF4B0_A4C0(void *, void *, float) __asm__("func_L00_001FF4B0");
extern int func_001F9938_A4C0(void *) __asm__("func_001F9938");
extern int func_001F9850_A4C0(int) __asm__("func_001F9850");
extern float func_001FA888_A4C0(int) __asm__("func_001FA888");
extern float func_001F9FC0_A4C0(float) __asm__("func_001F9FC0");
extern float func_L00_001FF860_A4C0(float, float) __asm__("func_L00_001FF860");
extern float func_001FA790_A4C0(float, float) __asm__("func_001FA790");
extern float func_001EC120_A4C0(float, float, float *, float, float, float) __asm__("func_001EC120");
extern float func_L00_001EB6A8_A4C0(void *, float, float, float, float, float) __asm__("func_L00_001EB6A8");
extern void func_002156E0_A4C0(void *, void *, void *, float) __asm__("func_002156E0");

void func_L00_002EA4C0(int arg) {
    float s[4] __attribute__((aligned(16)));
    float lh[4] __attribute__((aligned(16)));
    float ph[4] __attribute__((aligned(16)));
    float hv[4] __attribute__((aligned(16)));
    float w[4] __attribute__((aligned(16)));
    char *m = (char *)arg;
    char *g = D_L00_00166F10_A4C0;
    char *d = *(char **)(m + 0x70);
    char *d40 = d + 0x40;
    char *t = d + 0x20;
    char *d2;
    char *ofs;
    char *pos;
    float f;
    float len;
    float dh;
    float pit;
    float q;
    float tb;

    if (*(unsigned char *)(d40 + 0xC4) == 11) {
        func_001F9BF0_A4C0(m, D_0013E633_A4C0 + 0xE9D, m + 0x30);
        func_L00_001FF4B0_A4C0(m, m, 1.0f);
        func_001F9938_A4C0(t);
    } else {
        func_001F9C30_A4C0(s, g + 0x20, func_001F9C78_A4C0(d + 0x80, g + 0x20));
        func_001F9BF0_A4C0(lh, d + 0x80, s);
        d2 = *(char **)(m + 0x70);
        ofs = d2 + 0x130;
        pos = m + 0x30;
        func_001F9C30_A4C0(s, g + 0x20, func_001F9C78_A4C0(pos, g + 0x20));
        func_001F9BF0_A4C0(ph, pos, s);
        func_001F9BF0_A4C0(hv, lh, ph);
        len = func_001F9CB8_A4C0(hv);
        if (0.05f <= len) {
            func_001F9C30_A4C0(m, hv, 1.0f / len);
        }
        func_001F9CA0_A4C0(m + 0x10, m, D_L00_00166F30_A4C0);
        func_L00_001FF4B0_A4C0(m + 0x10, m + 0x10, 1.0f);
        *(float *)(t + 0x8) = func_001EC120_A4C0(*(float *)(t + 0x8), *(float *)(ofs + 0x30),
                                                 (float *)(d + 0x30), 0.004f, 0.2f, 0.0f);
        *(float *)(t + 0x4) = func_001EC120_A4C0(*(float *)(t + 0x4), *(float *)(d40 + 0xB0),
                                                 (float *)(d + 0x2C), 0.004f, 0.2f, 0.0f);
        dh = *(float *)(t + 0x8) - *(float *)(t + 0x4);
        func_001F9C30_A4C0(s, D_L00_00166F30_A4C0, -func_001F9C78_A4C0(d2 + 0x140, D_L00_00166F30_A4C0));
        func_001F9BF0_A4C0(s, s, hv);
        func_001F9BF0_A4C0(w, pos, s);
        func_001F9C30_A4C0(s, D_L00_00166F30_A4C0, -dh);
        func_001F9BF0_A4C0(w, w, s);
        func_001F9BF0_A4C0(hv, w, pos);
        len = func_001F9CB8_A4C0(hv);
        if (len != 0.0f) {
            f = func_001F9C78_A4C0(m, hv);
            pit = 1.5707964f - func_001F9FC0_A4C0(f / len);
            func_001F9CA0_A4C0(m + 0x20, m + 0x10, m);
            if (func_001F9C78_A4C0(m + 0x20, hv) < 0.0f) {
                pit = -pit;
            }
            f = func_001F9C78_A4C0(ofs, D_L00_00166F30_A4C0);
            q = func_L00_001FF860_A4C0(func_001F9CB8_A4C0(ofs), f) / 0.6981317f;
            tb = 0.0f;
            if (q < -0.1f) {
                q = -q;
                if (0.5f < q) {
                    q = 1.0f - q;
                }
                tb = (q + q) * 0.2617994f + tb;
            }
            *(float *)(t + 0x14) = func_L00_001EB6A8_A4C0(t + 0x18, *(float *)(t + 0x14), tb, 0.005f, 0.2f, 0.0f);
            pit = func_001FA790_A4C0(pit, *(float *)(t + 0x14));
            if (1.2217305f < pit) {
                pit = 1.2217305f;
            } else if (pit < -1.2217305f) {
                pit = -1.2217305f;
            }
            func_002156E0_A4C0(m, m, m + 0x10, pit);
            func_L00_001FF4B0_A4C0(m, m, 1.0f);
        }
    }
    if (*(short *)t != 0) {
        func_001F9938_A4C0(t);
        f = func_001FA888_A4C0(*(short *)t);
        f = f / func_001FA888_A4C0(func_001F9850_A4C0(D_L00_00161DF4_A4C0));
        func_001F9C08_A4C0(m, m, m + 0x40, f);
        func_L00_001FF4B0_A4C0(m, m, 1.0f);
    }
    func_001F9CA0_A4C0(m + 0x10, m, D_L00_00166F30_A4C0);
    func_L00_001FF4B0_A4C0(m + 0x10, m + 0x10, 1.0f);
    func_001F9CA0_A4C0(m + 0x20, m + 0x10, m);
}
