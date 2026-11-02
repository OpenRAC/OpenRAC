#include "include_asm.h"
extern void func_00282F20(void *, void *, float);
typedef struct {
    float direction[4];
    char *owner;
    float scale;
    int timer;
    int duration;
    float phase;
    float zero;
    float sign;
    float one;
} ParticleE3;
extern ParticleE3 *D_001B2754 __attribute__((section(".data")));
extern float func_002A7878(float, float);
extern int func_00282948(void *);
extern int func_002A7820(int, int);
extern int func_001163B0(void);
void func_0031E3A8(char *object)
{
    int i;
    ParticleE3 *p;
    for (i = 0; i < 64; i++) {
        p = (ParticleE3 *)(i * 48 + (unsigned int)D_001B2754);
        if (p->owner != object) continue;
        {
            float decrement = func_002A7878(0.01f, 0.075f);
            p->phase -= decrement;
            if (p->phase <= -8.0f) p->phase += 8.0f;
        }
        if (func_00282948(&p->timer)) {
            p->direction[0] = func_002A7878(-1.0f, 1.0f);
            p->direction[1] = func_002A7878(-1.0f, 1.0f);
            p->direction[2] = func_002A7878(-1.0f, 1.0f);
            p->zero = 0;
            p->duration = p->timer = func_002A7820(20, 50);
            p->sign = func_001163B0() & 1 ? -1.0f : 1.0f;
            p->one = 1.0f;
            func_00282F20(p, p, 1.0f);
            p->scale = func_002A7878(0.7f, 0.9f);
        }
    }
}

