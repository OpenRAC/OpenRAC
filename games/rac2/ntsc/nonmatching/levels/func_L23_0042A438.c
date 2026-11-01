/* Retained reconstruction; not linked or counted.
 * Tier: GPT-6.1 Sol (high effort), task v1008, 29 October 2026.
 * Current US profile: -O2 -G8 -ffunction-sections -g -fno-rerun-loop-opt
 * -frac2-qcopy -frac2-qcopy-barrier. Best valid result: 1136/1156 bytes,
 * 0/1 places match. Other releases not checked for this draft.
 * A complete matrix object, independent basis pointer and early alpha-scale
 * snapshot improve the matrix/FP lifetimes. Separating scaledLength from the
 * final ribbonWidth also recovers the retail f0 intermediate and f12 product.
 * Remaining: the alpha word is forwarded into the packed GS value instead of
 * reloaded; global stores, packet colors, pointer allocation and scheduling differ.
 * Signed full-width bitfield and signed/unsigned union views did not restore the
 * alpha reload. A volatile field adds a GPR load/mtc1 sequence instead of lwc1.
 * A 1152-byte diagnostic used qzero_nc(axis), but it was rejected because the
 * later axis Z store overlaps that clear. This retained draft uses qzero(axis).
 * Keep qcopy for row 3, whose W component is then written. qzero_nc on row 3
 * precedes the external matrix call; surrounding C does not access that row.
 * Reopen with evidence for the extra alpha reload and the two matrix lifetimes.
 * All required declarations are included below. The October 22 notes are historical.
 */

#include "common.h"

/* func_L23_0042A438 (1156 bytes, level 23 only) -- NEAR MISS, not integrated.
   Measured on 22 October 2026 with the compiler built from master (qcopy barrier and the
   annulled-wide-store switch included): this C compiles to 1140 bytes, 16 short of retail's
   1156; check.py: "1140 bytes, expected 1156". It belongs in a unit of its own,
   src/levels/23_damosel_orbit/L23_0042A438-g8.c ($gp data, hence -G8). The best profile today is
   `-O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -frac2-zero-ti-barrier`: with an
   aligned diff that ignores registers, 71 rows differ (88 with the earlier `-frac2-qcopy
   -frac2-zero-ti-barrier`, 81 with the two qcopy switches alone); -frac2-no-annulled-wide-stores
   changes nothing. All of retail's copies here are grouped (`addiu; addiu; lq 0; sq 0`), so the
   plain TI assignments below are the right spelling for them.
   Right: the frame (0x1F0), the handle release, the three `func_002A7878` jitters with their
   loads after each call, func_L00_002FCF18 and the handle store, the GS words up to the calls,
   and the loop body's call sequence with all its constants and tables.
   What differs, and why it is not (only) a source question:
   - the two 16-byte clears, retail +0x228 `addiu a0,sp,0xC0; sq $zero,0(a0)` and +0x268
     `addiu v0,sp,0xE0; ... sq $zero,0(v0)`: this cc1 gives `por v0; sq v0,0xC0(sp)` and
     `por v0; sq v0,0xE0(sp)`. A 16-byte zero store is not a qcopy pattern, CSE folds the stack
     offset into it and rac2_fold_zero_ti_store only folds a register address; retail has 57 such
     register-address clears and no C in src/ reproduces one (see func_L23_0042A018's record,
     which has the same clear). Retail's clears and copies have the shape of volatile-asm macros,
     at which CSE also flushes its table: here CSE keeps `mp`'s address (sp+0x90) in s5 from the
     prologue on (+0x1C), where retail forms it after the first clear (+0x230);
   - the counter and the matrix address in two stack slots (0x170, 0x174) in retail, a saved
     register here (unchanged from the earlier record);
   - D_L23_001B3930's two stores: retail writes them absolute (`lui at; swc1`) before the `bc1f`,
     with `mul.s f12,f0,f3` in the slot; here the first store fills the slot (unchanged);
   - retail re-reads vars->alpha from memory after storing it (`sw v1,4(s0)`, then `lw a0,4(s0)`
     for the ALPHA word and `lwc1 f0,4(s0); cvt.s.w` for D_L23_001B1C18), all after the
     `sra; beql; break 7` divisor check; this cc1 forwards the stored register to both reads.
     Storing the field before the `%` statement (a block boundary for CSE) makes the float read a
     load again but not the int one, and puts the store before the check where retail has it
     after (74 rows). func_L23_0042A018, whose ALPHA word is the same, does forward the stored
     value in retail (`daddu v0,a2,zero`), so the difference is in this function's source.
   What would unblock it: the same opt-in register-address clear (and CSE flush at the macros) as
   func_L23_0042A018, then the stack-slot loop and the alpha re-reads.
   Earlier measurements (still valid): `TI16 *mp = m` for func_L03_002EFB60 and func_00283098 (a
   second name for the matrix) took 136 to 100 rows and +16 bytes; the `%` whose quotient is unused
   (written `i = ...`, overwritten by the loop) is the only way found to get retail's lone divisor
   check; a pointer-walk loop and a descending loop with an index expression (151 rows) were
   worse. */













