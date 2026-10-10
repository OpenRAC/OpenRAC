/* NON_MATCHING func_L00_00215A90 -- src/overlays/shared/help_00214D60.c
 * Best so far: SIZE ours 2024 / retail 2084, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   HeroLean: per-state joint modifier update (states 2, 8/0x81, 4) with clamps; 9 runs, best p6.c at 2024 bytes a
 *   Stopped: size still 60 bytes short and the block layout (case 2 call order, 0x81 block B) differs in several p
 */
extern void func_L00_0020A858(float, float);
extern float func_001F9B88(float);
extern float func_L00_001FF860(float, float);
extern float func_001FA790(float, float);
extern float func_001F9FA8(float);
extern float func_001F9F90(float);
extern float func_001FA748(float, float);
extern float D_0015EE6C MACRO_ADDR;
extern char D_L00_0017A780[];

// Procedural hero lean: sets the joint modifiers in the hero state from the current lean angle.
void func_L00_00215A90(void) {
    char *h;
    float t;
    float r;
    float d;
    float a;
    float nt;
    char *g = D_0013E633 + 0xE1D;
    int st = *(int *)(g + 0x2084);

    if (st == 2) {
        func_L00_0020A858(0.04f, 0.2f);
        func_L00_0020A858(0.04f, 0.2f);
        func_L00_0020A858(0.04f, 0.2f);
        func_L00_0020A858(0.02f, 0.2f);
        if (*(int *)(D_0013E633 + 0xE1D + 0x2088) == 1) {
            t = *(float *)(D_0013E633 + 0xE1D + 0x188);
            if (t > 1.4f) {
                t = 1.4f;
            } else if (t < -1.4f) {
                t = -1.4f;
            }
            nt = -t;
            d = *(float *)(D_0013E633 + 0xE1D + 0x164) / (D_0015EE6C * 5.7f);
            a = nt * 0.14f;
            h = D_L00_0017A780;
            *(float *)(h + 0x60) = a;
            if (d > 1.0f) d = 1.0f;
            if (d < 0.0f) d = 0.0f;
            *(float *)(h + 0x60) = a * d;
            *(float *)(h + 0x118) = t * 0.16f;
            *(float *)(h + 0x1C0) = t * -0.12f;
            *(float *)(h + 0x278) = t * 0.65f;
            r = func_001F9B88(t);
            *(float *)(h + 0x274) = r * -0.15f;
            r = func_001F9B88(t);
            *(float *)(h + 0x270) = r * 0.16f;
            *(float *)(h + 0x958) = nt * 0.35f;
            *(float *)(h + 0xB68) = nt * 0.3f;
            *(float *)(h + 0xA08) = nt * 0.3f;
            *(float *)(h + 0xAB8) = nt * 0.3f;
            r = func_001F9B88(t);
            *(float *)(h + 0x954) = r * 0.35f;
            r = func_001F9B88(t);
            *(float *)(h + 0xA04) = r * 0.2f;
        } else {
            float f20;
            h = D_L00_0017A780;
            f20 = *(float *)(D_0013E633 + 0xE1D + 0x188) * 0.5f;
            if (f20 > 0.8f) {
                f20 = 0.8f;
            } else if (f20 < -0.8f) {
                f20 = -0.8f;
            }
            r = func_001F9B88(f20);
            f20 = f20 * r * 2.0f;
            *(float *)(h + 0x118) = f20;
            *(float *)(h + 0x278) = f20;
            r = func_001F9B88(f20);
            *(float *)(h + 0x958) = -f20 * 0.35f;
            *(float *)(h + 0xB68) = -f20 * 0.3f;
            *(float *)(h + 0x274) = r * 0.25f;
            *(float *)(h + 0xA08) = -f20 * 0.3f;
            *(float *)(h + 0xAB8) = -f20 * 0.3f;
        }
    } else if (st == 8 || st == 0x81) {
        float f20;
        float f22;
        func_L00_0020A858(0.008f, 0.08f);
        func_L00_0020A858(0.015f, 0.08f);
        func_L00_0020A858(0.015f, 0.08f);
        func_L00_0020A858(0.008f, 0.1f);
        f20 = *(float *)(D_0013E633 + 0xE1D + 0x188) * 1.6f;
        if (f20 > 1.25f) {
            f20 = 1.25f;
        } else if (f20 < -1.25f) {
            f20 = -1.25f;
        }
        if (*(int *)(D_0013E633 + 0xE1D + 0x2084) == 0x81) {
            f20 = *(float *)(D_0013E633 + 0xE1D + 0x188) * 0.8f;
            if (f20 > 1.1f) {
                f20 = 1.1f;
            } else if (f20 < -1.1f) {
                f20 = -1.1f;
            }
        }
        f22 = -f20;
        h = D_L00_0017A780;
        d = f22 * 0.52f;
        *(float *)(h + 0x60) = d;
        if (d > 0.28f) {
            *(float *)(h + 0x60) = 0.28f;
        } else if (d < -0.28f) {
            *(float *)(h + 0x60) = -0.28f;
        }
        d = f20 * 1.2f;
        a = f20 * 0.58f;
        r = f20 * -0.28f;
        *(float *)(h + 0x278) = d;
        *(float *)(h + 0x118) = a;
        if (d > 0.6f) {
            *(float *)(h + 0x278) = 0.6f;
        } else if (d < -0.6f) {
            *(float *)(h + 0x278) = -0.6f;
        }
        *(float *)(h + 0x1C0) = r;
        r = func_001F9B88(f20);
        d = r * 0.2f;
        *(float *)(h + 0x274) = d;
        if (d > 0.2f) {
            *(float *)(h + 0x274) = 0.2f;
        } else if (d < -0.2f) {
            *(float *)(h + 0x274) = -0.2f;
        }
        r = func_001F9B88(f20);
        *(float *)(h + 0x270) = r * 0.16f;
        if (*(int *)(D_0013E633 + 0xE1D + 0x2084) == 0x81) {
            float q;
            float p;
            float s;
            s = *(float *)(D_0013E633 + 0xE1D + 0x164);
            if (D_0015EE6C * 0.5f < s) {
                q = func_L00_001FF860(*(float *)(D_0013E633 + 0xE1D + 0x100), *(float *)(D_0013E633 + 0xE1D + 0x104));
                q = func_001FA790(q, *(float *)(D_0013E633 + 0xE1D + 0x98));
                s = *(float *)(D_0013E633 + 0xE1D + 0x164) * 3.0f;
                r = func_001F9FA8(q);
                *(float *)(h + 0x60) = -r * s;
                p = func_001F9F90(q);
                *(float *)(h + 0x64) = p * s;
                if (*(float *)(h + 0x60) > 0.4363323f) {
                    *(float *)(h + 0x60) = 0.4363323f;
                } else if (*(float *)(h + 0x60) < -0.4363323f) {
                    *(float *)(h + 0x60) = -0.4363323f;
                }
                if (*(float *)(h + 0x64) > 0.4363323f) {
                    *(float *)(h + 0x64) = 0.4363323f;
                } else if (*(float *)(h + 0x64) < -0.4363323f) {
                    *(float *)(h + 0x64) = -0.4363323f;
                }
            }
            *(float *)(h + 0x64) = func_001FA748(*(float *)(h + 0x64), -0.12217305f);
        }
        *(float *)(h + 0x958) = f22 * 0.35f;
        *(float *)(h + 0xB68) = f22 * 0.3f;
        *(float *)(h + 0xA08) = f22 * 0.3f;
        *(float *)(h + 0xAB8) = f22 * 0.3f;
    } else if (st == 4) {
        float f12;
        func_L00_0020A858(0.04f, 0.2f);
        func_L00_0020A858(0.04f, 0.2f);
        func_L00_0020A858(0.04f, 0.2f);
        func_L00_0020A858(0.02f, 0.2f);
        h = D_L00_0017A780;
        f12 = *(float *)(D_0013E633 + 0xE1D + 0x188);
        if (f12 > 1.4f) {
            f12 = 1.4f;
        } else if (f12 < -1.4f) {
            f12 = -1.4f;
        }
        *(float *)(h + 0x958) = -f12 * 0.35f;
        *(float *)(h + 0x118) = f12 * 0.29f;
        *(float *)(h + 0x1C0) = f12 * -0.27f;
        *(float *)(h + 0xB68) = -f12 * 0.3f;
        *(float *)(h + 0x278) = f12 * 0.87f;
        *(float *)(h + 0xA08) = -f12 * 0.3f;
        *(float *)(h + 0xAB8) = -f12 * 0.3f;
    }
}
