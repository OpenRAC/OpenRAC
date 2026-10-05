/* NON_MATCHING func_L00_002091D8 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 8/956 (99.2% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002091D8 (9 of 10 runs used; best candidate p8.c, 948 vs 956 bytes, p7.c is close too): steers the pl
 *   (clamped yaw delta via func_001FA790/001FA748, aim vector into D_L00_001AEF10), then runs a 4-probe loop over 
 *   collecting lo/hi limits into moby+0x84/0x88.
 *   Difference: saved float registers. Retail puts cs(F90(ang)) in $f26 and the 2^-10 loop constant in $f25; ours 
 *   (cs $f25, const $f26), and that reorders the loop's mul/add schedule. The loop's p-pointer hi is a fresh lui i
 *   shared with $s5 (PRE) in p7; p8 makes it fresh but moves a lui into the bc1f delay slot. Structure otherwise r
 *   Unblock: something that lowers cs's allocation priority relative to the hoisted constant (unknown), then re-tu
 */
typedef s32 Q_208b60 __attribute__((mode(TI)));
typedef union { Q_208b60 q; f32 f[4]; } V_208b60;
typedef struct { u8 p0[0xC]; f32 fC; u8 p10[0x34 - 0x10]; u16 h34; u8 p36[0x84 - 0x36]; f32 f84; f32 f88; u8 p8C[0xBD - 0x8C]; u8 bBD; } M_208b60;
typedef struct {
    u8 p0[0x2D8]; f32 f2D8; u8 p2DC[0x2F0 - 0x2DC]; f32 f2F0; f32 f2F4; u8 p2F8[0x30E - 0x2F8]; s16 h30E;
    u8 p310[0xD20 - 0x310]; f32 fD20; u8 pD24[0x2080 - 0xD24]; M_208b60 *m2080; s32 i2084; u8 p2088[4]; s32 i208C;
} G_208b60;
typedef struct { f32 x, y, z, w; } T_208b60;
typedef struct { u8 p0[0x28]; f32 f28; } U_208b60;
extern G_208b60 D_0013F450_091D8 __asm__("D_0013F450");
extern V_208b60 D_L00_001AEF10;
extern T_208b60 D_L00_0017C400[];
extern U_208b60 D_L00_00173F40;
f32 func_001FA790(f32, f32);
f32 func_001FA748(f32, f32);
void func_L00_00251388_091D8(M_208b60 *, V_208b60 *) __asm__("func_L00_00251388");
f32 func_L00_001FF860(f32, f32);
f32 func_001F9F90(f32);
f32 func_001F9FA8(f32);
void func_001F9C30_091D8(V_208b60 *, M_208b60 *, f32) __asm__("func_001F9C30");
void func_L00_001FF4B0(V_208b60 *, V_208b60 *, f32);
void func_001F9BD8_091D8(V_208b60 *, V_208b60 *, V_208b60 *) __asm__("func_001F9BD8");
s32 func_L00_001EFFF0_091D8(V_208b60 *, V_208b60 *, s32, s32, s32) __asm__("func_L00_001EFFF0");
f32 func_001F9B98(f32, f32);
f32 func_001F9B90(f32, f32);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/ui_help_00203b18.c, FUN_L00_00208b60. */
void func_L00_002091D8(void) {
    V_208b60 v, w, x;
    f32 k, d, a, c, s, h, hi, lo, sc;
    T_208b60 *t;
    V_208b60 *pw, *px;
    s32 i;
    if (D_0013F450_091D8.i208C == 0x12 || D_0013F450_091D8.i208C == 0x15 || D_0013F450_091D8.i2084 == 0x7D || D_0013F450_091D8.i208C == 0x16) {
        D_0013F450_091D8.m2080->h34 &= 0xFBFF;
        return;
    }
    D_0013F450_091D8.m2080->h34 |= 0x400;
    D_0013F450_091D8.m2080->bBD = 1;
    k = 0.97f;
    if (D_0013F450_091D8.h30E != 0 || D_0013F450_091D8.i208C == 4) k = 1.5f;
    d = func_001FA790(k, D_0013F450_091D8.fD20);
    if (0.052f < d) d = 0.052f;
    else if (d < -0.052f) d = -0.052f;
    D_0013F450_091D8.fD20 = func_001FA748(D_0013F450_091D8.fD20, d);
    func_L00_00251388_091D8(D_0013F450_091D8.m2080, &v);
    a = func_L00_001FF860(v.f[0], v.f[1]);
    c = func_001F9F90(a);
    s = func_001F9FA8(a);
    {f32 e = func_001F9F90(D_0013F450_091D8.fD20);
    v.f[0] = c * e;
    v.f[1] = s * e;
    }
    v.f[2] = -func_001F9FA8(D_0013F450_091D8.fD20);
    qcopy(&D_L00_001AEF10, &v);
    h = D_0013F450_091D8.f2D8;
    if (h < 1.0f) {
        f32 t = D_0013F450_091D8.f2F0;
        if (1.0f <= t) h = t;
        else { t = D_0013F450_091D8.f2F4; if (1.0f < t) h = t; else { D_0013F450_091D8.m2080->f84 = 0.0f; D_0013F450_091D8.m2080->f88 = 0.0f; return; } }
    }
    hi = h;
    lo = hi;
    pw = &w; px = &x;
    t = D_L00_0017C400;
    for (i = 3; i >= 0; i--, t++) {
        M_208b60 *m = D_0013F450_091D8.m2080;
        sc = m->fC * 0.0009765625f;
        func_001F9C30_091D8(pw, m, 0.0009765625f);
        w.f[0] += c * t->x * sc;
        w.f[1] += s * t->y * sc;
        w.f[2] += t->z * sc;
        func_L00_001FF4B0(px, &v, ((w.f[2] - h) + 0.5f) / -v.f[2]);
        func_001F9BD8_091D8(px, pw, px);
        if (func_L00_001EFFF0_091D8(pw, px, 0x22, 0, 0)) {
            lo = func_001F9B98(lo, D_L00_00173F40.f28);
            hi = func_001F9B90(hi, D_L00_00173F40.f28);
        }
    }
    D_0013F450_091D8.m2080->f84 = lo - 0.12f;
    D_0013F450_091D8.m2080->f88 = hi + 0.24f;
    {
        M_208b60 *m = D_0013F450_091D8.m2080;
        f32 z = m->f84 + 3.0f;
        if (z < m->f88) m->f88 = z;
    }
}
