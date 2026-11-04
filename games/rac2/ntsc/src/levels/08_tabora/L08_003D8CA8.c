#include "common.h"
extern float func_00282E80(void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern float func_002A7878(float, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern char D_00189EA0[];
extern char *D_L08_001B29DC[] __attribute__((sda));
extern int func_002A77E0(int);
extern int func_002A8600(void *, int, int, int);
extern int func_002A86E8(void *, void *, int, int);
extern void func_L08_003D8AB0(char *);

void func_L08_003D8CA8(char *object) {
    float delta[4];
    char *other;
    char *state = *(char **)(object + 0x68);

    if (*(int *)(state + 0x44C) && func_00282E80(object + 0x10, D_00189EA0) < 8.0f) {
        func_00282CF0(delta, object + 0x10, D_00189EA0);
        delta[2] = 0;
        func_00282F20(delta, delta, 8.0f);
        func_00282CC0(state + 0x3F0, object + 0x10, delta);
    } else if (*(int *)(state + 0x254) != 2 && !func_002A77E0(2)) {
        *(float *)(state + 0x3F0) = func_00283180(func_002A7878(-3.1415927f, 3.1415927f)) * 3.0f;
        {
            float y = func_00283198(func_002A7878(-3.1415927f, 3.1415927f)) * 3.0f;
            *(float *)(state + 0x3F8) = 0;
            *(float *)(state + 0x3F4) = y;
            func_00282CC0(state + 0x3F0, state + 0x3F0, D_00189EA0);
        }
    } else {
        *(float *)(state + 0x3F0) = func_00283180(func_002A7878(-3.1415927f, 3.1415927f)) * 8.0f;
        {
            float y = func_00283198(func_002A7878(-3.1415927f, 3.1415927f)) * 8.0f;
            *(float *)(state + 0x3F8) = 0;
            *(float *)(state + 0x3F4) = y;
            func_00282CC0(state + 0x3F0, state + 0x3F0, object + 0x10);
        }
    }
    other = 0;
    func_002A8600(&other, (unsigned char)object[0x21], 0, 0);
    while (other) {
        float distance = func_00282E80(object + 0x10, other + 0x10);
        if (distance < 3.0f) {
            func_00282CF0(delta, object + 0x10, other + 0x10);
            func_00282F20(delta, delta, 4.0f / distance);
            func_00282CC0(state + 0x3F0, state + 0x3F0, delta);
        }
        func_002A86E8(&other, other, 0, 0);
    }
    if (*(int *)(state + 0x468) != -1) {
        char *mount = D_L08_001B29DC[0] + (*(int *)(state + 0x468) << 8);
        if (mount && (unsigned char)mount[0x20] != 0xFE && (unsigned char)mount[0x20] != 0xFD &&
            (func_00282E80(object + 0x10, mount + 0x10) < 3.6f ||
             func_00282E80(state + 0x3F0, D_L08_001B29DC[0] + (*(int *)(state + 0x468) << 8) + 0x10) < 3.6f)) {
            func_00282CF0(delta, object + 0x10, D_L08_001B29DC[0] + (*(int *)(state + 0x468) << 8) + 0x10);
            delta[2] = 0;
            func_00282F20(delta, delta, 5.0f);
            func_00282CC0(state + 0x3F0, delta, object + 0x10);
        }
    }
    *(float *)(state + 0x3F8) = *(float *)(object + 0x18);
    func_L08_003D8AB0(object);
}
