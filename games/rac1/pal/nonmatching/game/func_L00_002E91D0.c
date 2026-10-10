/* Follow-camera avoidance (US level 01 0x312ef8). Lists the mobys in a
   sphere halfway between the pivot and the camera: a few classes turn the
   offset 1 degree away from them, the others have their collision switched
   off (+0x94 = 0) until the end. Then the sphere chain and the end-sphere
   (blocked flag), the distance reduction and its timed recovery, the offset
   rescaled to distance - reduction, and the camera line from the pivot: a
   crate in the way gets a hit, 30 blocked ticks reset the camera. The
   switched-off mobys get their collision pointer back at the end.
   Adapted from ReRAC (crates/rc-game/src/follow_camera.rs Camera::avoidance,
   nudge, and tick.rs's crate hit; ISC License, Copyright (c) 2026 ReRAC
   contributors). */
extern char D_0013E633_91D0[] __asm__("D_0013E633");
extern unsigned char D_0013A5E0_91D0[] __asm__("D_0013A5E0");
extern char D_L00_00166F10_91D0[] __asm__("D_L00_00166F10");
extern char *D_L00_00178000_91D0[] __asm__("D_L00_00178000");
extern char *D_L00_001E7910_91D0[] __asm__("D_L00_001E7910");
extern char D_L00_00173F60_91D0[] __asm__("D_L00_00173F60");
extern char D_L00_00166EC0_91D0[] __asm__("D_L00_00166EC0");
extern int D_0015EE84_91D0[] __asm__("D_0015EE84");
extern int D_L00_0015F058_91D0 __asm__("D_L00_0015F058");
extern float D_L00_00161DA0_91D0 __asm__("D_L00_00161DA0");
extern void func_001F9C08_91D0(void *, void *, void *, float) __asm__("func_001F9C08");
extern void func_001F9BF0_91D0(void *, void *, void *) __asm__("func_001F9BF0");
extern void func_001F9BD8_91D0(void *, void *, void *) __asm__("func_001F9BD8");
extern float func_001F9C78_91D0(void *, void *) __asm__("func_001F9C78");
extern void func_001F9C30_91D0(void *, void *, float) __asm__("func_001F9C30");
extern void func_001F9CA0_91D0(void *, void *, void *) __asm__("func_001F9CA0");
extern float func_001F9FC0_91D0(float) __asm__("func_001F9FC0");
extern float func_001F9B88_91D0(float) __asm__("func_001F9B88");
extern void func_L00_001FF4B0_91D0(void *, void *, float) __asm__("func_L00_001FF4B0");
extern void func_002156E0_91D0(void *, void *, void *, float) __asm__("func_002156E0");
extern int func_L00_001F2BE8_91D0(float, void *, int, void *, void *) __asm__("func_L00_001F2BE8");
extern int func_L00_0025A868_91D0(void *) __asm__("func_L00_0025A868");
extern int func_L00_002E8EE0_91D0(void *) __asm__("func_L00_002E8EE0");
extern long func_L00_002E89D0_91D0(void) __asm__("func_L00_002E89D0");
extern int func_L00_002E87C8_91D0(void *) __asm__("func_L00_002E87C8");
extern int func_001F9850_91D0(int) __asm__("func_001F9850");
extern float func_001FA888_91D0(int) __asm__("func_001FA888");
extern int func_001F9938_91D0(void *) __asm__("func_001F9938");
extern float func_00214220_91D0(float, float, float) __asm__("func_00214220");
extern int func_L00_001EFFF0_91D0(void *, void *, int, void *, void *) __asm__("func_L00_001EFFF0");
extern int func_L00_002E5740_91D0(void *) __asm__("func_L00_002E5740");
extern int func_L00_0025F410_91D0(void *) __asm__("func_L00_0025F410");
extern void func_L00_0025A8C0_91D0(void *, void *, int, void *, float) __asm__("func_L00_0025A8C0");
extern void func_L00_0025AAC0_91D0(void *, void *) __asm__("func_L00_0025AAC0");
extern void func_L00_001ED600_91D0(void) __asm__("func_L00_001ED600");

