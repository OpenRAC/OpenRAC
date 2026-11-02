/* GPT-6.1 Sol high, 29 October 2026, task v0921.
 * Current pinned toolchain, USA v1.01: 932/932 bytes, 0/2 places;
 * first difference +0xD4 in levels 18 and 20. Profile:
 * -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
 * The full frame, initial quadword copies, basis construction, vertex loop,
 * opacity/customUV registers, color stores and custom-UV clamp loop now agree.
 * Remaining: the hoisted U/V base initializations are reversed; custom-UV setup
 * puts the f2 copy after the pointer moves instead of in the beqz delay slot.
 * The projected V update lacks retail's daddu v1,s1 and stores through s1;
 * its following U-clamp load is scheduled before the 1.0 literal, adding a nop
 * that keeps the complete function at the expected size.
 * A local customUV iterator fixes the older opacity register differences;
 * a local Y value plus ternary assignment fixes projected branch orientation.
 * Explicit V destination pointers introduce another induction variable or
 * reuse a call pointer at +4, rather than reproducing the retail v1 copy.
 * -g and -fno-rerun-loop-opt did not improve their tested source forms.
 * Fourteen bounded checks; no linked build. Reopen with an independently
 * matched UV projection/compound-assignment witness that preserves this copy.
 * Required declarations and packet layout follow below.
 */

#include "common.h"
typedef int VectorBits __attribute__((mode(TI)));
typedef union { VectorBits bits; float f[4]; } LensVector;
typedef struct { float u, v; } TexCoord;
typedef struct { LensVector vertex[4]; u32 colors[4]; TexCoord uv[4]; s64 primitive, texture, mode, flags; } QuadPacket;
extern float D_0018A0E0[4];
extern float D_L18_001AA250[4][4];
extern void func_00282DD8(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00281DE8(QuadPacket *, s32, s32, s32);
extern void func_00280B90(void *, void *, s32);
extern void func_0027C768(s32, float *, float *);
void func_L18_00307C60(float size, float spread, float distance, LensVector *position, LensVector *direction, s32 opacity, float **customUV)
{
    QuadPacket packet;
    LensVector basis[4], offset;
    float *source;
    s32 i;
    LensVector *vertex;
    basis[3].bits = position->bits;
    basis[0].bits = direction->bits;
    func_00282DD8(&basis[1], &basis[0], D_0018A0E0);
    func_00282F20(&basis[1], &basis[1], 1.0f);
    func_00282DD8(&basis[2], &basis[1], &basis[0]);
    func_00282D30(&offset, &basis[0], distance);
    func_00282CC0(&basis[3], &basis[3], &offset);
    source = D_L18_001AA250[0];
    vertex = packet.vertex;
    for (i = 3; i >= 0; i--) {
        func_00282D30(vertex, source, size);
        source += 4;
        func_002830C0(vertex, vertex, basis);
        vertex++;
    }
    func_00281DE8(&packet, 60, 5, 128);
    packet.colors[0] = packet.colors[1] = packet.colors[2] = packet.colors[3] = ((u32)opacity << 24) | 0x808080;
    packet.uv[0].u = 0.0f; packet.uv[0].v = 0.0f;
    packet.uv[1].u = 0.0f; packet.uv[1].v = 1.0f;
    packet.uv[2].u = 1.0f; packet.uv[2].v = 0.0f;
    packet.uv[3].u = 1.0f; packet.uv[3].v = 1.0f;
    func_00280B90(&packet, 0, 0);
    func_00281DE8(&packet, -1, 6, 128);
    func_00282D30(&offset, &basis[0], distance + 0.01f);
    if (customUV) {
        float *v = &packet.uv[0].v;
        float **uv = customUV;
        float *u = &packet.uv[0].u;
        for (i = 3; i >= 0; i--) {
            *u = (*uv)[0];
            *v = (*uv)[1];
            if (*u > 1.0f) *u = 1.0f;
            else if (*u < 0.0f) *u = 0.0f;
            if (*v > 1.0f) *v = 1.0f;
            else if (*v < 0.0f) *v = 0.0f;
            v += 2;
            uv++;
            u += 2;
        }
    } else {
        for (i = 0; i < 4; i++) {
            func_0027C768((s32)&packet.vertex[i], &packet.uv[i].u, &packet.uv[i].v);
            if (i >> 1) packet.uv[i].u -= spread;
            else packet.uv[i].u += spread;
            {
                float y = packet.uv[i].v;
                packet.uv[i].v = (i & 1) ? y + spread : y - spread;
            }
            if (packet.uv[i].u > 1.0f) packet.uv[i].u = 1.0f;
            else if (packet.uv[i].u < 0.0f) packet.uv[i].u = 0.0f;
            if (packet.uv[i].v > 1.0f) packet.uv[i].v = 1.0f;
            else if (packet.uv[i].v < 0.0f) packet.uv[i].v = 0.0f;
        }
    }
    func_00280B90(&packet, 0, 0);
}
