/* Veldin level code: per-moby step for the state at E+0x1C (retail func_L00_002EE948, 0x414 bytes).
 * E is the 32-byte record D_L00_0015F050[a->0x84]; r30 = E->0x1C holds the state: w20 (timer), h24, h26,
 * f28, f2C, f38, f3C, f40 (aim angles). With the timer below 2 it rebuilds the vector block of r30's
 * partner at D_0013E633 + 0xE9D / + 0x10AD, then steps the aim and calls the helpers. */
typedef struct { float x, y, z, w; } Vx_EE948 __attribute__((aligned(16)));
extern char *D_L00_0015F050_EE948 __asm__("D_L00_0015F050") NOT_SDA;
extern char D_0013A5E0_EE948[] __asm__("D_0013A5E0") NOT_SDA;
extern int func_L00_002E9870_EE948(void *) __asm__("func_L00_002E9870");
extern void func_L00_002E9838_EE948(void *) __asm__("func_L00_002E9838");
extern void func_L00_002E9AD0_EE948(void) __asm__("func_L00_002E9AD0");
extern void func_L00_002E9900_EE948(int, float, float) __asm__("func_L00_002E9900");
extern void func_L00_002E9968_EE948(float, float) __asm__("func_L00_002E9968");
extern void func_L00_002E99A0_EE948(int, float, float) __asm__("func_L00_002E99A0");
extern void func_L00_002E99F0_EE948(float) __asm__("func_L00_002E99F0");
extern void func_L00_002E9A18_EE948(int) __asm__("func_L00_002E9A18");

