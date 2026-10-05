/* NON_MATCHING func_L00_002B0D30 -- src/overlays/shared/vendor_002AB910.c
 * Best so far: BYTES 62/548 (88.7% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Debris moby update: applies gravity, then state 0 bounces off the ground (reflect, random spin) and state 1 fa
 *   Tried const globals, switch vs if, struct-typed moby/data (in_struct stores), operand order: same bytes. Likel
 *   Wave lb1 p10: ported Lombyte's FUN_L00_002afa48 (p8.c: u128 old + qcopy, else-if/&& chain, structs with aligne
 *   Only diff: retail's gp-rel global loads (-0x57E8/E4 before the two rnd calls, -0x57BC in state 1) are schedule
 *   Would unblock: a way to declare a 4-byte global gp-relative at -G2 that does not alias stores (none found); no
 */
typedef u32 u128_B0D30 __attribute__((mode(TI), aligned(16)));
typedef struct {
    u8 pad0[4];
    f32 f4;
    f32 f8;
    s16 hC;
    s16 hE;
    f32 vel[4] __attribute__((aligned(16)));
    f32 f20, f24, f28;
} PV_2afa48;
typedef struct {
    u8 pad0[0x10];
    f32 pos[4] __attribute__((aligned(16)));
    u8 b20, b21, b22, b23;
    u8 pad24[8];
    f32 f2c;
    u8 pad30[0x10];
    f32 f40, f44, f48;
    u8 pad4c[0x2C];
    PV_2afa48 *pv;
} M_2afa48;
extern f32 D_0015EE70 MACRO_ADDR;
extern f32 D_0015EE6C MACRO_ADDR;
extern short D_L00_00161518;
extern short D_L00_0016151C;
extern short D_L00_00161540;
extern short D_L00_00161544;
extern u128_B0D30 D_L00_00173F80[];
extern void func_001F9BD8(void *, void *, void *);
extern f32 func_001FA748(f32, f32);
extern s32 func_001F9938(void *);
extern s32 func_L00_001F10E0(void *, s32, s32, f32);
extern f32 func_001F9C78(void *, void *);
extern void func_L00_001FF610(void *, void *, void *);
extern void func_001F9C30(void *, void *, f32);
extern f32 func_L00_00258C80(f32, f32);
extern void func_0020D678(void *);
extern float D_0015EE6C_B0D30b __asm__("D_0015EE6C") MACRO_ADDR;

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/unclassified_002aa670.c, FUN_L00_002afa48. */
void func_L00_002B0D30(M_2afa48 *m) {
    PV_2afa48 *pv = m->pv;
    u128_B0D30 old;
    u128_B0D30 *t;
    qcopy(&old, m->pos);
    pv->vel[2] -= D_0015EE70 * 20.0f;
    func_001F9BD8(m->pos, m->pos, pv->vel);
    m->f40 = func_001FA748(m->f40, pv->f20);
    m->f44 = func_001FA748(m->f44, pv->f24);
    m->f48 = func_001FA748(m->f48, pv->f28);
    switch (m->b20) {
    case 0:
        if (m->pos[2] < pv->f4 - 3.0f) {
            m->b20 = 1;
        } else if (func_001F9938(&pv->hE) && pv->hC != 0 && func_L00_001F10E0(m->pos, 2, 0, pv->f8)) {
            t = D_L00_00173F80;
            if (func_001F9C78(pv->vel, t) < 0.0f) {
                qcopy(m->pos, t - 1);
                pv->hC--;
                func_L00_001FF610(pv->vel, pv->vel, t);
                func_001F9C30(pv->vel, pv->vel, 0.75f);
                pv->f20 = 0.0f;
                pv->f24 = func_L00_00258C80((*(f32 *)&D_L00_00161518), (*(f32 *)&D_L00_0016151C)) * 0.0174532924f * D_0015EE6C_B0D30b;
                pv->f28 = func_L00_00258C80((*(f32 *)&D_L00_00161518), (*(f32 *)&D_L00_0016151C)) * 0.0174532924f * D_0015EE6C_B0D30b;
            }
        }
        break;
    case 1:
        m->f2c *= (*(f32 *)&D_L00_00161540);
        if (m->b23 < (*(s32 *)&D_L00_00161544)) {
            func_0020D678(m);
        } else {
            m->b23 -= (*(s32 *)&D_L00_00161544);
        }
        break;
    }
}
