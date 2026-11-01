#include "include_asm.h"

/* Level counterpart of func_00327D50; the vector clear takes one register,
   as in the matched boot unit. */

typedef unsigned int RenderQuad __attribute__((mode(TI)));
typedef union { RenderQuad word; float v[4]; } RenderVector;
typedef struct { float x, y, z, w; } RenderVec __attribute__((aligned(16)));

typedef struct {
    RenderVec vertex[4];
    unsigned int color[4];
    float uv[4][2];
    long long tag[4];
} RenderPacket;

extern long long func_0027C878(int);
extern void func_00282CF0(void *, const void *, const void *);
extern void func_00282F20(void *, const void *, float);
extern void func_00282DD8(void *, const void *, const void *);
extern void func_00282D30(void *, const void *, float);
extern void func_002830C0(void *, const void *, const void *);
extern void func_0027C768(void *, float *, float *);
extern float func_00283180(float);
extern float func_00283198(float);
extern float func_002A7798(float);
extern float func_00283B98(float, float);
extern void func_00280B90(void *, void *, int);
extern char D_L00_001B8DC0[];
extern int D_L00_001B04F4 __attribute__((sda));
extern int D_L00_001B04F8 __attribute__((sda));
extern int D_L00_001B04FC __attribute__((sda));
extern int D_L00_001B0500 __attribute__((sda));
extern int D_L00_001B0504 __attribute__((sda));
extern int D_L00_001B0508 __attribute__((sda));
extern unsigned int D_L00_001B050C __attribute__((sda));
extern float D_L00_001B0434 __attribute__((sda));
extern short D_L00_002967A0[][4][2];
extern RenderVec D_L00_002961A0[];
extern float D_L00_002474F0[][2];

void func_L00_003DF338(char *object)
{
    RenderPacket packet;
    RenderVec matrix[4];
    RenderVector up;
    char *state;
    int face, corner, index, vtx;
    long long flags;
    float *u, *v, *uv;
    float delta, scale, c, s;

    state = *(char **)(object + 0x68);
    up.word = 0;
    up.v[2] = 1.0f;
    packet.tag[1] = func_0027C878(D_L00_001B0508);
    packet.tag[2] = 0xFF9000000260LL;
    flags = (long long)D_L00_001B04F4;
    flags |= (long long)D_L00_001B04F8 << 2;
    flags |= (long long)D_L00_001B04FC << 4;
    flags |= (long long)D_L00_001B0500 << 6;
    flags |= (long long)D_L00_001B0504 << 32;
    packet.tag[3] = flags;
    packet.tag[0] = 0;
    func_00282CF0(&matrix[0], D_L00_001B8DC0, object + 0x10);
    func_00282F20(&matrix[0], &matrix[0], 1.0f);
    func_00282DD8(&matrix[1], &matrix[0], &up);
    func_00282F20(&matrix[1], &matrix[1], 1.0f);
    func_00282DD8(&matrix[2], &matrix[1], &matrix[0]);
    matrix[3].x = *(float *)(object + 0x10);
    matrix[3].y = *(float *)(object + 0x14);
    matrix[3].z = *(float *)(object + 0x18);
    matrix[3].w = 1.0f;
    packet.color[3] = D_L00_001B050C;
    packet.color[2] = D_L00_001B050C;
    packet.color[1] = D_L00_001B050C;
    packet.color[0] = D_L00_001B050C;
    uv = &packet.uv[0][0];
    for (face = 0; face < 48; face++) {
        v = &packet.uv[0][1];
        u = uv;
        for (corner = 0; corner < 4; corner++) {
            vtx = D_L00_002967A0[face][corner][0];
            index = D_L00_002967A0[face][corner][1];
            func_00282D30(&packet.vertex[corner], &D_L00_002961A0[vtx], *(float *)(state + 0x24) * 2.0f);
            func_002830C0(&packet.vertex[corner], &packet.vertex[corner], matrix);
            func_0027C768(&packet.vertex[corner], &packet.uv[corner][0], &packet.uv[corner][1]);
            c = func_00283180(D_L00_002474F0[index][0]);
            delta = c * func_002A7798((1.0f - *(float *)(state + 0x24) / D_L00_001B0434) * 0.0125f);
            *u += delta;
            s = func_00283198(D_L00_002474F0[index][0]);
            scale = func_002A7798((1.0f - *(float *)(state + 0x24) / D_L00_001B0434) * 0.0125f);
            delta = -s;
            delta *= scale;
            *v += delta;
            if (*u > 1.0f) *u = 1.0f;
            else if (*u < 0.0f) *u = 0.0f;
            if (*v > 1.0f) *v = 1.0f;
            else if (*v < 0.0f) *v = 0.0f;
            v += 2;
            u += 2;
        }
        func_00280B90(&packet, 0, 0);
    }
    for (face = 0; face < 98; face++)
        D_L00_002474F0[face][0] = func_00283B98(D_L00_002474F0[face][0], D_L00_002474F0[face][1]);
}
