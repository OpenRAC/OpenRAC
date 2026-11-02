#include "include_asm.h"
#include "common.h"

typedef int Quadword __attribute__((mode(TI)));
typedef union { Quadword bits; float f[4]; } Vector4;
typedef struct {
    Vector4 point[4];
    unsigned int color[4];
    float uv[8];
    unsigned long tag, tex, alpha, test;
} Beam;

extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282F20(void *, void *, float);
extern void func_00282DD8(void *, void *, void *);
extern void func_002830C0(void *, void *, void *);
extern unsigned long func_0027C878(int);
extern void func_00280B90(void *, void *, int);
extern void func_L05_0041D1B0(void *);
extern Vector4 D_L05_001B96C0;

/* The func_L05_0037D7D8 strip with this level's own setup helper: one strip from start
   to end, one colour, a caller texture and each end's basis taken from that end. */
void func_L05_0041D218(Vector4 *start, Vector4 *end, unsigned int color, int texture, float width)
{
    Beam strip;
    Vector4 basis[4], size;

    strip.tex = func_0027C878(texture);
    size.bits = 0;
    strip.alpha = 0xFF9000000260UL;
    strip.test = 0x8000000048UL;
    strip.color[3] = strip.color[2] = strip.color[1] = strip.color[0] = color;
    size.f[2] = width;
    strip.tag = 0;
    size.f[3] = 1;
    func_L05_0041D1B0(&strip);
    strip.uv[0] = 0; strip.uv[1] = 0; strip.uv[2] = 0; strip.uv[3] = 1;
    strip.uv[4] = 1; strip.uv[5] = 0; strip.uv[6] = 1; strip.uv[7] = 1;
    func_00282CF0(&basis[0], end, start); func_00282F20(&basis[0], &basis[0], 1);
    func_00282CF0(&basis[1], &D_L05_001B96C0, start); func_00282DD8(&basis[2], &basis[1], &basis[0]);
    func_00282F20(&basis[2], &basis[2], 1); func_00282DD8(&basis[1], &basis[0], &basis[2]);
    basis[3].bits = start->bits;
    strip.point[0].bits = size.bits;
    func_00282D30(&strip.point[1], &strip, -1);
    func_002830C0(&strip, &strip, basis); func_002830C0(&strip.point[1], &strip.point[1], basis);
    func_00282CF0(&basis[0], end, start); func_00282F20(&basis[0], &basis[0], 1);
    func_00282CF0(&basis[1], &D_L05_001B96C0, end); func_00282DD8(&basis[2], &basis[1], &basis[0]);
    func_00282F20(&basis[2], &basis[2], 1); func_00282DD8(&basis[1], &basis[0], &basis[2]);
    basis[3].bits = end->bits;
    strip.point[2].bits = size.bits;
    func_00282D30(&strip.point[3], &strip.point[2], -1);
    func_002830C0(&strip.point[2], &strip.point[2], basis); func_002830C0(&strip.point[3], &strip.point[3], basis);
    func_00280B90(&strip, 0, 0);
}
