/* NON_MATCHING func_L00_0020D5F0 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: BYTES 2/928 (99.8% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int Q_cf58 __attribute__((mode(TI)));
typedef struct { f32 x, y, z, w; } __attribute__((aligned(16))) V_cf58;
extern u8 D_0013D5E5 NOT_SDA;
extern s32 D_0015EE84 MACRO_ADDR;
typedef struct {
    u8 p0[0x80]; V_cf58 pos; u8 p1[0x1C8 - 0x90]; s16 h1C8; s16 h1CA; u8 p2[0x41E - 0x1CC]; s16 h41E;
    u8 p3[0x500 - 0x420]; Q_cf58 q500; u8 p4[0x560 - 0x510]; s32 w560; s32 w564; f32 f568; s32 w56C; s32 w570;
    u8 p5[0x584 - 0x574]; s32 w584; u8 p6[0x5C4 - 0x588]; s32 w5C4; u8 p7[0x2084 - 0x5C8]; s32 w2084; s32 w2088; s32 w208C;
} GS_cf58;
extern GS_cf58 D_0013F450_0D5F0 __asm__("D_0013F450");
f32 func_001F9D10_0D5F0(void *, V_cf58 *) __asm__("func_001F9D10");
s32 func_L00_0020D3A0_0D5F0(Q_cf58 *, s32 *, V_cf58 *, s32 *, f32 *, s32 *, s32, s32) __asm__("func_L00_0020D3A0");
extern s32 D_0015EE84_0D5F0b __asm__("D_0015EE84") MACRO_ADDR;

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/ui_help_0020c758.c, FUN_L00_0020cf58. */
void func_L00_0020D5F0(void) {
    V_cf58 v0, v1, v2;

    s32 a, b, e;
    f32 c;
    s32 s;
    f32 d;
    if (D_0013D5E5 == 0) return;
    D_0013F450_0D5F0.w56C = 0;
    if (D_0013F450_0D5F0.h1C8 != 0) return;
    if (D_0015EE84_0D5F0b == 16) {
        *(Q_cf58 *)&v0 = 0; v0.x = 115.0f; v0.y = 264.0f; v0.z = 133.0f;
        if (12.0f < func_001F9D10_0D5F0(&D_0013F450_0D5F0.pos, &v0)) {
            *(Q_cf58 *)&v1 = 0; v1.x = 257.0f; v1.y = 301.0f; v1.z = 128.0f;
            if (12.0f < func_001F9D10_0D5F0(&D_0013F450_0D5F0.pos, &v1)) return;
        }
    } else if (D_0015EE84_0D5F0b == 14) {
        *(Q_cf58 *)&v0 = 0; v0.x = 157.0f; v0.y = 265.0f; v0.z = 51.0f;
        if (12.0f < func_001F9D10_0D5F0(&D_0013F450_0D5F0.pos, &v0)) {
            *(Q_cf58 *)&v1 = 0; v1.x = 162.0f; v1.y = 341.0f; v1.z = 48.0f;
            if (12.0f < func_001F9D10_0D5F0(&D_0013F450_0D5F0.pos, &v1)) {
                *(Q_cf58 *)&v2 = 0; v2.x = 308.0f; v2.y = 104.0f; v2.z = 69.0f;
                if (12.0f < func_001F9D10_0D5F0(&D_0013F450_0D5F0.pos, &v2)) return;
            }
        }
    } else if (D_0015EE84_0D5F0b == 6) {
        *(Q_cf58 *)&v0 = 0; v0.x = 177.0f; v0.y = 304.0f; v0.z = 143.0f;
        if (14.0f < func_001F9D10_0D5F0(&D_0013F450_0D5F0.pos, &v0)) return;
    }
    s = D_0013F450_0D5F0.w208C;
    if (s != 2 && s != 0 && s != 1) {
        if ((u32)(D_0013F450_0D5F0.w2084 - 41) < 2 && (D_0013F450_0D5F0.w570 != 0 || D_0013F450_0D5F0.w5C4 != 0)) {
        } else {
            if (s != 4) return;
            if (D_0013F450_0D5F0.h41E == 0 && D_0013F450_0D5F0.h1CA == 0) return;
        }
    }
    *(Q_cf58 *)&v1 = *(Q_cf58 *)&D_0013F450_0D5F0.pos;
    if (!func_L00_0020D3A0_0D5F0((Q_cf58 *)&v1, &a, &v0, &b, &c, &e, 0, 0)) return;
    if (D_0013F450_0D5F0.w2084 == 41 && D_0013F450_0D5F0.w5C4 != 0 && a == D_0013F450_0D5F0.w560) return;
    d = D_0013F450_0D5F0.pos.z - v0.z;
    if (!(-0.5f < d)) return;
    if (!(d < 0.57f) && D_0013F450_0D5F0.h1CA == 0) return;
    qcopy(&D_0013F450_0D5F0.q500, &v0);
    D_0013F450_0D5F0.w560 = a;
    D_0013F450_0D5F0.w564 = b;
    D_0013F450_0D5F0.f568 = c;
    D_0013F450_0D5F0.w584 = e;
    D_0013F450_0D5F0.w56C = 1;
}
