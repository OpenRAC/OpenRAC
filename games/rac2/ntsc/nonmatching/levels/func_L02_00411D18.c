/* Claude Opus 5.5 (xhigh), 31 October 2026: recompiled this C unchanged in a
 * dedicated -O2 -G0 -ffunction-sections -g -fno-gcse unit; still 1880/1928 bytes,
 * 0/1 places in USA v1.01. Compiler blocker in the prologue: retail saves the
 * quadword-held $s2 (it carries right.vertex[3] to sq 0x110) in a 16-byte slot
 * at the bottom of the GPR area, sq $s2,0x620 then sd $s0,0x630, $s1,0x638,
 * $s3..$ra 0x640..0x670, and only $f20/$f21 (0x680/0x688). This compiler gives
 * every GPR an 8-byte slot in ascending order, and a quadword-held one gets
 * sq into its 8-byte slot (overlapping the next): a two-line test gives
 * sq $16,0($sp); sd $17,8($sp), and this draft gives sq $s7,0x658 with
 * sd $fp,0x660. No C can then place s2 at 0x620 and s0 at 0x630. The 0x690 frame
 * here agrees only because this draft saves a third FPR ($f22); with retail's two
 * this compiler would give 0x680. Retail's only other game prologues mixing sq and
 * sd saves are func_L01_00324550 and func_L08_00389B68 (varargs, unmatched).
 * Reopen only with a compiler that sizes quadword-saved GPR slots as retail does.
 *
 * GPT-6.1 Sol (high), 29 October 2026: 1880/1928 bytes, 0/1 places
 * in USA v1.01, USA v2.00 and PAL. No linked build was run.
 * Profile: -O2 -G0 -ffunction-sections -g -fno-gcse.
 * SDA declarations for the two 32-bit texture globals recover retail's repeated
 * absolute lui/lw pairs instead of hoisting their addresses across calls.
 * The 0x690 frame agrees. Initial twelve TI loads/save order, context copy
 * addressing, FP constant lifetimes and later cursor spills still differ.
 * This supersedes the 1864-byte draft only for the measured global-load fix.
 * Keeping quad source pointers from builder calls through the second loop
 * gave 1896 bytes when initialized at entry, 1872 when initialized at first
 * use. Explicit transform/expansion pointers before the UV loop gave 1908
 * bytes but changed retail's direct stack stores into indirect stores; size
 * alone is no improvement. Keeping those stores direct gave 1884 bytes.
 * These lifetime trials remain 0/1 and are not retained over the simpler fix.
 * Earlier aggregate/all-helper and flag sweeps remain exhausted: preserve
 * ordinary lowering for the initial twelve loads; no repeated flag sweep.
 * Reopen with independent cursor/constant lifetime or copy-API evidence.
 * Complete rendering-path semantics have not been independently verified.
 * Required declarations follow below.
 */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { TI128 quad; float f[4]; } EffectVector;
