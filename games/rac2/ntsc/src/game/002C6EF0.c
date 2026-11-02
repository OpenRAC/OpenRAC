#include "include_asm.h"
#include "common.h"

typedef int TI __attribute__((mode(TI)));

typedef struct {
    u8 f0;
    u8 f1;
    u8 shade;
    u8 f3;
    u32 color;
    u8 life;
    u8 f9;
    s16 age;
    u32 color2;
    f32 pos[4];
    f32 prev[4];
    f32 vel[4];
} Spark2C6EF0;

extern u8 *D_001F26A8[];

extern s32 func_00282978(s16 *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern int func_002A77E0(int);
extern void func_002B9180(void *);

void func_002C6EF0(Spark2C6EF0 *e) {
    f32 old[4] __attribute__((aligned(16)));
    f32 step[4] __attribute__((aligned(16)));
    f32 *vel = e->vel;
    f32 *pos;
    f32 t;
    u32 a;

    if (func_00282978(&e->age)) {
        func_002B9180(e);
        return;
    }
    pos = e->pos;
    e->shade = D_001F26A8[0][func_002A77E0(2)];
    qcopy(old, pos);
    func_00282CC0(pos, pos, vel);
    func_00282CF0(step, pos, old);
    func_00282F20(step, step, 0.9f);
    func_00282CF0(e->prev, pos, step);
    e->prev[3] = 2.0f;
    vel[2] = vel[2] - vel[3];
    if (e->age < e->life) {
        t = func_00283CE0(e->age);
        t = t / func_00283CE0(e->life);
        a = func_00283CF0(t * 127.0f) << 24;
        e->color = a | (e->color & 0xFFFFFF);
        e->color2 = a | (e->color2 & 0xFFFFFF);
    }
}
