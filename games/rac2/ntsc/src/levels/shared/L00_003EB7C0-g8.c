#include "include_asm.h"

typedef int OrbitQuad __attribute__((mode(TI)));
typedef union { OrbitQuad q; float v[4]; } OrbitVector;

extern char D_00189E20[] __attribute__((nosda));
extern float func_00283CE0(int value);
extern float func_00283180(float angle);
extern float func_00283198(float angle);
extern void func_00282F20(void *out, void *vector, float scale);
extern void func_00282CC0(void *out, void *left, void *right);

void func_L00_003EB7C0(void *unused, char *position)
{
    OrbitVector temporary;
    float scale = 1.2f;
    float angle = func_00283CE0(*(int *)(position + 0x38));
    char *base;
    char *axis0, *axis1, *axis2;
    float wave;
    wave = func_00283CE0(3);
    wave = 3.1415927f / wave;
    angle *= wave;
    base = D_00189E20;
    axis0 = *(char **)(base + 0x2290) + 0xd0;
    wave = func_00283180(angle);
    func_00282F20(&temporary, axis0, wave * scale);
    func_00282CC0(position, &temporary, base + 0x80);
    axis1 = *(char **)(base + 0x2290) + 0xe0;
    wave = func_00283198(angle);
    func_00282F20(&temporary, axis1, wave * scale + 2.2f);
    func_00282CC0(position, position, &temporary);
    axis2 = *(char **)(base + 0x2290) + 0xc0;
    wave = func_00283198(angle);
    func_00282F20(&temporary, axis2, wave + wave);
    func_00282CC0(position, position, &temporary);
}
