#include "include_asm.h"
typedef struct { float x, y, z, w; } Vector;
typedef struct { Vector vertices[4]; unsigned int colors[4]; float uv[8]; long zero, texture, state, flags; } Packet;
typedef struct {
    short current, end, active, unused06;
    float first, second, opacity;
    int unused14, effect, flash, unused20, unused24, unused28, unused2C, unused30, unused34, visible;
} Context;
typedef struct { unsigned char prefix[0x150]; short width, height; } Screen;
extern Screen D_001A6480;
extern int D_001A79F0 __attribute__((sda));
extern Vector D_L00_001B8DC0;
extern void func_002FC560(unsigned int, unsigned long long);
extern void func_0027DE50(int, int, int, int, unsigned long);
extern void ref_L05_003FE430_00(void *, float, float, float, float, float, int, int);
extern void func_002AFFD8(char *, char *, int);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern void func_00281DE8(void *, int, int, int);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282DD8(void *, void *, void *);
extern void func_00280B90(void *, void *, int);

static inline float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

void func_L05_003FE430(char *object)
{
    Packet packet;
    Vector up, difference, unit, center, horizontal, vertical, corner;
    Context *context = *(Context **)(object + 0x68);
    float amount, opacity, width;
    int delta;
    unsigned int color;

    if (context->end - context->current < 2 && context->active != 0 && context->flash != 0) {
        func_002FC560(0x42, 0x48);
        func_0027DE50(0, D_001A6480.height, 0, D_001A6480.width, 0xFFFFFFFF80808080UL);
    }
    if (context->effect != 0) {
        delta = context->end - context->current;
        if (delta < 10) {
            amount = lerp(context->first, context->second, (float)delta * 0.1f);
            ref_L05_003FE430_00(object + 0x10, amount, 0.5f, 0.0f, 0.0f, 0.0f, 60, 0);
        } else if ((unsigned short)(context->current - 46) < 4) {
            amount = lerp(context->first, context->second, (float)(context->current - 45) * 0.1f);
            ref_L05_003FE430_00(object + 0x10, amount, 0.5f, 0.0f, 0.0f, 0.0f, 60, 0);
        }
    }
    if (context->active != 0 && D_001A79F0 != 13 && D_001A79F0 != 16 && context->visible != 0) {
        width = 0.075f;
        func_002AFFD8(object + 0x10, (char *)&up, 1);
        opacity = func_00283CE0(context->active);
        opacity *= context->opacity;
        func_00281DE8(&packet, 11, 4, 0x80);
        color = ((unsigned int)func_00283CF0(opacity * 255.0f) << 24) | 0xFFFFFFu;
        packet.colors[3] = color;
        packet.colors[2] = color;
        packet.colors[1] = color;
        packet.colors[0] = color;
        packet.uv[0] = 0.0f;
        packet.uv[1] = 0.0f;
        packet.uv[2] = 0.0f;
        packet.uv[3] = 1.0f;
        packet.uv[4] = 1.0f;
        packet.uv[5] = 0.0f;
        packet.uv[6] = 1.0f;
        packet.uv[7] = 1.0f;
        func_00282CF0(&difference, object + 0x10, &D_L00_001B8DC0);
        func_00282F20(&unit, &difference, 0.1f);
        func_00282CC0(&center, &D_L00_001B8DC0, &unit);
        func_00282DD8(&horizontal, &difference, &up);
        func_00282F20(&horizontal, &horizontal, width);
        func_00282DD8(&vertical, &horizontal, &difference);
        func_00282F20(&vertical, &vertical, width);
        func_00282CC0(&corner, &center, &vertical);
        func_00282CC0(&packet.vertices[0], &corner, &horizontal);
        func_00282CF0(&packet.vertices[1], &corner, &horizontal);
        func_00282CF0(&corner, &center, &vertical);
        func_00282CC0(&packet.vertices[2], &corner, &horizontal);
        func_00282CF0(&packet.vertices[3], &corner, &horizontal);
        func_00280B90(&packet, 0, 0);
    }
}
