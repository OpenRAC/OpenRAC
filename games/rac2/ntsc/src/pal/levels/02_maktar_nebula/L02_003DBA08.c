#include "include_asm.h"

extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern float func_00283248(float, float);
extern int func_00283CF0(float);
extern void func_L02_003D2248(char *, int);
extern void func_L02_003F17B0(char *, int);
extern void func_L02_0040B868(char *, int);
extern void func_L02_0040D080(char *, int);
typedef struct {
    char pad0[0x80];
    float x;
    float y;
} L02_003D7900_Hero;
extern L02_003D7900_Hero D_00189E20;

/* Builds a four-float vector and hands it to the two vector helpers, tints the three colour
   records of the actor's state from it, then walks the three actors the state points at:
   each gets the angle to the hero written at 0xF8 and the behaviour its type selects. */
void func_PAL_L02_003DBA08(char *arg0) {
    float v[4];
    char *state = *(char **)(arg0 + 0x68);
    char *obj;
    int i;
    int count;

    v[0] = 0.0f;
    v[1] = -9.0f;
    v[2] = -3.3f;
    v[3] = 0.0f;
    func_00283098(v, v, arg0 + 0xC0);
    func_00282CC0(v, v, arg0 + 0x10);
    if (*(char **)(state + 0x10) != 0) {
        *(float *)(*(char **)(state + 0x10) + 0x10) = v[0] - 2.0f;
        *(float *)(*(char **)(state + 0x10) + 0x14) = v[1];
        *(float *)(*(char **)(state + 0x10) + 0x18) = v[2];
        *(float *)(*(char **)(state + 0x10) + 0x1C) = v[3];
    }
    if (*(char **)(state + 0x14) != 0) {
        *(float *)(*(char **)(state + 0x14) + 0x10) = v[0];
        *(float *)(*(char **)(state + 0x14) + 0x14) = v[1] + 1.0f;
        *(float *)(*(char **)(state + 0x14) + 0x18) = v[2];
        *(float *)(*(char **)(state + 0x14) + 0x1C) = v[3];
    }
    if (*(char **)(state + 0x18) != 0) {
        *(float *)(*(char **)(state + 0x18) + 0x10) = v[0];
        *(float *)(*(char **)(state + 0x18) + 0x14) = v[1] - 1.0f;
        *(float *)(*(char **)(state + 0x18) + 0x18) = v[2];
        *(float *)(*(char **)(state + 0x18) + 0x1C) = v[3];
    }
    obj = *(char **)(state + 0x10);
    for (count = 0, i = 0; i < 3; i++) {
        L02_003D7900_Hero *hero = &D_00189E20;

        if (i == 1) {
            obj = *(char **)(state + 0x14);
        }
        if (i == 2) {
            obj = *(char **)(state + 0x18);
        }
        if (obj == 0) {
            continue;
        }
        *(int *)(obj + 0xF0) = 0;
        *(int *)(obj + 0xF4) = 0;
        *(float *)(obj + 0xF8) =
            func_00283248(hero->x - *(float *)(obj + 0x10),
                          hero->y - *(float *)(obj + 0x14));
        *(int *)(obj + 0xFC) = 0;
        switch (*(short *)(obj + 0xAA)) {
        case 0x9CA:
            func_L02_003D2248(obj, 0);
            break;
        case 0xB22:
            func_L02_003F17B0(obj, 0);
            break;
        case 0xC3B:
            func_L02_0040B868(obj, 0);
            break;
        case 0xC3E:
            *(float *)(obj + 0x18) = *(float *)(obj + 0x18) + 0.5f;
            func_L02_0040D080(obj, func_00283CF0((float)count * 0.16666667f));
            break;
        }
    }
    *(int *)(state + 0x10) = 0;
    *(int *)(state + 0x14) = 0;
    *(int *)(state + 0x18) = 0;
}

