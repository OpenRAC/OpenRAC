/* Class-17 camera region update (US level 01 0x318de0). While the hero is
   inside the region's volumes and the region owns the follow camera, it
   steers the camera toward the region's yaw (turn rate ramped over the
   counter at +0x2E), sets distance, pivot and look heights, script pitch,
   stick locks and spring constants from the region record; on leaving it
   marks the region done (+0x4A) and resets the counter. Mode 5 snaps the
   follow camera to the region's offset on its first tick.
   Adapted from ReRAC (crates/rc-game/src/follow_camera/level.rs region_update,
   region_snap; ISC License, Copyright (c) 2026 ReRAC contributors). */
extern char D_L00_00166D80_DE38[] __asm__("D_L00_00166D80");
extern float D_L00_00166ED8_DE38[] __asm__("D_L00_00166ED8");
extern float D_L00_00166FB4_DE38[] __asm__("D_L00_00166FB4");
extern unsigned char D_0013A5E0_DE38[] __asm__("D_0013A5E0");
extern int func_L00_002EDC98_DE38(void *) __asm__("func_L00_002EDC98");
extern int func_L00_002E9870_DE38(void *) __asm__("func_L00_002E9870");
extern int func_L00_002E9838_DE38(void *) __asm__("func_L00_002E9838");
extern float func_001FA850_DE38(float, float) __asm__("func_001FA850");
extern int func_001F9850_DE38(int) __asm__("func_001F9850");
extern float func_001FA888_DE38(int) __asm__("func_001FA888");
extern void func_L00_001FF4B0_DE38(void *, void *, float) __asm__("func_L00_001FF4B0");
extern void func_001F9C30_DE38(void *, void *, float) __asm__("func_001F9C30");
extern void func_001F9BD8_DE38(void *, void *, void *) __asm__("func_001F9BD8");
extern float func_001F9CB8_DE38(void *) __asm__("func_001F9CB8");
extern float func_001F9F90_DE38(float) __asm__("func_001F9F90");
extern float func_001F9FA8_DE38(float) __asm__("func_001F9FA8");
extern void func_L00_002E9B30_DE38(void) __asm__("func_L00_002E9B30");
extern void func_L00_002E9AD0_DE38(void) __asm__("func_L00_002E9AD0");
extern void func_L00_002E9AF8_DE38(void) __asm__("func_L00_002E9AF8");
extern void func_L00_002E9DC8_DE38(void *, float, float) __asm__("func_L00_002E9DC8");
extern void func_L00_002E9900_DE38(float, float, int) __asm__("func_L00_002E9900");
extern void func_L00_002E9940_DE38(float) __asm__("func_L00_002E9940");
extern void func_L00_002E9968_DE38(float, float) __asm__("func_L00_002E9968");
extern void func_L00_002E99A0_DE38(int, float, float) __asm__("func_L00_002E99A0");
extern void func_L00_002E99F0_DE38(float) __asm__("func_L00_002E99F0");
extern void func_L00_002E9A18_DE38(int) __asm__("func_L00_002E9A18");
extern void func_L00_002E9A40_DE38(float, float) __asm__("func_L00_002E9A40");

