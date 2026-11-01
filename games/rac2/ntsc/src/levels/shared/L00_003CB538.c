#include "include_asm.h"
typedef struct {
    float v[3];
    float scale;
    void *owner;
    float f14;
    int f18;
    int f1C;
    float f20;
    int f24;
    float f28;
    float f2C;
} Rec_3CB538;

extern Rec_3CB538 *D_L00_001B2AD4 __attribute__((section(".sdata")));
extern float func_002A7878(float, float);
extern int func_002A7820(int, int);
extern void func_00282F20(void *, void *, float);
extern int func_001163B0(void);

void func_L00_003CB538(void *owner)
{
    Rec_3CB538 *p;
    int i;

    for (i = 0; i < 64; i++) {
        p = &D_L00_001B2AD4[i];
        if (p->owner == 0) {
            p->v[0] = func_002A7878(-1.0f, 1.0f);
            p->v[1] = func_002A7878(-1.0f, 1.0f);
            p->v[2] = func_002A7878(-1.0f, 1.0f);
            func_00282F20(p, p, p->f14);
            p->f1C = p->f18 = func_002A7820(0x14, 0x32);
            p->f20 = func_002A7878(-1.0f, 1.0f);
            p->owner = owner;
            p->f24 = 0;
            p->f28 = (func_001163B0() & 1) ? -1.0f : 1.0f;
            p->f2C = 1.0f;
            p->f14 = func_002A7878(0.7f, 0.9f);
            return;
        }
    }
}