void func_L00_002E91D0(int arg) {
    float c[4] __attribute__((aligned(16)));
    float a[4] __attribute__((aligned(16)));
    float b[4] __attribute__((aligned(16)));
    float e[4] __attribute__((aligned(16)));
    float n[4] __attribute__((aligned(16)));
    char hit[0x30] __attribute__((aligned(16)));
    char *m = (char *)arg;
    char *g = D_L00_00166F10_91D0;
    char *h = D_0013E633_91D0 + 0xE1D;
    char *d = *(char **)(m + 0x70);
    char *ofs = d + 0x130;
    char *rd = d + 0x1D0;
    char *pivot;
    char **list;
    char **off;
    char *o;
    char *mob;
    float r;
    float f;
    int blocked = 0;
    int count = 0;
    int all = 0;
    int k;
    short cls;

    r = (*(float *)(ofs + 0x2C) - *(float *)(rd + 0x30)) + 1.5f;
    if (D_0015EE84_91D0[0] == 0xF) {
        all = *(unsigned char *)(h + 0x20A4) == 2;
    }
    pivot = d + 0x90;
    func_001F9C08_91D0(c, pivot, g - 0x50, 0.5f);
    k = func_L00_001F2BE8_91D0(r * 0.5f, c, 1, *(void **)(h + 0x2080), 0);
    if (k > 0) {
        list = D_L00_00178000_91D0;
        off = D_L00_001E7910_91D0;
        do {
            o = *list;
            if (*(int *)(o + 0x94) != 0 && (func_L00_0025A868_91D0(o) == 0 || all != 0)) {
                cls = *(short *)(o + 0xA6);
                if (cls == 0x72 || cls == 0xB || cls == 0x2C || cls == 0x9A || cls == 0xFF
                    || cls == 0x392 || cls == 0x452 || cls == 0x353) {
                    /* turn the offset away from it */
                    func_001F9BF0_91D0(a, o + 0x10, pivot);
                    func_001F9C30_91D0(b, g + 0x30, func_001F9C78_91D0(g + 0x30, a));
                    func_001F9BF0_91D0(e, a, b);
                    func_L00_001FF4B0_91D0(e, e, 1.0f);
                    f = func_001F9C78_91D0(e, ofs);
                    f = func_001F9FC0_91D0(f / (*(float *)(ofs + 0x2C) - *(float *)(rd + 0x30)));
                    if (func_001F9B88_91D0(1.5707964f - f) < 0.43633232f) {
                        func_001F9CA0_91D0(n, ofs, g + 0x30);
                        func_L00_001FF4B0_91D0(n, n, 1.0f);
                        if (0.0f < func_001F9C78_91D0(n, a)) {
                            func_002156E0_91D0(ofs, ofs, g + 0x20, -0.017453292f);
                        } else {
                            func_002156E0_91D0(ofs, ofs, g + 0x20, 0.017453292f);
                        }
                    }
                } else {
                    *off++ = o;
                    count++;
                    *(int *)(o + 0x94) = 0;
                }
            }
            k--;
            list++;
        } while (k != 0);
    }

    /* sphere chain (not in state 0x7F, nor on level 13 in state 0x7B) and end sphere */
    if (*(int *)(h + 0x2084) != 0x7F
        && !(D_0015EE84_91D0[0] == 0xD && *(int *)(h + 0x2084) == 0x7B)) {
        blocked = func_L00_002E8EE0_91D0(m) != 0;
    }
    if (func_L00_002E89D0_91D0() != 0) {
        blocked = 1;
    }
    if (func_L00_002E87C8_91D0(m) != 0) {
        blocked = 1;
    }

    /* distance reduction: at most distance - D_L00_00161DA0, then a timed recovery */
    f = *(float *)(ofs + 0x2C);
    if (f - *(float *)(rd + 0x30) < D_L00_00161DA0_91D0) {
        *(float *)(rd + 0x30) = f - D_L00_00161DA0_91D0;
        *(short *)(rd + 0x34) = func_001F9850_91D0(2000);
    }
    if (*(short *)(rd + 0x3A) != 0) {
        goto recover;
    }
    if (blocked != 0) {
        goto hold;
    }
    if (*(short *)(rd + 0x34) < func_001F9850_91D0(2000) && *(short *)(rd + 0x34) > 0) {
        goto recover;
    }
    if (*(float *)(D_0013A5E0_91D0 + 0x2460 + 0x100) < 0.3f
        && *(float *)(D_0013A5E0_91D0 + 0x2460 + 0x104) < 0.3f) {
        goto recover;
    }
hold:
    k = *(short *)(rd + 0x34);
    if (func_001F9850_91D0(2000) < k) {
        func_001F9938_91D0(rd + 0x34);
    } else {
        *(short *)(rd + 0x34) = func_001F9850_91D0(2000);
    }
    goto rescale;
recover:
    if (func_001F9938_91D0(rd + 0x34) != 0) {
        *(short *)(rd + 0x3A) = 0;
    }
    if (*(short *)(rd + 0x3A) != 0) {
        if (func_001F9938_91D0(rd + 0x34) != 0) {
            *(short *)(rd + 0x3A) = 0;
        }
    }
    f = func_001FA888_91D0(*(short *)(rd + 0x34));
    f = f / func_001FA888_91D0(func_001F9850_91D0(2000));
    *(float *)(rd + 0x30) = func_00214220_91D0(0.0f, *(float *)(rd + 0x30), f);
rescale:
    func_L00_001FF4B0_91D0(ofs, ofs, *(float *)(ofs + 0x2C) - *(float *)(rd + 0x30));
    func_001F9BD8_91D0(a, ofs, pivot);

    /* the camera line from the pivot to the camera */
    if (*(int *)(h + 0x2084) == 0x7F
        || (D_0015EE84_91D0[0] == 0xD && *(int *)(h + 0x2084) == 0x7B)
        || func_L00_001EFFF0_91D0(pivot, a, D_L00_0015F058_91D0, *(void **)(h + 0x2080), 0) == 0
        || func_L00_002E5740_91D0(D_L00_00173F60_91D0) != 0) {
        *(int *)(rd + 0x4C) = 0;
    } else {
        mob = *(char **)(D_L00_00173F60_91D0 - 0x20 + 0x18);
        if (mob != 0 && func_L00_0025F410_91D0(mob) != 0) {
            /* a crate in the way: hit it, horizontally away from the camera */
            func_001F9BF0_91D0(b, *(char **)(D_L00_00173F60_91D0 - 0x20 + 0x18) + 0x10, D_L00_00166EC0_91D0);
            *(int *)&b[2] = 0;
            func_L00_001FF4B0_91D0(b, b, 1.0f);
            b[3] = 5627.9248f;
            func_L00_0025A8C0_91D0(hit, *(void **)(h + 0x2080), 0x800000, b, 20.0f);
            hit[0x19] = 3;
            hit[0x18] = 3;
            *(short *)(hit + 0x1A) = *(unsigned short *)(*(char **)(h + 0x2080) + 0xA6);
            func_L00_0025AAC0_91D0(*(char **)(D_L00_00173F60_91D0 - 0x20 + 0x18), hit);
        }
        *(int *)(rd + 0x4C) = *(int *)(rd + 0x4C) + 1;
        if (!(*(int *)(rd + 0x4C) < func_001F9850_91D0(30))) {
            func_L00_001ED600_91D0();
        }
    }

    /* the switched-off mobys get their collision back */
    if (count > 0) {
        off = D_L00_001E7910_91D0;
        do {
            o = *off++;
            count--;
            *(int *)(o + 0x94) = *(int *)(*(char **)(o + 0x24) + 0x10);
        } while (count != 0);
    }
}
