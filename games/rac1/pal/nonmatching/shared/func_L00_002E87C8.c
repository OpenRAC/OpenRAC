/* NON_MATCHING func_L00_002E87C8 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: BYTES 15/516 (97.1% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E87C8 (camera stick probe): up to 3 iterations of a probe loop, then func_L00_002E85E8 and a clamp
 *   What worked: `char *g = D_L00_00166F10;` base pointer; `while (i<3)` with `if (A) flag1=1; else { if (!B) brea
 *   Left: prologue schedule only. Retail loads D_L00_00161DA8 ($f4), then -0x10($19), then r[0x48] ($22), with 1.0
 */
typedef unsigned int Q002e7318 __attribute__((mode(TI)));
typedef union V002e7318 { f32 f[4]; Q002e7318 q; } V002e7318;
typedef struct X002e7318 {
    u8 pad[0x20];
    V002e7318 m;
    u8 pad30[8];
    s16 s38;
    u8 pad3a[0xe];
    s32 i48;
} X002e7318;
typedef struct P002e7318 { u8 pad[0x20c]; f32 a; f32 b; f32 c; } P002e7318;
typedef struct O002e7318 { u8 pad[0x70]; u8 *p70; } O002e7318;
typedef struct G002e7318 {
    u8 pad[0x30];
    V002e7318 v40;
    u8 pad50[0x84];
    s32 i114;
} G002e7318;
extern G002e7318 D_L00_00166F10_E87C8 __asm__("D_L00_00166F10");
extern short D_L00_00161DA8;
extern V002e7318 D_L00_00173F60_E87C8 __asm__("D_L00_00173F60");
extern V002e7318 D_L00_00173F70_E87C8 __asm__("D_L00_00173F70");
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0_E87C8(void *, void *, void *) __asm__("func_001F9BF0");
extern void func_L00_001FF4B0(void *, void *, f32);
extern f32 func_001F9C78(void *, void *);
extern s32 func_L00_002E5630_E87C8(V002e7318 *, f32) __asm__("func_L00_002E5630");
extern s32 func_L00_001F10E0_E87C8(V002e7318 *, f32, s32, s32) __asm__("func_L00_001F10E0");

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/overlays/shared/gameplay_vendor_002df730.c, FUN_L00_002e7318. */
s32 func_L00_002E87C8(O002e7318 *o)
{
    V002e7318 pos;
    V002e7318 tmp;
    u8 *b = o->p70;
    X002e7318 *x = (X002e7318 *)(b + 0x1D0);
    u8 *b40 = b + 0x40;
    u8 *b130 = b + 0x130;
    P002e7318 *p;
    f32 lim;
    f32 d;
    s32 w;
    s32 i;
    s32 ha = 0;
    s32 hb = 0;
    s32 r;
    G002e7318 *g = &D_L00_00166F10_E87C8;

    func_001F9BD8(&pos, b + 0x90, b130);
    x->s38 = 2;
    p = (P002e7318 *)((O002e7318 **)g)[-4]->p70;
    w = x->i48;
    lim = (*(f32 *)&D_L00_00161DA8) + p->c * p->a * (p->b - 1.0f);
    i = 0;
    while (i < 3) {
        if (func_L00_002E5630_E87C8(&pos, lim)) {
            ha = 1;
        } else {
            if (!func_L00_001F10E0_E87C8(&pos, lim, w, g->i114)) break;
            hb = 1;
        }
        if (x->s38 != 0) {
            func_001F9BF0_E87C8(&tmp, &D_L00_00173F60_E87C8, &pos);
            func_L00_001FF4B0(&tmp, &tmp, 1.0f);
            d = func_001F9C78(&g->v40, &tmp);
            if (d > 0.75f) x->s38 = 1;
            else if (d < -0.75f) x->s38 = -1;
            else x->s38 = 0;
        }
        qcopy(&pos, &D_L00_00173F70_E87C8);
        i++;
    }
    r = func_L00_002E85E8(o, &pos, ha, hb);
    func_001F9BD8(&x->m, b40 + 0x50, b130);
    if (x->s38 == 2) x->s38 = 0;
    return r;
}
