/* NON_MATCHING func_L00_002C0358 -- src/overlays/shared/vendor_002BA7C8.c
 * Best so far: BYTES 1/1980 (100.0% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int V4 __attribute__((mode(TI), aligned(16)));
typedef union { V4 q; f32 f[4]; } VU;
typedef struct { u8 pad0[0x46]; s16 h46; } Cls;
typedef struct { u8 pad0[0x24]; Cls *p24; } Hit;
typedef struct { u8 pad0[0x38]; Hit *p38; } Ext;
typedef struct {
    u8 pad0[0x10];
    f32 f10;
    f32 f14;
    u8 pad18[0x30];
    f32 f48;
    u8 pad4C[0x2C];
    Ext *p78;
} Mob;
typedef struct { u8 pad0[0x18]; Hit *unk18; } XD;
extern Hit * D_L00_00173F58_C0358 __asm__("D_L00_00173F58");
extern XD D_L00_00173F40_C0358 __asm__("D_L00_00173F40");
extern VU D_L00_00173F60;
extern s32 func_001FA898(f32);
extern f32 func_L00_001FF860(f32, f32);
extern f32 func_001FA748(f32, f32);
extern f32 func_001FA790(f32, f32);
extern f32 func_001FA850(f32, f32);
extern f32 func_001F9F90(f32);
extern f32 func_001F9FA8(f32);
extern f32 func_001F9D10(void *, void *);
extern s32 func_L00_001EFFF0_C0358(void *, void *, s32, void *, s32) __asm__("func_L00_001EFFF0");
extern f32 func_L00_002BFF88_C0358(void *, void *) __asm__("func_L00_002BFF88");

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/unclassified_002b94d0.c, FUN_L00_002bf050. */
f32 func_L00_002C0358(Mob *m, VU *pos, f32 *tgt, f32 *out, f32 ang, f32 arc, f32 step, f32 h, f32 r2, f32 r1) {
    VU v0;
    VU v1;
    f32 da;
    s32 n;
    s32 i;
    f32 best;
    f32 bestDist;
    f32 bestDiff;
    f32 cur;
    f32 a;
    f32 h2;
    f32 d;
    f32 diff;
    Ext *x;
    Hit *t;

    da = arc / step;
    best = ang;
    n = func_001FA898(step);
    bestDist = 0.0f;
    x = m->p78;
    bestDiff = 6.2831855f;
    *out = bestDist;
    if (tgt != 0) {
        cur = func_L00_001FF860(tgt[0] - m->f10, tgt[1] - m->f14);
    } else {
        cur = ang;
    }
    arc *= 0.5f;
    func_001FA748(ang, arc);
    arc = func_001FA790(ang, arc);

    qcopy(&v0, pos);
    v0.f[1] += func_001F9FA8(cur) * r1;
    v0.f[0] += func_001F9F90(cur) * r1;
    v0.f[2] += h;
    qcopy(&v1, pos);
    v1.f[1] += func_001F9FA8(cur) * r2;
    v1.f[0] += func_001F9F90(cur) * r2;
    v1.f[2] += h + h;
    if (func_L00_001EFFF0_C0358(&v0, &v1, 4, m, 0) == 0) {
        if (func_L00_002BFF88_C0358(&v1, m) != 0.0f) {
            best = cur;
            bestDist = r2;
            *out = r2;
        }
    } else {
        if (D_L00_00173F58_C0358 == x->p38 || (D_L00_00173F58_C0358 != 0 && D_L00_00173F58_C0358->p24 != 0 && D_L00_00173F58_C0358->p24->h46 == 5)) {
            *out = 100.0f;
            return cur;
        }
        best = cur;
        bestDist = func_001F9D10(&v0, &D_L00_00173F60);
        bestDiff = 0.0f;
        if (*out < bestDist) {
            *out = bestDist;
        }
    }

    for (i = 0; i < n; i++) {
        qcopy(&v0, pos);
        v0.f[1] += func_001F9FA8(arc) * r1;
        v0.f[0] += func_001F9F90(arc) * r1;
        v0.f[2] += h;
        qcopy(&v1, pos);
        v1.f[1] += func_001F9FA8(arc) * r2;
        v1.f[0] += func_001F9F90(arc) * r2;
        v1.f[2] += h + h;
        if (func_L00_001EFFF0_C0358(&v0, &v1, 4, m, 0) == 0) {
            if (func_L00_002BFF88_C0358(&v1, m) != 0.0f) {
                diff = func_001FA850(arc, cur);
                if (diff < bestDiff || bestDist < r2) {
                    bestDiff = diff;
                    best = arc;
                    bestDist = r2;
                    if (*out < r2) {
                        *out = r2;
                    }
                }
            }
        } else {
            if (D_L00_00173F40_C0358.unk18 == x->p38 || (D_L00_00173F40_C0358.unk18 != 0 && D_L00_00173F40_C0358.unk18->p24 != 0 && D_L00_00173F40_C0358.unk18->p24->h46 == 5)) {
                *out = 100.0f;
                return arc;
            }
            d = func_001F9D10(&v0, &D_L00_00173F60);
            if (bestDist < d) {
                best = arc;
                bestDist = d;
                bestDiff = func_001FA850(best, cur);
                if (*out < d) {
                    *out = d;
                }
            } else if (d == bestDist) {
                diff = func_001FA850(arc, cur);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    best = arc;
                    bestDist = d;
                    if (*out < d) {
                        *out = d;
                    }
                }
            }
        }
        arc = func_001FA748(arc, da);
    }

    ang = func_001FA790(best, 1.5707964f);
    qcopy(&v0, pos);
    v0.f[1] += func_001F9FA8(ang) * r1;
    v0.f[0] += func_001F9F90(ang) * r1;
    v0.f[2] += h * 0.125f;
    qcopy(&v1, &v0);
    v1.f[1] += func_001F9FA8(best) * r2;
    v1.f[0] += func_001F9F90(best) * r2;
    v1.f[2] += 0.12f;
    if (func_L00_001EFFF0_C0358(&v0, &v1, 4, m, 0) != 0) {
        if (D_L00_00173F58_C0358 != x->p38 && !(D_L00_00173F58_C0358 != 0 && D_L00_00173F58_C0358->p24 != 0 && D_L00_00173F58_C0358->p24->h46 == 5)) {
            d = func_001F9D10(&v0, &D_L00_00173F60);
            best = func_001FA748(best, (r2 - d) * 3.1415927f / (r2 * 3.0f));
        }
    }
    ang = func_001FA748(ang, 3.1415927f);
    qcopy(&v0, pos);
    v0.f[1] += func_001F9FA8(ang) * r1;
    v0.f[0] += func_001F9F90(ang) * r1;
    v0.f[2] += h * 0.125f;
    qcopy(&v1, &v0);
    v1.f[1] += func_001F9FA8(best) * r2;
    v1.f[0] += func_001F9F90(best) * r2;
    v1.f[2] += 0.12f;
    if (func_L00_001EFFF0_C0358(&v0, &v1, 4, m, 0) != 0) {
        if (D_L00_00173F58_C0358 != x->p38 && !(D_L00_00173F58_C0358 != 0 && D_L00_00173F58_C0358->p24 != 0 && D_L00_00173F58_C0358->p24->h46 == 5)) {
            d = func_001F9D10(&v0, &D_L00_00173F60);
            best = func_001FA790(best, (r2 - d) * 3.1415927f / (r2 * 3.0f));
        }
    }
    if (2.5132742f < func_001FA850(best, m->f48)) {
        best = func_001FA748(m->f48, func_001FA790(best, m->f48) * 0.5f);
    }
    return best;
}
