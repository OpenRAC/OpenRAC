#include "include_asm.h"
typedef unsigned char u8;
typedef unsigned long u64;
typedef int TI128 __attribute__((mode(TI)));
typedef struct { u8 pad[0x70]; long f70, f78, f80, f88; } QuadPacket;
extern void func_00281DE8(QuadPacket *, int, int, int);
typedef union { TI128 bits; float f[4]; } UiVector;
typedef struct {
    UiVector vertices[4];
    unsigned int colors[4];
    float uv[4][2];
    u64 primitive, texture, state, flags;
} UiDrawPacket;
extern float D_001B5340[];
extern float D_0018A0E0[];
extern UiVector D_001A9FD0[];
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282DD8(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00280B90(void *, int, int);
extern void func_0027C768(int, float *, float *);

/* Draws a camera-facing quad, then redraws it with texture coordinates from its projected
   corners. Assigning through value leaves one store after the join, its address computed
   before the branch: that is retail's copy of the y cursor. */
void func_002820D0(UiVector *position, int alpha, float **uv,
                   float size, float inset, float distance)
{
    UiDrawPacket packet;
    UiVector matrix[4], offset;
    int i;
    unsigned int color;
    matrix[3].bits = position->bits;
    func_00282CF0(&matrix[0], D_001B5340, &matrix[3]);
    func_00282F20(&matrix[0], &matrix[0], 1.0f);
    func_00282DD8(&matrix[1], &matrix[0], D_0018A0E0);
    func_00282F20(&matrix[1], &matrix[1], 1.0f);
    func_00282DD8(&matrix[2], &matrix[1], &matrix[0]);
    func_00282D30(&offset, &matrix[0], distance);
    func_00282CC0(&matrix[3], &matrix[3], &offset);
    for (i = 3; i >= 0; i--) {
        int index = 3 - i;
        func_00282D30(&packet.vertices[index], D_001A9FD0 + index, size);
        func_002830C0(&packet.vertices[index], &packet.vertices[index], matrix);
    }
    func_00281DE8((QuadPacket *)&packet, 59, 5, 128);
    color = (alpha << 24) | 0x808080;
    packet.uv[0][0] = 0.0f;
    packet.colors[0] = packet.colors[1] = packet.colors[2] = packet.colors[3] = color;
    packet.uv[0][1] = 0.0f;
    packet.uv[1][0] = 0.0f;
    packet.uv[1][1] = 1.0f;
    packet.uv[2][0] = 1.0f;
    packet.uv[2][1] = 0.0f;
    packet.uv[3][0] = 1.0f;
    packet.uv[3][1] = 1.0f;
    func_00280B90(&packet, 0, 0);
    func_00281DE8((QuadPacket *)&packet, -1, 6, 128);
    func_00282D30(&offset, &matrix[0], distance + 0.01f);
    if (uv != 0) {
        for (i = 3; i >= 0; i--) {
            int index = 3 - i;
            packet.uv[index][0] = uv[index][0];
            packet.uv[index][1] = uv[index][1];
            if (packet.uv[index][0] > 1.0f)
                packet.uv[index][0] = 1.0f;
            else if (packet.uv[index][0] < 0.0f)
                packet.uv[index][0] = 0.0f;
            if (packet.uv[index][1] > 1.0f)
                packet.uv[index][1] = 1.0f;
            else if (packet.uv[index][1] < 0.0f)
                packet.uv[index][1] = 0.0f;
        }
    } else {
        for (i = 0; i < 4; i++) {
            func_0027C768((int)&packet.vertices[i], &packet.uv[i][0], &packet.uv[i][1]);
            if (i >> 1)
                packet.uv[i][0] -= inset;
            else
                packet.uv[i][0] += inset;
            {
                float value = packet.uv[i][1];
                packet.uv[i][1] = value = (i & 1) ? value - inset : value + inset;
            }
            if (packet.uv[i][0] > 1.0f)
                packet.uv[i][0] = 1.0f;
            else if (packet.uv[i][0] < 0.0f)
                packet.uv[i][0] = 0.0f;
            if (packet.uv[i][1] > 1.0f)
                packet.uv[i][1] = 1.0f;
            else if (packet.uv[i][1] < 0.0f)
                packet.uv[i][1] = 0.0f;
        }
    }
    func_00280B90(&packet, 0, 0);
}
