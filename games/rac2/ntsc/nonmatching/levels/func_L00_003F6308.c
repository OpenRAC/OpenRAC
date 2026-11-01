/* Nonmatching C: Build the three ring layers and submit the surface and ribbon draw packets.
 * Tested profile: -O2 -G8 -ffunction-sections.
 * Measured US size: 1400 bytes; retail: 1524 bytes.
 * Placement checks: US, US2 and PAL: 0/27 each.
 * Remaining: Ring cursor spills, loop addressing, floating-point scheduling and pointer allocation differ.
 * Reopen with independent ring and packet cursor lifetimes; preserve all three twenty-vector rings, the complete frame and packet, and unsigned color shifts.
 */
#include "common.h"

typedef unsigned int QuadWord __attribute__((mode(TI)));
typedef union { QuadWord word; float v[4]; } Vector;
typedef struct { Vector row[3]; Vector position; } Frame;
typedef struct { unsigned char pad[0x70]; long f70, f78, f80, f88; } QuadPacket;
typedef struct {
    Vector vertex[4];
    unsigned int color[4];
    float uv[4][2];
    long f70, f78, f80, flags;
} RenderPacket;

extern Vector D_L00_001B8DC0;
extern void func_00281DE8(QuadPacket *, int, int, int);
extern void func_00282CF0(Vector *, const Vector *, const Vector *);
extern void func_00282CC0(Vector *, const Vector *, const Vector *);
extern void func_00282DD8(Vector *, const Vector *, const Vector *);
extern void func_00282F20(Vector *, const Vector *, float);
extern void func_00282D30(Vector *, const Vector *, float);
extern void func_002830C0(Vector *, const Vector *, const Frame *);
extern void func_00280B90(RenderPacket *, void *, int);

typedef struct {
    Vector center[3];
    Vector direction[2];
    short active, lifetime;
    float phase, ribbon_phase, fade;
} RingEffect;

extern char *func_L00_0032A088(int, int);
extern int *func_L00_0032A0B8(int, int);
extern int func_00283CF0(float);
extern Vector D_L00_0029F0D0[20];
extern float D_L00_001B097C __attribute__((sda)), D_L00_001B0980 __attribute__((sda)), D_L00_001B0984 __attribute__((sda));
extern unsigned int D_L00_001B098C __attribute__((sda)), D_L00_001B0990 __attribute__((sda));
extern int D_L00_001B0994 __attribute__((sda));
extern float D_L00_001B09A0 __attribute__((sda));
extern unsigned int D_L00_001B09A4 __attribute__((sda)), D_L00_001B09A8 __attribute__((sda));
extern int D_L00_001B09AC __attribute__((sda));
extern float D_L00_001B09B0 __attribute__((sda));
extern unsigned int D_L00_001B09B4 __attribute__((sda)), D_L00_001B09B8 __attribute__((sda));
extern int D_L00_001B09BC __attribute__((sda));

