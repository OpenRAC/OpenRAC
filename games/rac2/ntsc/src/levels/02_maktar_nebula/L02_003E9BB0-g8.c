#include "common.h"
typedef int Quad __attribute__((mode(TI)));
typedef union { Quad quad; float f[4]; } Vector;
typedef struct { Vector vertex[4]; u32 color[4]; float uv[8]; u64 primitive, texture, mode, flags; } RibbonPacket;
extern char D_L02_001BA500[];
extern u64 func_0027C878(int);
extern void func_00280B90(void *, void *, int), func_002FC560(int, u64);
extern void func_00282CC0(void *, void *, void *), func_00282CF0(void *, void *, void *);
extern void func_00282DD8(void *, void *, void *), func_00282D30(void *, void *, float);
extern float func_00282DF0(void *);
extern float D_L02_001B0C68 __attribute__((sda)), D_L02_001B0C80 __attribute__((sda));
extern int D_L02_001B0CBC __attribute__((sda)), D_L02_001B0CC0 __attribute__((sda));
extern u32 D_L02_001B0C70 __attribute__((sda)), D_L02_001B0C74 __attribute__((sda));
/* Draws the narrow ribbon and its camera-facing halo along the point chain.
 * The EE ABI independently orders integer and FP arguments: count is a3,
 * unused is f12, width is f13, and extend is f14. */
void func_L02_003E9BB0(Vector *points, u32 color, u32 outerColor, float unused, float width, float extend, int count) {
    RibbonPacket core, halo;
    Vector normal, widthVector, eye, tangent, first, last, firstRay, lastRay, toEye, extension;
    float length, initialLength, haloLength, firstLength, lastLength;
    int i;
    u32 coreColor, haloColor;
    core.texture = func_0027C878(D_L02_001B0CBC);
    coreColor = color;
    core.flags = 0x8000000048ULL;
    core.mode = 0xFF9000000260ULL;
    core.primitive = 0;
    core.color[3] = coreColor; core.color[2] = coreColor;
    core.color[1] = coreColor; core.color[0] = coreColor;
    halo.texture = func_0027C878(D_L02_001B0CC0);
    halo.flags = core.flags; halo.mode = core.mode; halo.primitive = core.primitive;
    haloColor = outerColor;
    halo.color[3] = haloColor; halo.color[2] = haloColor;
    halo.color[1] = haloColor; halo.color[0] = haloColor;
    halo.uv[0] = 0; halo.uv[1] = 0; halo.uv[2] = 0; halo.uv[3] = 1;
    halo.uv[4] = 1; halo.uv[5] = 0; halo.uv[6] = 1; halo.uv[7] = 1;
    core.uv[0] = D_L02_001B0C68; core.uv[1] = 0; core.uv[2] = D_L02_001B0C68; core.uv[3] = 1;
    core.uv[4] = D_L02_001B0C68 + 1.0f; core.uv[5] = 0; core.uv[6] = D_L02_001B0C68 + 1.0f; core.uv[7] = 1;
    func_00282CF0(&eye, points, D_L02_001BA500);
    func_00282CF0(&tangent, points + 1, points);
    func_00282DD8(&normal, &tangent, &eye);
    initialLength = func_00282DF0(&normal);
    if (initialLength != 0) initialLength = 1.0f / initialLength;
    func_00282D30(&normal, &normal, D_L02_001B0C80 * initialLength);
    func_00282CC0(&core.vertex[0], points, &normal);
    func_00282CF0(&core.vertex[1], points, &normal);
    for (i = 1; i < count - 1; i++) {
        func_00282CF0(&eye, points + i, D_L02_001BA500);
        func_00282CF0(&tangent, points + i + 1, points + i);
        func_00282DD8(&normal, &tangent, &eye);
        length = func_00282DF0(&normal);
        if (length != 0) length = 1.0f / length;
        func_00282D30(&normal, &normal, D_L02_001B0C80 * length);
        func_00282CC0(&core.vertex[2], points + i, &normal);
        func_00282CF0(&core.vertex[3], points + i, &normal);
        if (i == 1) { core.color[1] = D_L02_001B0C70; core.color[0] = D_L02_001B0C70; }
        else if (i == count - 2) { core.color[3] = D_L02_001B0C70; core.color[2] = D_L02_001B0C70; }
        else if (i == 2) { core.color[1] = coreColor; core.color[0] = coreColor; }
        func_00280B90(&core, 0, 0);
        qcopy(&core.vertex[0], &core.vertex[2]); qcopy(&core.vertex[1], &core.vertex[3]);
    }
    for (i = 0; i < count - 1; i++) {
        func_00282CF0(&firstRay, points + i, D_L02_001BA500);
        func_00282CF0(&lastRay, points + i + 1, D_L02_001BA500);
        firstLength = func_00282DF0(&firstRay);
        lastLength = func_00282DF0(&lastRay);
        if (lastLength < firstLength) {
            if (lastLength == 0) { qcopy(&lastRay, &firstRay); }
            else func_00282D30(&lastRay, &lastRay, firstLength / lastLength);
        } else if (firstLength == 0) { qcopy(&firstRay, &lastRay); }
        else func_00282D30(&firstRay, &firstRay, lastLength / firstLength);
        func_00282CC0(&first, D_L02_001BA500, &firstRay);
        func_00282CC0(&last, D_L02_001BA500, &lastRay);
        func_00282CF0(&tangent, &last, &first);
        func_00282D30(&extension, &tangent, extend);
        func_00282CF0(&first, &first, &extension);
        func_00282CC0(&last, &last, &extension);
        func_00282CF0(&toEye, D_L02_001BA500, &first);
        func_00282DD8(&normal, &toEye, &tangent);
        haloLength = func_00282DF0(&normal);
        if (haloLength != 0) haloLength = 1.0f / haloLength;
        func_00282D30(&widthVector, &normal, width * haloLength);
        func_00282CC0(&halo.vertex[0], &first, &widthVector);
        func_00282CF0(&halo.vertex[1], &first, &widthVector);
        func_00282CC0(&halo.vertex[2], &last, &widthVector);
        func_00282CF0(&halo.vertex[3], &last, &widthVector);
        if (i == 0) { halo.color[1] = D_L02_001B0C74; halo.color[0] = D_L02_001B0C74; }
        else if (i == count - 2) { halo.color[3] = D_L02_001B0C74; halo.color[2] = D_L02_001B0C74; }
        else if (i == 1) { halo.color[1] = haloColor; halo.color[0] = haloColor; }
        func_00280B90(&halo, 0, 0);
    }
    func_002FC560(0x47, 0x5360B);
}
