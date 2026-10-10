/* NON_MATCHING func_L00_002E74B0 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 1696 / retail 1716, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Camera tweak by hero state: sways the aim (FA748/F9F90/F9FA8 into E9DC8), zoom and drift timers from the level
 *   Differences left: the tail (0.75 constant and D_L00_0015F040 store via lui in retail, ours gp-relative), the 4
 *   Stopped at run 8 of 10 (candidate far from EXACT, a structural tail difference left).
 */
extern char D_0013E633[];
extern unsigned char D_0013A5E0[];
extern char D_0013F450[];
extern short D_L00_00161E54;
extern short D_L00_00161E58;
extern short D_L00_00161E64;
extern short D_L00_00161E68;
extern short D_L00_00161E6C;
extern short D_L00_00161E70;
extern short D_L00_00161E74;
extern short D_L00_00161E78;
extern float D_L00_0015F040 MACRO_ADDR;
extern char D_L00_00167100[];
extern char D_L00_00166D80[];
extern char D_L00_00166F10[];
extern int *D_L00_00178000[];
extern int func_001F9850(int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001FA748(float, float);
extern float func_001FA888(int);
extern float func_001F9B88(float);
extern int func_001F9938(void *);
extern float func_001FA850(float, float);
extern unsigned char *func_L00_0025D390(int);
extern int func_001F2BE8(float, void *, int, void *, void *);
extern void func_L00_002E9DC8(void *, float, float);
extern void func_L00_002E9AF8(void);
extern void func_L00_002E9968(float, float);
extern void func_L00_002E9900(float, float, int);
extern void func_L00_002E99A0(int, float, float);
extern void func_L00_002E9AD0(void);
extern void func_L00_002E9A18(int);
extern void func_L00_002E9A40(float, float);
extern void func_L00_002E9E20(int, float, float);

/* camera tweaks by hero state: sway the aim, nudge the zoom, and drift the lookat from the level's camera blocks */
void func_L00_002E74B0(char *m) {
    char *h = *(char **)(m + 0x70) + 0x220;
    float a[4];
    float t;
    float f;
    float acc;
    int flag = 0;
    int n;
    int i;
    char *w;

    if (*(int *)(D_0013F450 + 0x2284) == 0xD) {
        if (*(int *)(h + 0x10) != 0x14D) {
            t = func_001FA748(*(float *)(D_0013F450 + 0x4E4), 3.1415927f);
            a[0] = func_001F9F90(t);
            a[1] = func_001F9FA8(t);
            *(int *)&a[2] = 0;
            func_L00_002E9DC8(a, 0.20943952f, a[2]);
            func_L00_002E9AF8();
        }
    }

    if (*(int *)(D_0013F450 + 0x208C) == 0x11) {
        *(float *)(*(char **)(m + 0x70) + 0xF0) = 0.25f;
        func_L00_002E9968(0.5f, 0.003f);
    }

    if (*(int *)(D_0013F450 + 0x208C) == 5) {
        char *q = *(char **)(m + 0x70) + 0x40;
        flag = 1;
        if (*(unsigned char *)(D_0013F450 + 0x20A4) == 1) {
            *(float *)(q + 0xB0) = 0.25f;
            func_L00_002E9968(1.5f, 0.003f);
        } else {
            *(float *)(q + 0xB0) = 0.25f;
            func_L00_002E9968(2.5f, 0.003f);
        }
    }

    if (*(int *)(D_0013F450 + 0x208C) == 0x10) {
        *(float *)(*(char **)(m + 0x70) + 0xF0) = 0.25f;
    }
    if (*(unsigned char *)(D_0013F450 + 0x20A4) == 1) {
        if (*(int *)h == 0) {
            char *q = *(char **)(m + 0x70) + 0x40;
            func_L00_002E9900(3.0f, 0.003f, 1);
            if (flag == 0) {
                *(float *)(q + 0xB0) = 1.0f;
                func_L00_002E9968(1.0f, 0.003f);
            }
        }
        *(float *)(*(char **)(m + 0x70) + 0x20C) = 0.6f;
    }

    if (*(unsigned char *)(D_0013F450 + 0x20A4) == 2) {
        if (*(int *)(h + 0x10) < 2) {
            func_L00_002E99A0(0, *(float *)&D_L00_00161E68, 0.005f);
            func_L00_002E9900(*(float *)&D_L00_00161E6C, 0.003f, 1);
            func_L00_002E9968(*(float *)&D_L00_00161E70, 0.003f);
        }
        func_L00_002E9AD0();
        *(float *)(*(char **)(m + 0x70) + 0x20C) = *(float *)&D_L00_00161E78;
        *(float *)(*(char **)(m + 0x70) + 0x214) = *(float *)&D_L00_00161E74;
    }

    if (*(unsigned char *)(D_0013F450 + 0x20AA) != 0) {
        if (*(int *)(D_0013F450 + 0x208C) != 0xF) {
            func_L00_002E9A18(0);
            func_L00_002E9AF8();
            func_L00_002E9A40(0.04f, 0.2f);
        }
    }
    {
        void (*fp)(void) = *(void (**)(void))(D_L00_00167100 + 0x10);
        char *g2 = D_L00_00167100 - 0x380;
        if (fp != 0) {
            fp();
        }
        if (*(int *)(g2 + 0x25C) != 0 && *(int *)(h + 0x10) < 2) {
            char *p = *(char **)(g2 + 0x25C);
            if (*(signed char *)(p + 0x20) < 0) {
                *(int *)(g2 + 0x25C) = 0;
            } else {
                char *q = D_0013A5E0 + 0x2460;
                if (0.3f <= func_001F9B88(*(float *)(q + 0x100)) || 0.3f <= func_001F9B88(*(float *)(q + 0x104))) {
                    *(int *)(g2 + 0x260) = 0;
                }
                {
                    char *r = D_L00_00166D80;
                    *(int *)(r + 0x260) = *(int *)(r + 0x260) + 1;
                    i = func_001F9850(0x190);
                    if (i < *(int *)(r + 0x260)) {
                        *(int *)(r + 0x260) = func_001F9850(0x190);
                    }
                    t = func_001FA888(*(int *)(r + 0x260));
                    t = t / func_001FA888(func_001F9850(0x190));
                    func_L00_002E9E20(*(int *)(r + 0x25C) + 0x10, t * 0.20943952f, 0.0f);
                }
            }
        }
        *(int *)(h + 0x10) = 0;
    }

    {
        char *p2 = D_0013E633 + 0xE9D;
        n = func_001F2BE8(15.0f, p2, 1, *(void **)(p2 + 0x2000), 0);
    }
    if (n > 0) {
        for (i = 0; i < n; i++) {
            unsigned char *p = func_L00_0025D390((int)D_L00_00178000[i]);
            if (p == 0) {
                continue;
            }
            if ((float)p[0xD] == 0.0f) {
                continue;
            }
            *(int *)(h + 0x10) = 1;
            break;
        }
    }

    if (*(int *)(D_0013F450 + 0x2084) == 0x81) {
        if ((*(int *)(D_0013A5E0 + 0x2600) & 3) != 0) {
            func_L00_002E9A18(0);
            func_L00_002E9AF8();
            func_L00_002E9A40(0.04f, 0.2f);
            w = *(char **)(m + 0x70);
            *(short *)(w + 0x14) = 0;
            *(short *)(w + 0x16) = 0;
            *(short *)(w + 0x12) = 0;
            return;
        }
    }
    if (*(int *)&D_L00_00161E64 == 0) {
        return;
    }
    w = *(char **)(m + 0x70);
    {
        float *v = (float *)(D_L00_00166F10 + 0xB0);
        acc = 0.0f;
        for (i = 0; i < 4; i++) {
            f = func_001FA850(v[-1], v[0]);
            a[i] = f;
            acc += f;
            v++;
        }
    }
    t = acc * 0.25f;
    {
        float u = t * 5.0f;
        if (1.0f < u) {
            u = 1.0f;
        }
        t = 1.0f - u;
    }
    if (t < *(float *)&D_L00_00161E54) {
        *(short *)(w + 0x12) = func_001F9850(0x20);
    }
    if (*(short *)(w + 0x12) != 0) {
        if (func_001F9938(w + 0x12)) {
            *(short *)(w + 0x14) = func_001F9850(0x2D);
        }
        func_L00_002E9AF8();
        {
            int v16 = *(unsigned short *)(w + 0x16) + 1;
            *(short *)(w + 0x16) = v16;
            if (func_001F9850(0x1E) < (short)v16) {
                *(short *)(w + 0x16) = func_001F9850(0x1E);
            }
        }
        t = func_001FA888(*(short *)(w + 0x16));
        t = t / func_001FA888(func_001F9850(0x1E));
        f = (*(float *)&D_L00_00161E58 - 0.015f) * t + 0.015f;
        func_L00_002E9A40(f, 0.2f);
        D_L00_0015F040 = 0.0f;
        return;
    } else {
        if (*(short *)(w + 0x14) == 0) {
            *(short *)(w + 0x16) = 0;
            D_L00_0015F040 = 0.75f;
            return;
        }
        func_001F9938(w + 0x14);
        t = func_001FA888(*(short *)(w + 0x14));
        t = t / func_001FA888(func_001F9850(0x2D));
        t = t * -0.75f + 0.75f;
        D_L00_0015F040 = t;
        *(short *)(w + 0x16) = 0;
        return;
    }
}