typedef int TI16 __attribute__((mode(TI)));

/* the state object word, read through an absolute access */
extern u8 *D_L23_001B1484_2;

extern float D_L23_001B1470;

extern s32 D_L23_001B1474;

extern float D_L23_001B39D0 __attribute__((sda));

extern float D_L23_001B3930;

extern s16 D_L23_001B1B94 __attribute__((sda));

extern s32 D_L23_001B1BAC __attribute__((sda));

extern s32 D_L23_001B1BB0 __attribute__((sda));

extern s32 D_L23_001B1BB4 __attribute__((sda));

extern s32 D_L23_001B1BB8 __attribute__((sda));

extern s32 D_L23_001B1BC4 __attribute__((sda));

extern s32 D_L23_001B1BC8 __attribute__((sda));

extern float D_L23_001B1C00 __attribute__((sda));

extern float D_L23_001B1C04 __attribute__((sda));

extern float D_L23_001B1C0C __attribute__((sda));

extern float D_L23_001B1C10 __attribute__((sda));

extern s32 D_L23_001B1C14 __attribute__((sda));

extern float D_L23_001B1C18 __attribute__((sda));

extern float D_L23_001B1C1C __attribute__((sda));

extern float D_L23_001B1C20 __attribute__((sda));

extern s32 D_L23_001B1C24 __attribute__((sda));

extern float D_L23_001B1C28 __attribute__((sda));

extern float D_L23_001B1C2C __attribute__((sda));

extern float D_L23_001B1C30 __attribute__((sda));

/* the quad record handed to the draw helper */
typedef struct {
    TI16 pos[4];
    u32 col[4];
    float uv[4][2];
    s64 tail[4];
} L23Quad;

/* the spin state at object + 0x68 */
typedef struct {
    float turn;
    s32 alpha;
} L23Spin3;

extern s32 D_L23_0025E8B0[];

extern TI16 D_L23_002A75F0[];

extern TI16 D_L23_002A7170[];

extern TI16 D_L23_001BA640[];

extern void func_00291450(s32);

extern void func_00282D30(void *, void *, float);

extern void func_00282CC0(void *, void *, void *);

extern void func_00282CF0(void *, void *, void *);

extern void func_00282DD8(void *, void *, void *);

extern void func_00282F20(void *, void *, float);

extern float func_00282DF0(void *);

extern float func_002A7878(float, float);

extern s32 func_L00_002FCF18(void *, float, float, float, float, float);

extern long long func_0027C878(s32);

extern void func_002FC560(u32, u64);

extern void func_L03_002EFB60(void *, void *, float);

extern void func_002833F0(void *, void *);

extern void func_00283738(void *, void *, void *);

extern void func_002830C0(void *, void *, void *);

extern void func_00283098(void *, void *, void *);

extern void func_00280B90(void *, void *, s32);

/* releases the held handle, spawns a new one from jittered values, updates the globals, then
   draws one quad for every live entry of the 72-entry table, tilted by the spin state */
