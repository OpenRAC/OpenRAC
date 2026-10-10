/* NON_MATCHING func_L14_002D7AF8 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 2468 / retail 2476, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Level 14 moby update: eases the keyframe position (sample loop over the tab at 0x60), then steps the state byt
 *   Differences left: retail copies n into a register before the loop (daddu $3,$6) and loads the loop base early;
 *   Unblock: a sibling with the same zero-float store pattern, or a matched form of the 0xC0 vector (retail's floa
 */
extern char *D_L14_0015F7EC_p __asm__("D_L14_0015F7EC") MACRO_ADDR;
extern int func_L00_0025EFC0(void *, void *, float *, int *, float *, int, float, float, float);
extern int func_L00_0025E860_2AF918(int *tab, float *out, int *a, float *b, int c, float d) __asm__("func_L00_0025E860");
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern float func_L00_001FF860(float, float);
extern int func_001F9908(void *);
extern int func_001F9850(int);
extern void func_0020DAF8(char *, int, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001FA4A0(void *, void *);
extern void func_001FA540(void *, void *, void *);
extern void func_002150B0(void *, void *);
extern float func_001FA888(int);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA790(float, float);
extern int func_L00_002E33D8(void *, int, int);
extern void func_L00_001FFED8(void *, int, float);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001FA5C8(void *, void *, void *, float);
extern int func_L00_002E3640(void *, void *, void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern char *func_L00_002757E8(void *, void *, int, void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_L00_0023F0D0(float *, float, float, float, float, float);
extern void func_L00_0023F1D0(int);
extern short D_L14_00161B08;
extern short D_L14_00161B0C;
extern short D_L14_00161B10;
extern short D_L14_00161B14;
extern short D_L14_00161B18;
extern short D_L14_00161B1C;
extern short D_L14_00161B20;
extern short D_L14_00161B24;
extern short D_L14_00161B28;
extern float D_L14_0015F660[] MACRO_ADDR;
extern char D_L14_00180AC0[];
extern char D_0013E633[];


typedef int u128_2D7AF8 __attribute__((mode(TI)));

// Level 14 moby update: eases its keyframe position and runs the small state machine in byte 0xBC.
void func_L14_002D7AF8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    char *tab = *(char **)(D_L14_0015F7EC_p + (*(int *)(data + 0x60) << 5) + 0x10);
    char fr[0x1C0];
    int i = *(int *)(data + 0x64);
    int n = *(short *)(data + 0x8E);
    float f20;
    float f21;
    float f22;
    float f23;
    unsigned char *bc = (unsigned char *)(moby + 0xBC);

    if (i >= n) {
        float sum = 0.0f;
        float r;
        if (i > n) {
            int cnt = i - n;
            char *p = tab + (n << 4) + 0x1C;
            do {
                sum += *(float *)p;
                p += 16;
            } while (--cnt != 0);
        }
        sum += *(float *)(data + 0x68);
        r = sum / *(float *)(data + 0x78);
        if (r > 1.0f) {
            r = 1.0f;
        }
        *(float *)(data + 0x74) = *(float *)&D_L14_00161B08 + (-3.0f - *(float *)&D_L14_00161B08) * r;
    }

    *(int *)(data + 0x68) = 0;
    f22 = *(float *)(data + 0x68);
    *(int *)(data + 0x64) = 0;
    f21 = 0.01f;
    func_L00_0025EFC0(tab, D_0013E633 + 0xE9D, (float *)(fr + 0x00), (int *)(data + 0x64), (float *)(data + 0x68), 0, 20.0f, 5.0f, f22);
    f20 = 0.2f;
    func_L00_0025E860_2AF918((int *)tab, (float *)(fr + 0x00), (int *)(data + 0x64), (float *)(data + 0x68), 0, -*(float *)(data + 0x74));
    func_L00_0025C918((float *)(moby + 0x10), (float *)(data + 0x90), *(float *)(fr + 0x00), f21, f20, f22);
    func_L00_0025C918((float *)(moby + 0x14), (float *)(data + 0x94), *(float *)(fr + 0x04), f21, f20, f22);
    func_L00_0025C918((float *)(moby + 0x18), (float *)(data + 0x98), *(float *)(fr + 0x08), f21, f20, f22);

    {
        char *p2 = tab + (i << 4);
        char *p3 = tab + ((i + 5) << 4);
        *(float *)(moby + 0x48) = func_L00_001FF860(*(float *)(p3 + 0x10) - *(float *)(p2 + 0x10),
                                                    *(float *)(p3 + 0x14) - *(float *)(p2 + 0x14));
    }
    if (*bc == 5) {
        if (func_001F9908(data + 0x1C8)) {
            *bc = 1;
        }
    }
    if (*bc == 1) {
        *(int *)(data + 0x1C0) = func_001F9850(*(int *)&D_L14_00161B0C);
        *(int *)(data + 0x1C4) = 0;
        *bc = 2;
        func_0020DAF8(moby, 4, fr + 0x10);
        func_0020DAF8(moby, 5, fr + 0x50);
        func_L00_001FF4B0(fr + 0x50, fr + 0x50, 1.0f);
        func_L00_001FF4B0(fr + 0x60, fr + 0x60, 1.0f);
        func_L00_001FF4B0(fr + 0x70, fr + 0x70, 1.0f);
        func_001FA4A0(fr + 0x90, fr + 0x50);
        *(float *)(fr + 0xC0) = 0.0f;
        *(float *)(fr + 0xC4) = 0.0f;
        *(float *)(fr + 0xC8) = 0.0f;
        *(float *)(fr + 0xCC) = 1.0f;
        func_001FA540(fr + 0xD0, fr + 0x90, fr + 0x10);
        func_002150B0(data + 0x1D0, fr + 0xD0);
    }

    f23 = *(float *)(moby + 0x48);
    if (*bc >= 2 && *bc <= 4) {
        func_0020DAF8(moby, 0, data + 0xA0);
        f22 = 1.0f;
        {
            float a = func_001FA888(*(int *)(data + 0x1C0));
            int r = func_001F9850(*(int *)&D_L14_00161B0C);
            f20 = a / func_001FA888(r);
        }
        *(int *)(fr + 0x1B0) = *(int *)(data + 0x64);
        *(float *)(fr + 0x1B4) = *(float *)(data + 0x68);
        func_L00_0025E860_2AF918((int *)tab, (float *)(fr + 0x10), (int *)(fr + 0x1B0), (float *)(fr + 0x1B4), 0,
                                 *(float *)&D_L14_00161B10 + (f22 - *(float *)&D_L14_00161B10) * f20);
        qcopy(data + 0x1E0, fr + 0x10);
        func_001F9BF0(fr + 0x20, fr + 0x10, moby + 0x10);
        f23 = func_L00_001FF860(*(float *)(fr + 0x20), *(float *)(fr + 0x24));
        f21 = func_001FA790(f23, *(float *)(moby + 0x48));
        if (f21 > 1.2217305f) {
            f21 = 1.2217305f;
        } else if (f21 < -1.2217305f) {
            f21 = -1.2217305f;
        }
        *(int *)(data + 0x1C4) = *(int *)(data + 0x1C4) + 1;
        if (func_001F9850(30) < *(int *)(data + 0x1C4)) {
            if (*bc == 4) {
                *bc = 0;
            } else {
                func_L00_002E33D8(moby, 3, 1);
                *bc = 3;
            }
            *(int *)(data + 0x1C4) = func_001F9850(30);
        }
        if (*bc == 2 || *bc == 4) {
            f20 = func_001FA888(*(int *)(data + 0x1C4));
            {
                int r2 = func_001F9850(30);
                f22 = f20 / func_001FA888(r2);
            }
            if (*bc == 4) {
                f22 = 1.0f - f22;
            }
            f21 = (f21 - 0.0f) * f22 + 0.0f;
        }
        func_L00_001FFED8(data + 0x150, 2, f21);
        f20 = 1.0f;
        f21 = 0.0f;
        func_L00_00250800(moby, 4, fr + 0x30);
        *(u128_2D7AF8 *)(fr + 0x40) = 0;
        *(float *)(fr + 0x48) = f20;
        *(float *)(fr + 0x4C) = f20;
        func_L00_001FF4B0(fr + 0x60, fr + 0x20, f20);
        func_001F9CA0(fr + 0x70, fr + 0x60, fr + 0x40);
        func_L00_001FF4B0(fr + 0x70, fr + 0x70, f20);
        func_001F9CA0(fr + 0x50, fr + 0x70, fr + 0x60);
        *(float *)(fr + 0x88) = f21;
        *(float *)(fr + 0x84) = f21;
        *(float *)(fr + 0x80) = f21;
        *(float *)(fr + 0x8C) = f20;
        func_0020DAF8(moby, 5, fr + 0x90);
        func_L00_001FF4B0(fr + 0x90, fr + 0x90, f20);
        func_L00_001FF4B0(fr + 0xA0, fr + 0xA0, f20);
        func_L00_001FF4B0(fr + 0xB0, fr + 0xB0, f20);
        func_001FA4A0(fr + 0xD0, fr + 0x90);
        *(float *)(fr + 0x100) = f21;
        *(float *)(fr + 0x108) = f21;
        *(float *)(fr + 0x104) = f21;
        *(float *)(fr + 0x10C) = f20;
        func_001FA540(fr + 0x110, fr + 0xD0, fr + 0x50);
        *(u128_2D7AF8 *)(fr + 0x150) = 0;
        *(float *)(fr + 0x15C) = f20;
        *(float *)(fr + 0x154) = 512.0f / *(float *)(moby + 0x2C);
        func_002150B0(fr + 0x160, fr + 0x110);
        if (*bc == 2 || *bc == 4) {
            func_001FA5C8(data + 0x190, data + 0x1D0, fr + 0x160, f22);
        } else {
            qcopy(data + 0x190, fr + 0x160);
        }
        f20 = 1.0f;
        *(unsigned char *)(data + 0x183) = 1;
        *(float *)(data + 0x18C) = f20;
        *(u128_2D7AF8 *)(fr + 0x170) = 0;
        *(float *)(fr + 0x170) = f20;
        *(float *)(fr + 0x174) = f20;
        *(float *)(fr + 0x178) = f20;
        *(float *)(fr + 0x17C) = f20;
        qcopy(data + 0x1A0, fr + 0x170);
        qcopy(data + 0x1B0, fr + 0x150);
        if (func_001F9908(data + 0x1C0)) {
            *bc = 4;
            *(int *)(data + 0x1C4) = 0;
        }

        if (*bc == 3) {
            if (func_L00_002E3640(moby, data + 0xA0, data + 0x1E0) == 0) {
                func_L00_002E33D8(moby, 3, 1);
            }
            *(int *)(fr + 0x194) = 0x10001;
            *(int *)(fr + 0x190) = (int)moby;
            *(int *)(fr + 0x1A0) = 1;
            *(float *)(fr + 0x19C) = f20;
            *(float *)(fr + 0x180) = func_001F9F90(*(float *)(moby + 0x48));
            *(float *)(fr + 0x184) = func_001F9FA8(*(float *)(moby + 0x48));
            *(float *)(fr + 0x188) = f20;
            *(float *)(fr + 0x18C) = 5627.925f;
            *(unsigned char *)(fr + 0x199) = 1;
            *(short *)(fr + 0x19A) = *(unsigned short *)(moby + 0xA6);
            *(unsigned char *)(fr + 0x198) = 0;
            func_L00_001EFFF0(data + 0xD0, data + 0x1E0, 9, moby, fr + 0x180);
        } else if (*bc == 2 || *bc == 4) {
            func_001F9C30(fr + 0x180, data + 0xA0, *(float *)&D_L14_00161B14);
            func_001F9BD8(fr + 0x180, fr + 0x180, data + 0xD0);
            {
                char *res = func_L00_002757E8(fr + 0x180, D_L14_0015F660, 0x7F, moby);
                if (res != 0) {
                    float a0 = (*(float *)&D_L14_00161B18 - 1000.0f) * f22 + 1000.0f;
                    float a12 = f22 * 111.0f + 16.0f;
                    *(float *)(res + 0x0C) = a0;
                    *(short *)(res + 0x34) = func_001FA898_r(a12);
                    *(short *)(res + 0x36) = 1;
                    {
                        int r2 = func_001F9850(4);
                        *(short *)(res + 0x0A) = r2;
                        *(float *)(res + 0x30) = f20 / func_001FA888((short)r2);
                    }
                    *(int *)(res + 0x38) = 0x7F7F7F;
                }
            }
            qcopy(fr + 0x190, data + 0xD0);
            {
                float f1v = *(float *)(fr + 0x198) - 0.5f;
                *(float *)(fr + 0x198) = f1v;
            }
            f20 = (*(float *)&D_L14_00161B1C - 0.1f) * f22 + 0.1f;
            if (*(int *)(data + 0xF0) == -1) {
                *(int *)(data + 0xF0) = func_L00_0023F0D0((float *)(fr + 0x190), f20, 0.0f, *(float *)&D_L14_00161B20,
                                                          *(float *)&D_L14_00161B24, *(float *)&D_L14_00161B28);
            }
            if (*(int *)(data + 0xF0) >= 0) {
                char *q = D_L14_00180AC0 + (*(int *)(data + 0xF0) << 5);
                qcopy(q + 0x10, fr + 0x190);
                *(float *)(q + 0x00) = *(float *)&D_L14_00161B20;
                *(float *)(q + 0x04) = *(float *)&D_L14_00161B24;
                *(float *)(q + 0x08) = *(float *)&D_L14_00161B28;
                *(float *)(q + 0x1C) = f20;
            }
        } else {
            if (*(int *)(data + 0xF0) != -1) {
                func_L00_0023F1D0(*(int *)(data + 0xF0));
                *(int *)(data + 0xF0) = -1;
            }
        }
    } else {
        if (*(int *)(data + 0xF0) != -1) {
            func_L00_0023F1D0(*(int *)(data + 0xF0));
            *(int *)(data + 0xF0) = -1;
        }
    }

    func_001F9BF0(fr + 0x10, D_0013E633 + 0xE9D, moby + 0x10);
    {
        float tf = func_L00_001FF860(*(float *)(fr + 0x10), *(float *)(fr + 0x14));
        tf = func_001FA790(tf, f23);
        if (tf > 1.2217305f) {
            tf = 1.2217305f;
        } else if (tf < -1.2217305f) {
            tf = -1.2217305f;
        }
        func_L00_001FFED8(data + 0x110, 2, tf);
    }
}
