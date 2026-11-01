#include "common.h"

typedef unsigned int QuadWord __attribute__((mode(TI)));
typedef union { QuadWord word; float v[4]; } Vector;
typedef struct {
    Vector vertex[4];
    unsigned int color[4];
    float uv[4][2];
    long f70, f78, f80, flags;
} RenderPacket;
extern char D_L00_001A8F20[] MACRO_ADDR;
extern char D_L00_001B9000[];
extern int func_00283CF0(float);
extern void func_00281DE8(void *, int, int, int);
extern void func_002FC560(unsigned int, unsigned long long);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282D30(void *, void *, float);
extern void func_00280B90(char *, int, int);

/* Debug line labels reproduce the measured assembler division padding. */
void func_L00_00419668(char *object)
{
    RenderPacket packet;
    Vector step, a, b, c, d, center, temp;
    char *state = *(char **)(object + 0x68);
    int count = *(int *)(state + 0x20);
    int i = 0;
    float delta, divisor;
    packet.color[3] = 0x08404040;
    packet.color[2] = 0x08404040;
    packet.color[1] = 0x08404040;
    packet.color[0] = 0x08404040;
    *(int *)&packet.uv[0][0] = 0;
    *(int *)&packet.uv[0][1] = 0;
    *(int *)&packet.uv[1][0] = 0;
    packet.uv[1][1] = 1.0f;
    packet.uv[2][0] = 1.0f;
    *(int *)&packet.uv[2][1] = 0;
    packet.uv[3][0] = 1.0f;
    packet.uv[3][1] = 1.0f;
    func_00281DE8(&packet, *(int *)(state + 0x48), 1, 0x80);
    packet.f70 = 0;
    func_002FC560(0x42, 0x8000000048ULL);
    func_002FC560(8, 0);
    func_002FC560(0x14, 0xFF9000000260ULL);
    func_002FC560(0x47, 0x513F1);
    func_00282CF0(&a, D_L00_001A8F20, D_L00_001B9000);
    func_00282CF0(&a, &a, D_L00_001B9000 + 0x10);
    func_00282CF0(&b, D_L00_001B9000 + 0x10, D_L00_001B9000);
    func_00282CF0(&c, D_L00_001B9000, D_L00_001B9000 + 0x10);
    func_00282CC0(&d, D_L00_001B9000, D_L00_001B9000 + 0x10);
    func_00282CF0(&center, D_L00_001B9000 - 0x240, state);
    func_00282F20(&center, &center, 0.75f);
    func_00282CC0(&center, state, &center);
    func_00282D30(&temp, &a, *(float *)(state + 0xC));
    func_00282CC0(&packet.vertex[0], &temp, &center);
    func_00282D30(&temp, &b, *(float *)(state + 0xC));
    func_00282CC0(&packet.vertex[1], &temp, &center);
    func_00282D30(&temp, &c, *(float *)(state + 0xC));
    func_00282CC0(&packet.vertex[2], &temp, &center);
    func_00282D30(&temp, &d, *(float *)(state + 0xC));
    func_00282CC0(&packet.vertex[3], &temp, &center);
    divisor = count - 1;
    delta = (*(float *)(state + 0x1C) - *(float *)(state + 0xC)) / divisor;
    func_00282CF0(&step, state + 0x10, state);
    func_00282D30(&step, &step, 1.0f / divisor);
    func_00282D30(&temp, &a, delta);
    func_00282CC0(&a, &temp, &step);
    func_00282D30(&temp, &b, delta);
    func_00282CC0(&b, &temp, &step);
    func_00282D30(&temp, &c, delta);
    func_00282CC0(&c, &temp, &step);
    func_00282D30(&temp, &d, delta);
    func_00282CC0(&d, &temp, &step);
    for (; i < count; i++) {
        int alpha;
        unsigned int color;
        func_00282CC0(&packet.vertex[0], &packet.vertex[0], &a);
        func_00282CC0(&packet.vertex[1], &packet.vertex[1], &b);
        func_00282CC0(&packet.vertex[2], &packet.vertex[2], &c);
        func_00282CC0(&packet.vertex[3], &packet.vertex[3], &d);
        alpha = func_00283CF0((float)*(int *)(state + 0x2C) * (float)(count - i) / (float)count);
        if (alpha > 255) alpha = 255;
        else if (alpha < 0) alpha = 0;
        color = *(unsigned int *)(state + 0x40) + (alpha << 24);
        packet.color[3] = color;
        packet.color[2] = color;
        packet.color[1] = color;
        packet.color[0] = color;
        if (alpha > 0) func_00280B90((char *)&packet, 0, 0);
    }
}
