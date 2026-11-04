/* func_002E7338 - 568 bytes, 28 places (boot and 27 levels).
 * Tier opus-1, 28 October 2026, game -O2 -G8 -ffunction-sections (append to
 * src/game/002E7018-g8.c, or a unit of its own named *-g8.c).
 * check.py: 568/568 bytes, 0/28 places; only +0x110..+0x11c differ (four
 * instructions in the loop preheader). Body adapted from the sol-high draft;
 * recompiled on this toolchain.
 *
 * Retail:  blez v0,exit / lui v1,0x20 / li.s f21,0.025 / ori v1,v1,0x58B0 /
 *          sw v1,0xA8(sp) / lh a0,0xB2(s7) / nop (loop-label alignment)
 * Ours:    blez v0,exit / lui v1,0x20 / li.s f21,0.025 / sw v1,0xA8(sp) /
 *          ori v1,v1,0x58B0 / sw v1,0xA8(sp) / lh a0,0xB2(s7)
 *
 * What happens (RTL dumps): loop moves baseColor = 0x2058B0 into the
 * preheader; sched1's split_block_insns splits it into P = 0x200000 and
 * P = P | 0x58B0 before reload; P has no hard register (stack slot 0xA8,
 * slot order 0xA0 table, 0xA4 count, 0xA8 P, 0xAC &position, as in retail).
 * Reload gives the first set an output reload (v1, sw), then reloads P again
 * for the ior instead of inheriting v1: the reload load is emitted and later
 * deleted by reload_cse (NOTE_INSN_DELETED uid 480 in the greg dump), so
 * delete_output_reload never removes the first store. Retail's single store
 * is what inheritance plus delete_output_reload gives (or a single
 * unsplit set at reload time). Adjacent lui/ori (0.025f removed, a
 * diagnostic) still gives two stores, so the li.s between them is not the
 * cause. Tried without change: constant inline in the ior (stays in the
 * loop: 560 bytes), colour computed in the inner loop (stays in the outer
 * loop), operand order of the ior, color = c << 24; color |= base, the three
 * locals in a stack struct (constant then stored in the loop). The earlier
 * notes' other trials (baseColor across the random call, -fmove-all-movables)
 * also leave it.
 * Next: find why the ior's input reload of P does not inherit v1 (reload
 * debug output of choose_reload_regs for that insn), then the source shape
 * that lets it.
 *
 * The body corresponds to RAC1's DrawResidentEffectQuads (func_0022F4C8,
 * src/game/space.c) without its 0x215 colour case.
 */
#include "common.h"

typedef struct { float f[4]; } EffectRow;
typedef struct {
    float positions[4][4];
    unsigned int colors[4];
    float uv[4][2];
    unsigned long long reserved, texture, state, primitive;
} EffectQuad;
extern unsigned int D_001A7AF0 MACRO_ADDR;
extern int D_001AC040 __attribute__((sda));
extern EffectRow *D_001AC048 __attribute__((sda));
extern int D_001AC050 __attribute__((sda));
extern float D_001ABFF0 __attribute__((sda));
extern float D_001ABFF4[] MACRO_ADDR;
extern long func_0027C878(int);
extern void func_00282D30(void *, void *, float);
extern int func_002A77E0(int);
extern float func_00283CE0(int);
extern void func_00282CC0(void *, void *, void *);
extern void func_00283098(void *, void *, void *);
extern void func_00280B90(void *, int, int);

void func_002E7338(unsigned char *object)
{
    EffectQuad quad;
    EffectRow position;
    EffectRow *table;
    int count;
    int i, j;
    unsigned int baseColor;
    int variant = (D_001A7AF0 >> 4) & 1;
    table = (&D_001AC048)[variant];
    count = (&D_001AC040)[variant];
    quad.texture = func_0027C878(5);
    quad.state = 0xFF9000000260ULL;
    quad.primitive = 0x8000000048ULL;
    quad.reserved = 0;
    {
        float *vSource = D_001ABFF4;
        unsigned char *vDest = (unsigned char *)&quad + 0x54;
        unsigned char *uDest = (unsigned char *)&quad + 0x50;
        float *uSource = &D_001ABFF0;
        int k;
        for (k = 0; k < 4; k++) {
            *(float *)uDest = *uSource;
            *(float *)vDest = *vSource;
            uDest += 8;
            vDest += 8;
            uSource += 2;
            vSource += 2;
        }
    }
    func_00282D30(&position, object, 0.0009765625f);
    for (i = 0; i < count; i++) {
        int c = object[0xBC];
        unsigned int color;
        float scale;
        if (*(short *)(object + 0xB2)) c += func_002A77E0(*(short *)(object + 0xB2));
        baseColor = 0x2058B0;
        scale = func_00283CE0(c) * 0.025f * table[i].f[3];
        color = (c << 24) | baseColor;
        for (j = 0; j < 4; j++) {
            quad.colors[j] = color;
            func_00282D30(quad.positions[j], ((EffectRow *)&D_001AC050) + j, scale);
            func_00282CC0(quad.positions[j], quad.positions[j], &table[i]);
            func_00283098(quad.positions[j], quad.positions[j], object + 0xC0);
            func_00282CC0(quad.positions[j], quad.positions[j], &position);
        }
        func_00280B90(&quad, 0, 0);
    }
}
