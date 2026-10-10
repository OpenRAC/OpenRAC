/* CamType0HeroStateTweaks (US level 01 0x3111d8): per-tick follow-camera
   tweaks from the hero's state. Ledge hang (state 0xD): turn toward the
   ledge yaw + pi; water / glide / sinking-floor groups: look and pivot
   heights; Clank (body 1) and giant Clank (body 2): distance, heights and
   the sphere chain's scale; a weapon held up: leash off and the horizontal
   spring; then the look-hint callback, the scripted focus moby's turn, the
   15-unit focus scan (+0x230 = 1), state 0x81 with L2/R2 and the yaw
   stabiliser.
   Adapted from ReRAC (crates/rc-game/src/follow_camera.rs update_type0, the
   0x3111d8 tweaks; ISC License, Copyright (c) 2026 ReRAC contributors). */
extern char D_0013E633_74B0[] __asm__("D_0013E633");
extern unsigned char D_0013A5E0_74B0[] __asm__("D_0013A5E0");
extern char D_L00_00167100_74B0[] __asm__("D_L00_00167100");
extern char D_L00_00166F10_74B0[] __asm__("D_L00_00166F10");
extern char *D_L00_00178000_74B0[] __asm__("D_L00_00178000");
extern float D_L00_00161E54_74B0 __asm__("D_L00_00161E54");
extern float D_L00_00161E58_74B0 __asm__("D_L00_00161E58");
extern int D_L00_00161E64_74B0 __asm__("D_L00_00161E64");
extern float D_L00_00161E68_74B0 __asm__("D_L00_00161E68");
extern float D_L00_00161E6C_74B0 __asm__("D_L00_00161E6C");
extern float D_L00_00161E70_74B0 __asm__("D_L00_00161E70");
extern float D_L00_00161E74_74B0 __asm__("D_L00_00161E74");
extern float D_L00_00161E78_74B0 __asm__("D_L00_00161E78");
extern float D_L00_0015F040_74B0 __asm__("D_L00_0015F040");
extern float func_001FA748_74B0(float, float) __asm__("func_001FA748");
extern float func_001F9F90_74B0(float) __asm__("func_001F9F90");
extern float func_001F9FA8_74B0(float) __asm__("func_001F9FA8");
extern float func_001F9B88_74B0(float) __asm__("func_001F9B88");
extern float func_001FA850_74B0(float, float) __asm__("func_001FA850");
extern int func_001F9850_74B0(int) __asm__("func_001F9850");
extern float func_001FA888_74B0(int) __asm__("func_001FA888");
extern int func_001F9938_74B0(void *) __asm__("func_001F9938");
extern int func_L00_001F2BE8_74B0(float, void *, int, void *, void *) __asm__("func_L00_001F2BE8");
extern char *func_L00_0025D390_74B0(char *) __asm__("func_L00_0025D390");
extern void func_L00_002E9DC8_74B0(void *, float, float) __asm__("func_L00_002E9DC8");
extern void func_L00_002E9E20_74B0(void *, float, float) __asm__("func_L00_002E9E20");
extern void func_L00_002E9900_74B0(float, float, int) __asm__("func_L00_002E9900");
extern void func_L00_002E9968_74B0(float, float) __asm__("func_L00_002E9968");
extern void func_L00_002E99A0_74B0(int, float, float) __asm__("func_L00_002E99A0");
extern void func_L00_002E9A18_74B0(int) __asm__("func_L00_002E9A18");
extern void func_L00_002E9A40_74B0(float, float) __asm__("func_L00_002E9A40");
extern void func_L00_002E9AD0_74B0(void) __asm__("func_L00_002E9AD0");
extern void func_L00_002E9AF8_74B0(void) __asm__("func_L00_002E9AF8");

