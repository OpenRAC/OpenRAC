#include "include_asm.h"

extern float func_002A7878(float, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern void func_00282CC0(void *, void *, void *);
extern float func_002A7940(void);
extern int func_002A7820(int, int);
extern void func_L01_0032EF38(void *, int, int, float, float, int);

/* The func_L01_003BD410 puffs, `count` of them around the state's +0x380 point. */
void func_L01_003DBB88(char *object, int count)
{
    float position[4];
    float angle, radius, first, second;
    char *state;
    int life, i;

    state = *(char **)(object + 0x68);
    for (i = 0; i < count; i++) {
        angle = func_002A7940();
        radius = func_002A7878(0.0f, 0.6f);
        first = func_002A7878(0.0f, 1.5f);
        second = func_002A7878(1.5f, 5.5f);
        life = func_002A7820(100, 160);
        position[0] = func_00283180(angle) * radius;
        position[1] = func_00283198(angle) * radius;
        position[2] = 0.0f;
        func_00282CC0(position, position, state + 0x380);
        func_L01_0032EF38(position, 0x30305020, 0x182810, first, second, life);
    }
}