void func_L00_002EE948(void *a0) {
    char *a = (char *)a0;
    char *G1 = D_L00_0015F050_EE948;
    char *E;
    char *r30;
    char *Q;
    char *T;
    char *o;
    char *L;
    char *P40;
    char *Pm;
    char *B19;
    s32 v, ret, w20, h24;
    float f0, f1, f2, f12, f20, f21;
    Vx_EE948 v0, v10, v20, v30;

    E = G1 + ((s32)*(s16 *)(a + 0x84) << 5);
    r30 = *(char **)(E + 0x1C);
    ret = func_L00_002EE7F8(a);
    if (ret == 0) {
        v = *(s32 *)(r30 + 0x20);
        goto L9DC;
    }
    ret = func_L00_002E9870_EE948(a);
    if (ret <= 0) {
        v = *(s32 *)(r30 + 0x20);
        if (v != 0)
            *(s16 *)(r30 + 0x26) = 1;
        *(s32 *)(r30 + 0x20) = 0;
        return;
    }
    if ((*(s32 *)(D_0013A5E0_EE948 + 0x2600) & 5) == 0) {
        v = *(s32 *)(r30 + 0x20);
        goto L9FC;
    }
    v = *(s32 *)(r30 + 0x20);

L9DC:
    if (v != 0)
        *(s16 *)(r30 + 0x26) = 1;
    *(s32 *)(r30 + 0x20) = 0;
    func_L00_002E9838_EE948(a);
    return;

L9FC:
    if (v >= 2)
        goto L_EC30;

    /* timer below 2: rebuild the vectors */
    Q = D_0013E633 + 0xE9D;
    T = D_0013E633 + 0x10AD;
    v0.x = *(float *)(E + 0x0);
    v0.y = *(float *)(E + 0x4);
    v0.z = *(float *)(E + 0x8);
    v0.w = 0.0f;
    func_001F9BF0(&v10, &v0, Q);
    f20 = func_001F9C78(&v10, T);
    func_001F9C30(&v20, T, f20);
    func_001F9BF0(&v30, &v10, &v20);
    f0 = func_001F9CB8(&v30);
    f20 = -f20;
    f21 = *(float *)(r30 + 0x2C);
    *(float *)(r30 + 0x38) = f0;
    *(float *)(r30 + 0x3C) = f20;
    f0 = func_001F9B88(f21);
    if (f0 == 1.5707964f) {
        *(s32 *)(r30 + 0x40) = 0;
    } else {
        f20 = func_001F9FA8_2eb938(f21);
        f0 = func_001F9F90_2eb938(f21);
        f20 = f20 / f0;
        f1 = *(float *)(r30 + 0x3C);
        f0 = *(float *)(r30 + 0x38) * f20;
        f1 = f1 - f0;
        *(float *)(r30 + 0x40) = f1;
    }

    Pm = D_L00_00166F00_2eb938;
    o = *(char **)(Pm + 0x70);
    B19 = Pm + 0x30;
    P40 = o + 0x40;
    *(Vx_EE948 *)B19 = v0;
    L = o + 0x130;
    *(float *)(L + 0x2C) = *(float *)(r30 + 0x38);
    *(float *)(L + 0x30) = *(float *)(r30 + 0x3C);
    *(float *)(P40 + 0xB0) = *(float *)(r30 + 0x40);
    func_001F9C30(&v20, T, -*(float *)(L + 0x30));
    func_001F9BD8(o + 0x90, Q, &v20);
    func_001F9BF0(L, &v0, o + 0x90);
    *(Vx_EE948 *)(o + 0x140) = *(Vx_EE948 *)L;
    func_001F9C30(&v20, T, -*(float *)(P40 + 0xB0));
    func_001F9BD8(o + 0x80, Q, &v20);
    *(Vx_EE948 *)(o + 0xD0) = *(Vx_EE948 *)(o + 0x80);
    *(Vx_EE948 *)P40 = *(Vx_EE948 *)Q;
    *(Vx_EE948 *)(o + 0xA0) = *(Vx_EE948 *)P40;
    *(Vx_EE948 *)o = *(Vx_EE948 *)B19;
    *(Vx_EE948 *)(o + 0x1F0) = *(Vx_EE948 *)B19;
    *(s32 *)(o + 0x1D0 + 0x30) = 0;
    *(s32 *)(o + 0x20 + 0x14) = 0;
    f1 = *(float *)(P40 + 0xB0);
    f0 = *(float *)(o + 0x20 + 0x14);
    *(float *)(o + 0x20 + 0x4) = f1;
    f2 = *(float *)(L + 0x30);
    *(float *)(o + 0x20 + 0xC) = f0;
    *(float *)(o + 0x20 + 0x8) = f2;
    *(s16 *)(o + 0x20 + 0x2) = 0;
    *(float *)(o + 0x20 + 0x18) = f0;
    *(float *)(o + 0x20 + 0x10) = f0;

L_EC30:
    w20 = *(s32 *)(r30 + 0x20);
    *(s32 *)(r30 + 0x20) = w20 + 1;
    ret = func_001F9850_2eb938(200);
    if (ret < *(s32 *)(r30 + 0x20))
        *(s32 *)(r30 + 0x20) = func_001F9850_2eb938(200);
    f12 = *(float *)(r30 + 0x38);

    f20 = 0.0f;
    if (f12 == f20) {
        f12 = *(float *)(r30 + 0x3C);
    } else {
        func_L00_002E9900_EE948(0, f12, 0.003f);
        func_L00_002E9AD0_EE948();
        f12 = *(float *)(r30 + 0x3C);
    }

    if (f12 == f20) {
        f12 = *(float *)(r30 + 0x40);
    } else {
        func_L00_002E9968_EE948(f12, 0.003f);
        f12 = *(float *)(r30 + 0x40);
    }

    if (f12 == f20) {
        f0 = *(float *)(r30 + 0x28);
    } else {
        func_L00_002E99A0_EE948(0, f12, 0.005f);
        f0 = *(float *)(r30 + 0x28);
    }

    if (f0 != f20)
        func_L00_002E99F0_EE948(f0 * 0.017453292f);
    h24 = *(s16 *)(r30 + 0x24);
    if (h24 == 0)
        func_L00_002E9A18_EE948(0);
    return;
}