void func_L23_0042A438(u8 *moby) {
    L23Spin3 *vars;
    L23Quad pkt;
    struct { TI16 rows[4]; } matrix;
    TI16 *mp;
    float *basis;
    TI16 w[1];
    TI16 axis[1];
    TI16 rot[3];
    TI16 v120[1];
    TI16 v130[1];
    TI16 v140[1];
    TI16 v150[1];
    TI16 v160[1];
    float fa;
    float fb;
    float fc;
    float f;
    float t;
    float alphaScale;
    float scaledLength;
    float ribbonWidth;
    s32 i;
    u8 *hero;
    s64 reg;
    s64 tex;

    if (D_L23_001B1484_2 != 0) {
        if (D_L23_001B1B94 != -1) {
            func_00291450(D_L23_001B1B94);
        }
        func_00282D30(w, D_L23_001B1484_2 + 0xE0, 10.0f);
        func_00282CC0(w, w, D_L23_001B1484_2 + 0x10);
        f = func_002A7878(0.0f, D_L23_001B1C28);
        fa = D_L23_001B1C28 + f;
        f = func_002A7878(0.0f, D_L23_001B1C2C);
        fb = D_L23_001B1C2C + f;
        f = func_002A7878(0.0f, D_L23_001B1C30 * 0.5f);
        fc = D_L23_001B1C30 + f;
        D_L23_001B1B94 = func_L00_002FCF18(w, 20.0f, 0.0f, fa, fb, fc);
        D_L23_001B1C00 = D_L23_001B1C00 - D_L23_001B1C04;
        D_L23_001B39D0 = D_L23_001B1470 * 0.94f;
        vars = *(L23Spin3 **)(moby + 0x68);
        if (D_L23_001B1C00 <= -6.0f) {
            D_L23_001B1C00 = D_L23_001B1C00 + 6.0f;
        }
        i = *(u16 *)(moby + 0x96) % (D_L23_001B1474 >> 2);
        vars->alpha = D_L23_001B1C24;
        reg = D_L23_001B1BAC | ((s64)D_L23_001B1BB0 << 2)
            | ((s64)D_L23_001B1BB4 << 4) | ((s64)D_L23_001B1BB8 << 6)
            | ((s64)vars->alpha << 32);
        pkt.tail[3] = reg;
        D_L23_001B1C18 = (float)vars->alpha * D_L23_001B1C1C;
        tex = func_0027C878(D_L23_001B1C14);
        pkt.tail[2] = 0xFF9000000260LL;
        pkt.tail[1] = tex;
        pkt.tail[0] = 0;
        func_002FC560(0x47, 0x53001);
        hero = D_L23_001B1484_2;
        qcopy_nc(&matrix.rows[0], hero + 0xC0);
        qcopy_nc(&matrix.rows[1], hero + 0xD0);
        qcopy_nc(&matrix.rows[2], hero + 0xE0);
        qzero_nc(&matrix.rows[3]);
        mp = matrix.rows;
        func_L03_002EFB60(mp, mp, D_L23_001B1C0C);
        qcopy(&matrix.rows[3], D_L23_001B1484_2 + 0x10);
        ((float *)&matrix.rows[3])[3] = 1.0f;
        basis = (float *)mp;
        qzero(axis);
        ((float *)axis)[2] = vars->turn;
        func_002833F0(rot, axis);
        func_00283738(basis, basis, rot);
        pkt.col[0] = D_L23_001B1BC4;
        pkt.col[2] = D_L23_001B1BC8;
        pkt.col[1] = D_L23_001B1BC4;
        pkt.col[3] = D_L23_001B1BC8;
        for (i = 0; i < 0x48; i++) {
            if (D_L23_0025E8B0[i] != 0) {
                alphaScale = D_L23_001B1C18;
                func_002830C0(v120, &D_L23_002A75F0[i], &matrix);
                func_00283098(v140, &D_L23_002A7170[i], mp);
                func_00282CF0(v150, v120, D_L23_001BA640);
                func_00282DD8(v160, v140, v150);
                func_00282F20(v160, v160, alphaScale);
                f = func_00282DF0(v150);
                scaledLength = f * D_L23_001B1C10;
                t = D_L23_001B3930;
                ribbonWidth = scaledLength * t;
                D_L23_001B3930 = t + D_L23_001B1C20;
                if (D_L23_001B3930 > 1.0f) {
                    D_L23_001B3930 = 1.0f;
                }
                func_00282D30(v130, v140, ribbonWidth);
                func_00282CC0(v130, v130, v120);
                func_00282CC0(&pkt.pos[0], v120, v160);
                func_00282CF0(&pkt.pos[1], v120, v160);
                func_00282CC0(&pkt.pos[2], v130, v160);
                func_00282CF0(&pkt.pos[3], v130, v160);
                pkt.uv[0][0] = D_L23_001B1C00;
                pkt.uv[0][1] = 0.0f;
                pkt.uv[1][0] = D_L23_001B1C00;
                pkt.uv[1][1] = 1.0f;
                pkt.uv[2][0] = D_L23_001B1C00 + 1.0f;
                pkt.uv[2][1] = 0.0f;
                pkt.uv[3][0] = D_L23_001B1C00 + 1.0f;
                pkt.uv[3][1] = 1.0f;
                func_00280B90(&pkt, 0, 0);
            }
        }
    }
}
