/* NON_MATCHING func_L00_002EDE38 -- src/overlays/shared/vendor_002EB0D8.c
 * Best so far: SIZE ours 2212 / retail 2216, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-frame update for one object (state table at D_L00_0015F050, +0x1C per 32-byte entry): mode checks against 
 *   Still differing: retail reads the mode field as a word (`lw $v1,0x2C($fp)`) at two compares where ours reads a
 *   Wall-free so far; unblock by matching the mode-5 block's temporaries (retail keeps the q pointer in a register
 */
extern char *D_L00_0015F050 MACRO_ADDR;
extern char *D_L00_00166F00_2eb938 __asm__("D_L00_00166F00") NOT_SDA;
extern char D_L00_00166D80_c[] __asm__("D_L00_00166D80") NOT_SDA;
extern char D_0013E633[] NOT_SDA;
extern unsigned char D_0013A5E0[];
extern float D_L00_00166ED8 MACRO_ADDR;
extern float D_L00_00166FB4;
extern int func_L00_002E9870(char *);
extern void func_L00_002E9838(char *);
extern float func_001FA850(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9C30_2eb938(void *, void *, f32) __asm__("func_001F9C30");
extern void func_001F9BD8(void *, void *, void *);
extern f32 func_001F9CB8_2eb938(void *) __asm__("func_001F9CB8");
extern int func_001F9850_2eb938(int) __asm__("func_001F9850");
extern float func_001FA888(int);
extern f32 func_001F9F90_2eb938(f32) __asm__("func_001F9F90");
extern f32 func_001F9FA8_2eb938(f32) __asm__("func_001F9FA8");
extern void func_L00_002E9DC8_2eb938(void *, f32, f32) __asm__("func_L00_002E9DC8");
extern void func_L00_002E9B30(void);
extern void func_L00_002E9AD0(void);
extern void func_L00_002E9AF8(void);
extern void func_L00_002E9900(float, float, int);
extern void func_L00_002E9940(float);
extern void func_L00_002E9968(float, float);
extern void func_L00_002E99A0(int, float, float);
extern void func_L00_002E99F0(float);
extern void func_L00_002E9A18(int);
extern void func_L00_002E9A40(float, float);

/* Per-frame update for one object: runs its state machine and drives its motion calls. */
void func_L00_002EDE38(char *p) {
    char tmp[16] __attribute__((aligned(16)));
    char *st = *(char **)(D_L00_0015F050 + (*(short *)(p + 0x84) << 5) + 0x1C);
    short mode = *(short *)(st + 0x2C);
    int f17;
    short s16;
    int r;
    float f0, f20, fx, t;

    if (mode == 8) {
        if (*(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x230) == 0) goto L_END;
    }
    if (mode == 6) {
        if (*(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x230) != 0) goto L_END;
    }

    if (func_L00_002EDC98(p) == 0) goto L_EDF78;
    r = func_L00_002E9870(p);
    if (r <= 0) {
        if (*(short *)(st + 0x2E) != 0) *(short *)(st + 0x4A) = 1;
        *(short *)(st + 0x2E) = 0;
        goto L_END;
    }
    if (*(short *)(st + 0x36) == 0) goto L_EDF4C;
    {
        char *e = D_L00_0015F050 + (*(short *)(p + 0x84) << 5);
        float k = D_L00_00166ED8;
        f0 = func_001FA850(*(float *)(e + 0x18), k);
    }
    if (1.39626336f <= f0) goto L_EDF78;

L_EDF4C:
    if (!(*(short *)(st + 0x48) != 0 && *(short *)(st + 0x2C) == 5 &&
        (*(int *)(D_0013A5E0 + 0x2600) & 5) != 0))
        goto L_EDF9C;
L_EDF78:
    if (*(short *)(st + 0x2E) != 0) *(short *)(st + 0x4A) = 1;
    *(short *)(st + 0x2E) = 0;
    func_L00_002E9838(p);
    goto L_END;

L_EDF9C:
    if (*(short *)(st + 0x2E) == 0) {
        *(float *)(st + 0x40) = *(float *)(*(char **)(*(char **)(D_L00_00166D80_c + 0x180) + 0x70) + 0x11C);
        *(float *)(st + 0x44) = *(float *)(*(char **)(*(char **)(D_L00_00166D80_c + 0x180) + 0x70) + 0x120);
    }

    if (*(short *)(st + 0x2C) == 5) {
        char *m70, *a16, *a18, *q, *a20, *a19, *a22, *a17, *a21, *a23;
        *(int *)st = 0;
        m70 = *(char **)(D_L00_00166F00_2eb938 + 0x70);
        a16 = m70;
        a18 = m70 + 0x130;
        q = m70 + 0x1D0;
        a20 = m70 + 0x40;
        a19 = a16 + 0x20;
        a22 = D_L00_00166F00_2eb938 + 0x30;
        func_L00_001FF4B0(a18, a18, *(float *)(st + 0x24));
        *(float *)(a18 + 0x2C) = *(float *)(st + 0x24);
        *(float *)(a18 + 0x30) = *(float *)(st + 0x28);
        *(float *)(a20 + 0xB0) = *(float *)(st + 0x30);
        qcopy(a16 + 0x140, a18);
        a17 = D_0013E633 + 0x10AD;
        func_001F9C30_2eb938(tmp, a17, -*(float *)(a18 + 0x30));
        a21 = a16 + 0x90;
        a23 = a16 + 0x80;
        func_001F9BD8(a21, D_0013E633 + 0xE9D, tmp);
        func_001F9BD8(a22, a21, a18);
        func_001F9C30_2eb938(tmp, a17, -*(float *)(a20 + 0xB0));
        func_001F9BD8(a23, D_0013E633 + 0xE9D, tmp);
        qcopy(a16 + 0xD0, a23);
        qcopy(a20, D_0013E633 + 0xE9D);
        qcopy(a16 + 0xA0, a20);
        qcopy(a16, a22);
        qcopy(a16 + 0x1F0, a22);
        f0 = func_001F9CB8_2eb938(a18);
        *(float *)(q + 0x30) = *(float *)(a18 + 0x2C) - f0;
        *(int *)(a19 + 0x14) = 0;
        {
            float f1v = *(float *)(a20 + 0xB0);
            float f2v;
            f0 = *(float *)(a19 + 0x14);
            *(float *)(a19 + 4) = f1v;
            f2v = *(float *)(a18 + 0x30);
            *(float *)(a19 + 0xC) = f0;
            *(float *)(a19 + 8) = f2v;
            *(float *)(a19 + 0x18) = f0;
            *(float *)(a19 + 0x10) = f0;
        }
    }

    f17 = 0;
    r = func_001F9850_2eb938(0xC8);
    f20 = func_001FA888(r);
    *(short *)(st + 0x2E) = *(unsigned short *)(st + 0x2E) + 1;
    if ((*(short *)(st + 0x2C)) == 10) {
        func_L00_002E9B30();
        *(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x230) = 0x14D;
    }
    if ((*(short *)(st + 0x2C)) == 11) {
        func_L00_002E9B30();
        *(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x230) = 2;
    }

    if ((*(short *)(st + 0x2C)) == 2 || (*(short *)(st + 0x2C)) == 4) {
        *(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x1B8) |= 3;
        s16 = *(short *)(st + 0x2E);
        r = func_001F9850_2eb938(0xC8);
        if (r < s16) {
            f17 = 1;
            *(short *)(st + 0x2E) = func_001F9850_2eb938(0xC8);
        }
        func_L00_002E9AD0();
        goto L_EE328;
    }

    if ((*(short *)(st + 0x2C)) == 1 || (*(short *)(st + 0x2C)) == 9 || (*(short *)(st + 0x2C)) == 10) goto L_EE224;
    if ((*(short *)(st + 0x2C)) == 7) goto L_EE228;
    goto L_EE2C8;

L_EE224:
L_EE228:
    if (*(float *)(D_0013A5E0 + 0x2460 + 0x100) == 0.0f &&
        *(float *)(D_0013A5E0 + 0x2460 + 0x104) == 0.0f) {
        s16 = *(short *)(st + 0x2E);
        goto L_EE268;
    }
    *(short *)(st + 0x2E) = func_001F9850_2eb938(0xC8);
    s16 = *(short *)(st + 0x2E);

L_EE268:
    r = func_001F9850_2eb938(0xC8);
    if (s16 < r) {
        s16 = *(short *)(st + 0x2E);
        goto L_EE2A8;
    }
    f17 = 1;
    if (0.0f < D_L00_00166FB4) *(short *)(st + 0x2E) = func_001F9850_2eb938(0xC8);
    s16 = *(short *)(st + 0x2E);

L_EE2A8:
    r = func_001F9850_2eb938(0x190);
    if (s16 >= r) {
        f17 = 0;
        *(short *)(st + 0x2E) = 1;
    }
    goto L_EE328;

L_EE2C8:
    if ((*(short *)(st + 0x2C)) == 3) {
        f20 = func_001FA888(func_001F9850_2eb938(1));
        s16 = *(short *)(st + 0x2E);
        if (func_001F9850_2eb938(1) < s16) {
            f17 = 1;
            *(short *)(st + 0x2E) = func_001F9850_2eb938(1);
        }
        goto L_EE328;
    }
    s16 = *(short *)(st + 0x2E);
    if (func_001F9850_2eb938(0xC8) < s16) {
        f17 = 1;
        *(short *)(st + 0x2E) = func_001F9850_2eb938(0xC8);
    }

L_EE328:
    fx = func_001F9F90_2eb938(*(float *)(D_L00_0015F050 + (*(short *)(p + 0x84) << 5) + 0x18));
    *(float *)tmp = fx;
    *(float *)(tmp + 4) = func_001F9FA8_2eb938(*(float *)(D_L00_0015F050 + (*(short *)(p + 0x84) << 5) + 0x18));
    *(int *)(tmp + 8) = 0;
    fx = (*(float *)st * 0.0174532924f) * ((float)*(short *)(st + 0x2E) / f20);

    if (!(((*(short *)(st + 0x2C)) == 1 || (*(short *)(st + 0x2C)) == 7 || (*(short *)(st + 0x2C)) == 9) && f17 != 0)) {
        f0 = *(float *)(st + 0x20);
        func_L00_002E9DC8_2eb938(tmp, fx, f0 * 0.0174532924f);
    }
    fx = *(float *)(st + 0x24);

    f0 = 0.0f;
    if (fx == f0) {
        fx = *(float *)(st + 0x28);
        goto L_EE46C;
    }
    if ((*(short *)(st + 0x2C)) == 6 || (*(short *)(st + 0x2C)) == 8) {
        func_L00_002E9900(fx, 0.00200000009f, 0);
        func_L00_002E9940(*(float *)(st + 0x24) + 1.36000001f);
    } else {
        func_L00_002E9900(fx, 0.00300000003f, 0);
        func_L00_002E9AD0();
    }
    fx = *(float *)(st + 0x28);

    f0 = 0.0f;
L_EE46C:
    if (fx == f0) {
        fx = *(float *)(st + 0x30);
        goto L_EE4CC;
    }
    if ((*(short *)(st + 0x2C)) == 6 || (*(short *)(st + 0x2C)) == 8) func_L00_002E9968(fx, 0.00200000009f);
    else func_L00_002E9968(fx, 0.00300000003f);
    fx = *(float *)(st + 0x30);

L_EE4CC:
    f0 = 0.0f;
    if (fx == f0) {
        f0 = *(float *)(st + 0x50);
        goto L_EE530;
    }
    if ((*(short *)(st + 0x2C)) == 6 || (*(short *)(st + 0x2C)) == 8) func_L00_002E99A0(0, fx, 0.00200000009f);
    else func_L00_002E99A0(0, fx, 0.00499999989f);
    f0 = *(float *)(st + 0x50);

L_EE530:
    f20 = 0.0f;
    if (f0 != f20) func_L00_002E99F0(f0 * 0.0174532924f);
    if (*(short *)(st + 0x54) != 0)
        *(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x1B8) |= 2;
    if (*(short *)(st + 0x56) != 0)
        *(int *)(*(char **)(D_L00_00166F00_2eb938 + 0x70) + 0x1B8) |= 1;

    if (*(short *)(st + 0x34) == 0) func_L00_002E9A18(0);
    f0 = *(float *)(st + 0x38);

    if (f0 == f20) {
        f0 = *(float *)(st + 0x3C);
        if (f0 == f20) goto L_EE660;
    }
    s16 = *(short *)(st + 0x2E);

    r = func_001F9850_2eb938(0x78);
    if (s16 < r) {
        f0 = func_001FA888(func_001F9850_2eb938(0x78));
        t = (float)*(short *)(st + 0x2E) / f0;
        func_L00_002E9A40(*(float *)(st + 0x40) + (*(float *)(st + 0x38) - *(float *)(st + 0x40)) * t,
                          *(float *)(st + 0x44) + (*(float *)(st + 0x3C) - *(float *)(st + 0x44)) * t);
    } else {
        func_L00_002E9A40(*(float *)(st + 0x38), *(float *)(st + 0x3C));
    }

L_EE660:
    if ((*(short *)(st + 0x2C)) == 1 || (*(short *)(st + 0x2C)) == 7 || (*(short *)(st + 0x2C)) == 10) {
        func_L00_002E9AD0();
        func_L00_002E9AF8();
        func_L00_002E9A40(0.00999999978f, 0.200000003f);
    }
L_END:
    ;
}