void func_L00_003F6308(char *object)
{
    RenderPacket packet;
    Frame frame;
    Vector rings[3][20];
    Vector temporary, side;
    int effect_index, layer, segment, next, count;
    char *state = *(char **)(object + 0x68);
    char *base;
    RingEffect *effects, *effect;
    unsigned int color, edge;
    float fade, u, v, v_next, phase;

    effect_index = 0;
    base = func_L00_0032A088(0xc04, *(short *)(state + 0x1a));
    effects = (RingEffect *)(base + *func_L00_0032A0B8(0xc04, *(short *)(state + 0x1a)));
    func_00281DE8((QuadPacket *)&packet, 0x15, 0, 0x80);
    do {
        effect = &effects[effect_index];
        if (effect->active != 0) {
            qcopy(&frame.row[0], &effect->direction[0]);
            qcopy(&frame.row[2], &effect->direction[1]);
            func_00282DD8(&frame.row[1], &frame.row[0], &frame.row[2]);
            fade = (float)effect->lifetime * effect->fade;
            color = ((u32)func_00283CF0(fade * (float)D_L00_001B0994) << 24) | D_L00_001B098C;
            packet.color[0] = color;
            packet.color[1] = color;
            packet.color[2] = color;
            packet.color[3] = color;
            for (layer = 0; layer < 3; ++layer) {
                Vector *circle = D_L00_0029F0D0;
                Vector *output = rings[layer];
                qcopy(&frame.position, &effect->center[layer]);
                count = 19;
                do {
                    func_00282D30(&temporary, circle, D_L00_001B0984);
                    ++circle;
                    --count;
                    func_002830C0(output, &temporary, &frame);
                    ++output;
                } while (count >= 0);
            }
            u = effect->phase;
            edge = D_L00_001B0990;
            for (layer = 0; layer < 2; ++layer) {
                v = 0.0f;
                for (segment = 0; segment < 20; ++segment) {
                    next = (segment + 1) % 20;
                    qcopy(&packet.vertex[0], &rings[layer + 1][segment]);
                    qcopy(&packet.vertex[1], &rings[layer + 1][next]);
                    qcopy(&packet.vertex[2], &rings[layer][segment]);
                    qcopy(&packet.vertex[3], &rings[layer][next]);
                    v_next = v + D_L00_001B0980;
                    packet.uv[0][0] = u;
                    packet.uv[0][1] = v;
                    packet.uv[1][0] = u;
                    packet.uv[1][1] = v_next;
                    packet.uv[2][0] = u + D_L00_001B097C;
                    packet.uv[2][1] = v;
                    packet.uv[3][0] = packet.uv[2][0];
                    packet.uv[3][1] = v_next;
                    packet.color[2] = edge;
                    packet.color[3] = edge;
                    func_00280B90(&packet, 0, 0);
                    v = v_next;
                }
                edge = color;
            }
            func_00281DE8((QuadPacket *)&packet, 0xe, 1, 0x80);
            color = ((u32)func_00283CF0(fade * (float)D_L00_001B09AC) << 24) | D_L00_001B09A8;
            edge = D_L00_001B09A4;
            packet.uv[0][0] = 0.0f;
            packet.uv[0][1] = 0.0f;
            packet.uv[1][0] = 0.0f;
            packet.uv[2][1] = 0.0f;
            packet.uv[3][0] = 0.5f;
            packet.uv[2][0] = 0.5f;
            packet.color[0] = color;
            packet.color[1] = color;
            packet.color[2] = color;
            packet.color[3] = color;
            packet.uv[1][1] = 1.0f;
            packet.uv[3][1] = 1.0f;
            func_00282CF0(&temporary, &D_L00_001B8DC0, &effect->center[0]);
            func_00282DD8(&frame.row[1], &frame.row[0], &temporary);
            func_00282F20(&side, &frame.row[1], D_L00_001B09A0);
            for (layer = 0; layer < 2; ++layer) {
                func_00282CC0(&packet.vertex[0], &effect->center[layer + 1], &side);
                func_00282CF0(&packet.vertex[1], &effect->center[layer + 1], &side);
                func_00282CC0(&packet.vertex[2], &effect->center[layer], &side);
                func_00282CF0(&packet.vertex[3], &effect->center[layer], &side);
                packet.color[2] = edge;
                packet.color[3] = edge;
                func_00280B90(&packet, 0, 0);
                packet.uv[0][0] = packet.uv[2][0];
                packet.uv[3][0] = 3.0f;
                packet.uv[1][0] = packet.uv[2][0];
                packet.uv[2][0] = 3.0f;
                edge = color;
            }
            func_00281DE8((QuadPacket *)&packet, 0x17, 1, 0x80);
            color = ((u32)func_00283CF0(fade * (float)D_L00_001B09BC) << 24) | D_L00_001B09B8;
            edge = D_L00_001B09B4;
            phase = effect->ribbon_phase;
            packet.uv[0][0] = 0.0f;
            packet.uv[0][1] = 0.0f;
            packet.uv[1][0] = 0.0f;
            packet.uv[2][1] = 0.0f;
            packet.uv[2][0] = phase + 0.5f;
            packet.color[0] = color;
            packet.color[1] = color;
            packet.color[2] = color;
            packet.color[3] = color;
            packet.uv[1][1] = 1.0f;
            packet.uv[3][0] = packet.uv[2][0];
            packet.uv[3][1] = 1.0f;
            func_00282F20(&side, &frame.row[1], D_L00_001B09B0);
            for (layer = 0; layer < 2; ++layer) {
                func_00282CC0(&packet.vertex[0], &effect->center[layer + 1], &side);
                func_00282CF0(&packet.vertex[1], &effect->center[layer + 1], &side);
                func_00282CC0(&packet.vertex[2], &effect->center[layer], &side);
                func_00282CF0(&packet.vertex[3], &effect->center[layer], &side);
                packet.color[2] = edge;
                packet.color[3] = edge;
                func_00280B90(&packet, 0, 0);
                packet.uv[3][0] = phase + 3.0f;
                packet.uv[0][0] = packet.uv[2][0];
                packet.uv[1][0] = packet.uv[2][0];
                edge = color;
                packet.uv[2][0] = packet.uv[3][0];
            }
        }
        ++effect_index;
    } while (effect_index < 3);
}