void func_L00_002EDE38(char *m) {
    float v[4] __attribute__((aligned(16)));
    char *p;
    char *g;
    char *d;
    char *ofs;
    char *look;
    char *pos;
    float steps;
    int stopped;
    int c;
    short k;

    p = *(char **)(D_L00_0015F050_EDC98 + *(short *)(m + 0x84) * 32 + 0x1C);
    k = *(short *)(p + 0x2C);
    if (k == 8 && *(int *)(*(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x230) == 0) {
        return;
    }
    if (k == 6 && *(int *)(*(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x230) != 0) {
        return;
    }
    if (func_L00_002EDC98_DE38(m) == 0) {
        goto leave;
    }
    if (func_L00_002E9870_DE38(m) <= 0) {
        if (*(short *)(p + 0x2E) != 0) {
            *(short *)(p + 0x4A) = 1;
        }
        *(short *)(p + 0x2E) = 0;
        return;
    }
    if (*(short *)(p + 0x36) != 0
        && 1.3962634f <= func_001FA850_DE38(*(float *)(D_L00_0015F050_EDC98 + *(short *)(m + 0x84) * 32 + 0x18),
                                            D_L00_00166ED8_DE38[0])) {
        goto leave;
    }
    if (*(short *)(p + 0x48) != 0 && *(short *)(p + 0x2C) == 5
        && (*(int *)(D_0013A5E0_DE38 + 0x2600) & 5) != 0) {
        goto leave;
    }
    if (*(short *)(p + 0x2E) == 0) {
        *(float *)(p + 0x40) = *(float *)(*(char **)(*(char **)(D_L00_00166D80_DE38 + 0x180) + 0x70) + 0x11C);
        *(float *)(p + 0x44) = *(float *)(*(char **)(*(char **)(D_L00_00166D80_DE38 + 0x180) + 0x70) + 0x120);
    }
    /* mode 5 with the counter at 0 (one word read of +0x2C/+0x2E) */
    if (*(int *)(p + 0x2C) == 5) {
        *(int *)(p + 0x0) = 0;
        g = D_L00_00166F00_EDC98[0];
        d = *(char **)(g + 0x70);
        ofs = d + 0x130;
        look = d + 0x40;
        pos = d + 0x20;
        func_L00_001FF4B0_DE38(ofs, ofs, *(float *)(p + 0x24));
        *(float *)(ofs + 0x2C) = *(float *)(p + 0x24);
        *(float *)(ofs + 0x30) = *(float *)(p + 0x28);
        *(float *)(look + 0xB0) = *(float *)(p + 0x30);
        qcopy(d + 0x140, ofs);
        func_001F9C30_DE38(v, D_0013E633 + 0x10AD, -*(float *)(ofs + 0x30));
        func_001F9BD8_DE38(d + 0x90, D_0013E633 + 0xE9D, v);
        func_001F9BD8_DE38(g + 0x30, d + 0x90, ofs);
        func_001F9C30_DE38(v, D_0013E633 + 0x10AD, -*(float *)(look + 0xB0));
        func_001F9BD8_DE38(d + 0x80, D_0013E633 + 0xE9D, v);
        qcopy(d + 0xD0, d + 0x80);
        qcopy(look, D_0013E633 + 0xE9D);
        qcopy(d + 0xA0, look);
        qcopy(d, g + 0x30);
        qcopy(d + 0x1F0, g + 0x30);
        *(float *)(d + 0x1D0 + 0x30) = *(float *)(ofs + 0x2C) - func_001F9CB8_DE38(ofs);
        *(int *)(pos + 0x14) = 0;
        *(float *)(pos + 0x4) = *(float *)(look + 0xB0);
        *(float *)(pos + 0xC) = *(float *)(pos + 0x14);
        *(float *)(pos + 0x8) = *(float *)(ofs + 0x30);
        *(float *)(pos + 0x18) = *(float *)(pos + 0x14);
        *(float *)(pos + 0x10) = *(float *)(pos + 0x14);
    }
    stopped = 0;
    steps = func_001FA888_DE38(func_001F9850_DE38(200));
    *(short *)(p + 0x2E) = *(unsigned short *)(p + 0x2E) + 1;
    if (*(short *)(p + 0x2C) == 10) {
        func_L00_002E9B30_DE38();
        *(int *)(*(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x230) = 0x14D;
    }
    if (*(short *)(p + 0x2C) == 11) {
        func_L00_002E9B30_DE38();
        *(int *)(*(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x230) = 2;
    }
    k = *(short *)(p + 0x2C);
    if (k == 2 || k == 4) {
        d = *(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x1A8;
        *(int *)(d + 0x10) |= 3;
        c = *(short *)(p + 0x2E);
        if (func_001F9850_DE38(200) < c) {
            stopped = 1;
            *(short *)(p + 0x2E) = func_001F9850_DE38(200);
        }
        func_L00_002E9AD0_DE38();
    } else if (k == 1 || k == 7 || k == 10 || k == 9) {
        if (*(float *)(D_0013A5E0_DE38 + 0x2460 + 0x100) != 0.0f
            || *(float *)(D_0013A5E0_DE38 + 0x2460 + 0x104) != 0.0f) {
            *(short *)(p + 0x2E) = func_001F9850_DE38(200);
        }
        c = *(short *)(p + 0x2E);
        if (!(c < func_001F9850_DE38(200))) {
            stopped = 1;
            if (0.0f < D_L00_00166FB4_DE38[0]) {
                *(short *)(p + 0x2E) = func_001F9850_DE38(200);
            }
        }
        c = *(short *)(p + 0x2E);
        if (!(c < func_001F9850_DE38(400))) {
            stopped = 0;
            *(short *)(p + 0x2E) = 1;
        }
    } else if (k == 3) {
        steps = func_001FA888_DE38(func_001F9850_DE38(1));
        c = *(short *)(p + 0x2E);
        if (func_001F9850_DE38(1) < c) {
            stopped = 1;
            *(short *)(p + 0x2E) = func_001F9850_DE38(1);
        }
    } else {
        c = *(short *)(p + 0x2E);
        if (func_001F9850_DE38(200) < c) {
            stopped = 1;
            *(short *)(p + 0x2E) = func_001F9850_DE38(200);
        }
    }
    v[0] = func_001F9F90_DE38(*(float *)(D_L00_0015F050_EDC98 + *(short *)(m + 0x84) * 32 + 0x18));
    v[1] = func_001F9FA8_DE38(*(float *)(D_L00_0015F050_EDC98 + *(short *)(m + 0x84) * 32 + 0x18));
    *(int *)&v[2] = 0;
    {
        float rate = (*(float *)(p + 0x0) * 0.017453292f) * ((float)*(short *)(p + 0x2E) / steps);
        k = *(short *)(p + 0x2C);
        if (!((k == 1 || k == 7 || k == 9) && stopped != 0)) {
            func_L00_002E9DC8_DE38(v, rate, *(float *)(p + 0x20) * 0.017453292f);
        }
    }
    if (*(float *)(p + 0x24) != 0.0f) {
        k = *(short *)(p + 0x2C);
        if (k == 6 || k == 8) {
            func_L00_002E9900_DE38(*(float *)(p + 0x24), 0.002f, 0);
            func_L00_002E9940_DE38(*(float *)(p + 0x24) + 1.36f);
        } else {
            func_L00_002E9900_DE38(*(float *)(p + 0x24), 0.003f, 0);
            func_L00_002E9AD0_DE38();
        }
    }
    if (*(float *)(p + 0x28) != 0.0f) {
        k = *(short *)(p + 0x2C);
        if (k == 6 || k == 8) {
            func_L00_002E9968_DE38(*(float *)(p + 0x28), 0.002f);
        } else {
            func_L00_002E9968_DE38(*(float *)(p + 0x28), 0.003f);
        }
    }
    if (*(float *)(p + 0x30) != 0.0f) {
        k = *(short *)(p + 0x2C);
        if (k == 6 || k == 8) {
            func_L00_002E99A0_DE38(0, *(float *)(p + 0x30), 0.002f);
        } else {
            func_L00_002E99A0_DE38(0, *(float *)(p + 0x30), 0.005f);
        }
    }
    if (*(float *)(p + 0x50) != 0.0f) {
        func_L00_002E99F0_DE38(*(float *)(p + 0x50) * 0.017453292f);
    }
    if (*(short *)(p + 0x54) != 0) {
        d = *(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x1A8;
        *(int *)(d + 0x10) |= 2;
    }
    if (*(short *)(p + 0x56) != 0) {
        d = *(char **)(D_L00_00166F00_EDC98[0] + 0x70) + 0x1A8;
        *(int *)(d + 0x10) |= 1;
    }
    if (*(short *)(p + 0x34) == 0) {
        func_L00_002E9A18_DE38(0);
    }
    if (*(float *)(p + 0x38) != 0.0f || *(float *)(p + 0x3C) != 0.0f) {
        c = *(short *)(p + 0x2E);
        if (c < func_001F9850_DE38(120)) {
            float t = (float)*(short *)(p + 0x2E) / func_001FA888_DE38(func_001F9850_DE38(120));
            float k0 = *(float *)(p + 0x40);
            float d0 = *(float *)(p + 0x44);
            func_L00_002E9A40_DE38(k0 + (*(float *)(p + 0x38) - k0) * t,
                                   d0 + (*(float *)(p + 0x3C) - d0) * t);
        } else {
            func_L00_002E9A40_DE38(*(float *)(p + 0x38), *(float *)(p + 0x3C));
        }
    }
    k = *(short *)(p + 0x2C);
    if (k == 1 || k == 7 || k == 10) {
        func_L00_002E9AD0_DE38();
        func_L00_002E9AF8_DE38();
        func_L00_002E9A40_DE38(0.01f, 0.2f);
    }
    return;

leave:
    if (*(short *)(p + 0x2E) != 0) {
        *(short *)(p + 0x4A) = 1;
    }
    *(short *)(p + 0x2E) = 0;
    func_L00_002E9838_DE38(m);
}
