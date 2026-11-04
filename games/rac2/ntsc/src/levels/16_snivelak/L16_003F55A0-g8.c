#include "include_asm.h"
typedef int Quadword __attribute__((mode(TI)));
extern int D_L16_001B10E0 __attribute__((sda));
extern float func_002A78D8(float, float);
extern float func_002A7878(float, float);
extern float func_002A7940(void);
extern float func_00282C48(float);
extern void func_00282D30(void *, void *, float);
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_L16_003C3A10(void *, void *, void *);

/* Spawns a bounded set of random vectors outside the central rectangle. */
void func_L16_003F55A0(char *object)
{
    float position[4];
    float random[4];
    float direction[4];
    int i;
    float scale;
    for (i = 0; i < D_L16_001B10E0; ++i) {
        *(Quadword *)random = 0;
        random[0] = func_002A78D8(0.0f, 26.0f);
        random[1] = func_002A78D8(0.0f, 8.0f);
        random[2] = func_002A7878(0.0f, 30.0f);
        *(Quadword *)position = *(Quadword *)random;
        if (func_00282C48(position[0]) < 9.0f && func_00282C48(position[2]) < 25.0f)
            continue;
        scale = *(float *)(object + 44) / *(float *)(*(char **)(object + 36) + 36);
        func_00282D30(position, position, scale);
        func_00283098(position, position, object + 192);
        func_00282CC0(position, position, object + 16);
        *(Quadword *)direction = 0;
        direction[0] = func_002A7940();
        direction[1] = func_002A7940();
        direction[2] = func_002A7940();
        *(Quadword *)random = *(Quadword *)direction;
        func_L16_003C3A10(object, position, random);
    }
}