typedef struct { EffectVector vertex[4]; } EffectQuad;
typedef struct { float uv[8]; } EffectUV;
typedef struct { u32 word[5]; } EffectPalette;
/* The builder and renderer walk 0x90-byte packets, including the last header word. */
typedef struct {
    EffectVector vertex[4];
    u8 header[0x10];
    float uv[4][2];
    u8 tail[0x20];
} EffectPacket;
extern EffectUV D_L02_001B1AE0;
extern EffectUV D_L02_001B1B00;
extern EffectPalette D_L02_001B1B20;
extern EffectQuad D_L02_001B1B40;
extern EffectQuad D_L02_001B1B80;
extern EffectQuad D_L02_001B1BC0;
extern u32 D_L02_001B1C00 __attribute__((sda));
extern u32 D_L02_001B1C04 __attribute__((sda));
extern EffectVector D_L02_001BA500;
extern void func_00283658(void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CF0(void *, void *, void *);
extern void func_002AFDD8(char *, char *, char *, char *);
extern void func_00282D30(void *, void *, float);
extern float func_00283198(float);
extern u32 func_00283D38(u32, u32, float);
extern void func_L02_002EF968(EffectPacket *, u32, u32, float *, EffectVector *, s32);
extern void *func_00115484(void *, s32, u32);
extern void func_002833F0(void *, void *);
extern void func_L01_002E1510(EffectPacket *, s32, void *, s32, void *);
extern void func_00282CC0(void *, void *, void *);

/* Render seven related quads with scrolling UVs and two expanding wave pairs. */
void func_L02_00411D18(u8 *object)
{
    EffectUV primary_uv = D_L02_001B1AE0;
    EffectUV secondary_uv = D_L02_001B1B00;
    EffectPalette palette = D_L02_001B1B20;
    EffectQuad primary;
    EffectQuad left;
    EffectQuad right;
    EffectVector context[4];
    EffectVector direction;
    EffectPacket packets[7];
    u32 wave_color[2];
    EffectVector transform_parameters[2];
    EffectVector expansion[2];
    EffectVector zero_expansion[2];
    EffectVector matrix[3];
    u8 *state = *(u8 **)(object + 0x68);
    u32 primary_color;
    u32 secondary_color;
    float phase;
    float fade;
    s32 i;

    primary.vertex[0].quad = D_L02_001B1B40.vertex[0].quad;
    primary.vertex[1].quad = D_L02_001B1B40.vertex[1].quad;
    primary.vertex[2].quad = D_L02_001B1B40.vertex[2].quad;
    primary.vertex[3].quad = D_L02_001B1B40.vertex[3].quad;
    left.vertex[0].quad = D_L02_001B1B80.vertex[0].quad;
    left.vertex[1].quad = D_L02_001B1B80.vertex[1].quad;
    left.vertex[2].quad = D_L02_001B1B80.vertex[2].quad;
    left.vertex[3].quad = D_L02_001B1B80.vertex[3].quad;
    right.vertex[0].quad = D_L02_001B1BC0.vertex[0].quad;
    right.vertex[1].quad = D_L02_001B1BC0.vertex[1].quad;
    right.vertex[2].quad = D_L02_001B1BC0.vertex[2].quad;
    right.vertex[3].quad = D_L02_001B1BC0.vertex[3].quad;
    func_00283658(context, object + 0xC0);
    context[3].quad = *(TI128 *)(object + 0x10);
    context[3].f[3] = 1.0f;
    func_00282F20(context, context, 4.0f);
    func_00282F20(context + 1, context + 1, 10.0f);
    func_00282CF0(&direction, &D_L02_001BA500, object + 0x10);
    func_002AFDD8((char *)object, (char *)&direction, (char *)&direction, 0);
    if (direction.f[0] < 0.0f)
        func_00282D30(context, context, -1.0f);
    phase = func_00283198(*(float *)(state + 0x2C)) * 0.5f + 0.5f;
    primary_color = func_00283D38(palette.word[0], palette.word[1], phase);
    secondary_color = func_00283D38(palette.word[2], palette.word[3], phase);
    fade = *(float *)(state + 0x20);
    if (!(fade >= 0.2f))
        fade = 1.0f - fade * 5.0f;
    wave_color[0] = func_00283D38(palette.word[3], palette.word[3] & 0xFFFFFF, fade);
    fade = *(float *)(state + 0x24);
    if (!(fade >= 0.2f))
        fade = 1.0f - fade * 5.0f;
    wave_color[1] = func_00283D38(palette.word[3], palette.word[3] & 0xFFFFFF, fade);
    if (*(float *)(state + 0x1C) < 1.0f) {
        primary_color = func_00283D38(primary_color & 0xFFFFFF, primary_color, *(float *)(state + 0x1C));
        secondary_color = func_00283D38(secondary_color & 0xFFFFFF, secondary_color, *(float *)(state + 0x1C));
        wave_color[0] = func_00283D38(wave_color[0] & 0xFFFFFF, wave_color[0], *(float *)(state + 0x1C));
        wave_color[1] = func_00283D38(wave_color[1] & 0xFFFFFF, wave_color[1], *(float *)(state + 0x1C));
    }
    func_L02_002EF968(packets, D_L02_001B1C00, primary_color, primary_uv.uv, primary.vertex, 1);
    func_L02_002EF968(packets + 1, D_L02_001B1C04, secondary_color, secondary_uv.uv, left.vertex, 1);
    func_L02_002EF968(packets + 2, D_L02_001B1C04, secondary_color, secondary_uv.uv, right.vertex, 1);
    func_L02_002EF968(packets + 3, D_L02_001B1C04, wave_color[0], secondary_uv.uv, left.vertex, 1);
    func_L02_002EF968(packets + 4, D_L02_001B1C04, wave_color[0], secondary_uv.uv, right.vertex, 1);
    func_L02_002EF968(packets + 5, D_L02_001B1C04, wave_color[1], secondary_uv.uv, left.vertex, 1);
    func_L02_002EF968(packets + 6, D_L02_001B1C04, wave_color[1], secondary_uv.uv, right.vertex, 1);
    i = 3;
    {
        float *uv1 = packets[1].uv[0];
        float *uv2 = packets[2].uv[0];
        float *uv3 = packets[3].uv[0];
        float *uv4 = packets[4].uv[0];
        float *uv5 = packets[5].uv[0];
        float *uv6 = packets[6].uv[0];
        u8 *vertex = (u8 *)packets;
        do {
            *uv1 += *(float *)(state + 0x28); uv1 += 2;
            *uv2 += *(float *)(state + 0x28); uv2 += 2;
            *uv3 += *(float *)(state + 0x28); uv3 += 2;
            *uv4 += *(float *)(state + 0x28); uv4 += 2;
            *uv5 += *(float *)(state + 0x28); uv5 += 2;
            *uv6 += *(float *)(state + 0x28); uv6 += 2;
            *(float *)(vertex + 0x1B8) -= *(float *)(state + 0x20) + *(float *)(state + 0x20);
            *(float *)(vertex + 0x248) += *(float *)(state + 0x20) + *(float *)(state + 0x20);
            *(float *)(vertex + 0x2D8) -= *(float *)(state + 0x24) + *(float *)(state + 0x24);
            *(float *)(vertex + 0x368) += *(float *)(state + 0x24) + *(float *)(state + 0x24);
            vertex += 0x10;
        } while (--i >= 0);
    }
    transform_parameters[0].quad = 0;
    transform_parameters[1].quad = 0;
    transform_parameters[0].f[2] = -0.19634955f;
    transform_parameters[1].f[2] = 6.0f;
    func_00115484(zero_expansion, 0, sizeof(zero_expansion));
    zero_expansion[0].f[2] = *(float *)(state + 0x20) + *(float *)(state + 0x20);
    zero_expansion[1].f[2] = *(float *)(state + 0x24) + *(float *)(state + 0x24);
    expansion[0].quad = zero_expansion[0].quad;
    expansion[1].quad = zero_expansion[1].quad;
    func_002833F0(matrix, transform_parameters);
    func_L01_002E1510(packets, 7, matrix, 16, context);
    i = 3;
    {
        EffectVector *v0 = packets[0].vertex;
        EffectVector *v1 = packets[1].vertex;
        EffectVector *v2 = packets[2].vertex;
        EffectVector *v3 = packets[3].vertex;
        EffectVector *v4 = packets[4].vertex;
        EffectVector *v5 = packets[5].vertex;
        EffectVector *v6 = packets[6].vertex;
        EffectVector *p0 = primary.vertex;
        EffectVector *p1 = left.vertex;
        EffectVector *p2 = right.vertex;
        do {
            func_00282CC0(v0++, p0++, transform_parameters + 1);
            func_00282CC0(v1++, p1, transform_parameters + 1);
            func_00282CC0(v2++, p2, transform_parameters + 1);
            func_00282CC0(v3, p1, transform_parameters + 1);
            func_00282CC0(v4, p2, transform_parameters + 1);
            func_00282CC0(v5, p1++, transform_parameters + 1);
            func_00282CC0(v6, p2++, transform_parameters + 1);
            func_00282CF0(v3, v3, expansion); v3++;
            func_00282CC0(v4, v4, expansion); v4++;
            func_00282CF0(v5, v5, expansion + 1); v5++;
            func_00282CC0(v6, v6, expansion + 1); v6++;
        } while (--i >= 0);
    }
    func_002833F0(matrix, transform_parameters);
    func_L01_002E1510(packets, 7, matrix, 16, context);
}