void func_L00_002E74B0(void *arg) {
    char *m = arg;
    char *h = D_0013E633_74B0 + 0xE1D;
    char *own = *(char **)(m + 0x70) + 0x220;
    char *g;
    char *d;
    char *o;
    void (*cb)(void);
    float v[4] __attribute__((aligned(16)));
    float a;
    float sum;
    float f;
    int glide = 0;
    int n;
    int i;
    char **list;
    char *t;
    short c;

    if (*(int *)(h + 0x2284) == 0xD && *(int *)(own + 0x10) != 0x14D) {
        a = func_001FA748_74B0(*(float *)(h + 0x4E4), 3.1415927f);
        v[0] = func_001F9F90_74B0(a);
        v[1] = func_001F9FA8_74B0(a);
        *(int *)&v[2] = 0;
        func_L00_002E9DC8_74B0(v, 0.20943952f, v[2]);
        func_L00_002E9AF8_74B0();
    }
    if (*(int *)(h + 0x208C) == 0x11) {
        *(float *)(*(char **)(m + 0x70) + 0xF0) = 0.25f;
        func_L00_002E9968_74B0(0.5f, 0.003f);
    }
    if (*(int *)(h + 0x208C) == 5) {
        glide = 1;
        d = *(char **)(m + 0x70) + 0x40;
        if (*(unsigned char *)(h + 0x20A4) == 1) {
            *(float *)(d + 0xB0) = 0.25f;
            func_L00_002E9968_74B0(1.5f, 0.003f);
        } else {
            *(float *)(d + 0xB0) = 0.25f;
            func_L00_002E9968_74B0(2.5f, 0.003f);
        }
    }
    if (*(int *)(h + 0x208C) == 0x10) {
        *(float *)(*(char **)(m + 0x70) + 0xF0) = 0.25f;
    }
    if (*(unsigned char *)(h + 0x20A4) == 1) {
        if (*(int *)(own + 0x0) == 0) {
            d = *(char **)(m + 0x70) + 0x40;
            func_L00_002E9900_74B0(3.0f, 0.003f, 1);
            if (glide == 0) {
                *(float *)(d + 0xB0) = 1.0f;
                func_L00_002E9968_74B0(1.0f, 0.003f);
            }
        }
        *(float *)(*(char **)(m + 0x70) + 0x20C) = 0.6f;
    }
    if (*(unsigned char *)(h + 0x20A4) == 2) {
        if (*(int *)(own + 0x10) < 2) {
            func_L00_002E99A0_74B0(0, D_L00_00161E68_74B0, 0.005f);
            func_L00_002E9900_74B0(D_L00_00161E6C_74B0, 0.003f, 1);
            func_L00_002E9968_74B0(D_L00_00161E70_74B0, 0.003f);
        }
        func_L00_002E9AD0_74B0();
        d = *(char **)(m + 0x70) + 0x1D0;
        *(float *)(d + 0x3C) = D_L00_00161E78_74B0;
        *(float *)(d + 0x44) = D_L00_00161E74_74B0;
    }
    if (*(unsigned char *)(h + 0x20AA) != 0 && *(int *)(h + 0x208C) != 0xF) {
        func_L00_002E9A18_74B0(0);
        func_L00_002E9AF8_74B0();
        func_L00_002E9A40_74B0(0.04f, 0.2f);
    }

    /* the look-hint callback */
    cb = *(void (**)(void))(D_L00_00167100_74B0 + 0x10);
    if (cb != 0) {
        cb();
    }

    /* the scripted focus moby (D_L00_00166D80 + 0x25C), its counter at +0x260 */
    g = D_L00_00167100_74B0 - 0x380;
    o = *(char **)(g + 0x25C);
    if (o != 0 && *(int *)(own + 0x10) < 2) {
        if (*(signed char *)(o + 0x20) < 0) {
            *(char **)(g + 0x25C) = 0;
        } else {
            if (0.3f <= func_001F9B88_74B0(*(float *)(D_0013A5E0_74B0 + 0x2460 + 0x100))) {
                *(int *)(g + 0x260) = 0;
            } else if (0.3f <= func_001F9B88_74B0(*(float *)(D_0013A5E0_74B0 + 0x2460 + 0x104))) {
                *(int *)(g + 0x260) = 0;
            }
            *(int *)(g + 0x260) = *(int *)(g + 0x260) + 1;
            if (func_001F9850_74B0(400) < *(int *)(g + 0x260)) {
                *(int *)(g + 0x260) = func_001F9850_74B0(400);
            }
            f = func_001FA888_74B0(*(int *)(g + 0x260));
            f = f / func_001FA888_74B0(func_001F9850_74B0(400));
            func_L00_002E9E20_74B0(*(char **)(g + 0x25C) + 0x10, f * 0.20943952f, 0.0f);
        }
    }
    *(int *)(own + 0x10) = 0;

    /* the focus scan: a moby within 15 of the hero whose target record's byte +0xD is set */
    n = func_L00_001F2BE8_74B0(15.0f, D_0013E633_74B0 + 0xE9D, 1,
                               *(void **)(D_0013E633_74B0 + 0xE9D + 0x2000), 0);
    if (n > 0) {
        list = D_L00_00178000_74B0;
        for (i = 0; i < n; i++) {
            t = func_L00_0025D390_74B0(*list);
            if (t != 0 && (float)*(unsigned char *)(t + 0xD) != 0.0f) {
                *(int *)(own + 0x10) = 1;
                break;
            }
            list++;
        }
    }

    if (*(int *)(h + 0x2084) == 0x81 && (*(int *)(D_0013A5E0_74B0 + 0x2600) & 3) != 0) {
        func_L00_002E9A18_74B0(0);
        func_L00_002E9AF8_74B0();
        func_L00_002E9A40_74B0(0.04f, 0.2f);
        d = *(char **)(m + 0x70);
        *(short *)(d + 0x14) = 0;
        *(short *)(d + 0x16) = 0;
        *(short *)(d + 0x12) = 0;
        return;
    }
    if (D_L00_00161E64_74B0 == 0) {
        return;
    }

    /* the yaw stabiliser: mean change of the yaw history */
    d = *(char **)(m + 0x70);
    sum = 0.0f;
    for (i = 0; i < 4; i++) {
        f = func_001FA850_74B0(*(float *)(D_L00_00166F10_74B0 + 0xAC + i * 4),
                               *(float *)(D_L00_00166F10_74B0 + 0xB0 + i * 4));
        v[i] = f;
        sum += f;
    }
    sum = sum * 0.25f;
    f = sum * 5.0f;
    if (1.0f < f) {
        f = 1.0f;
    }
    f = 1.0f - f;
    if (f < D_L00_00161E54_74B0) {
        *(short *)(d + 0x12) = func_001F9850_74B0(32);
    }
    if (*(short *)(d + 0x12) != 0) {
        if (func_001F9938_74B0(d + 0x12) != 0) {
            *(short *)(d + 0x14) = func_001F9850_74B0(45);
        }
        func_L00_002E9AF8_74B0();
        c = *(unsigned short *)(d + 0x16) + 1;
        *(short *)(d + 0x16) = c;
        if (func_001F9850_74B0(30) < c) {
            *(short *)(d + 0x16) = func_001F9850_74B0(30);
        }
        f = func_001FA888_74B0(*(short *)(d + 0x16));
        f = f / func_001FA888_74B0(func_001F9850_74B0(30));
        func_L00_002E9A40_74B0((D_L00_00161E58_74B0 - 0.015f) * f + 0.015f, 0.2f);
        *(int *)&D_L00_0015F040_74B0 = 0;
    } else if (*(short *)(d + 0x14) != 0) {
        func_001F9938_74B0(d + 0x14);
        f = func_001FA888_74B0(*(short *)(d + 0x14));
        f = f / func_001FA888_74B0(func_001F9850_74B0(45));
        f = f * -0.75f + 0.75f;
        D_L00_0015F040_74B0 = f;
        *(short *)(d + 0x16) = 0;
    } else {
        *(short *)(d + 0x16) = 0;
        D_L00_0015F040_74B0 = 0.75f;
    }
}
