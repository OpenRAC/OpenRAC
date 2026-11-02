#include "include_asm.h"

/* func_002C3150: spawns a particle at position with velocity; mode 1 keeps it
   relative to the hero. Its own unit for the grouped lq/sq profile. */

typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    unsigned char unk00;
    unsigned char unk01;
    unsigned char unk02;
    unsigned char unk03;
    int unk04;
    unsigned char unk08;
    unsigned char unk09;
    short unk0A;
    float unk0C;
    TI128 position;
    struct {
        TI128 velocity;
        float offset[3];
        int mode;
    } motion;
} Particle_002C3150;

extern Particle_002C3150 *func_002B9030(int);
extern int func_00283CF0(float);
typedef struct {
    char pad00[0x80];
    float position[3];
} HeroView_002C3150;

extern char D_00189E20[];
extern unsigned char *D_001F2614;

Particle_002C3150 *func_002C3150(TI128 *position, TI128 *velocity, int arg2,
                                 unsigned char arg3, unsigned char arg4, int mode, float scale)
{
    Particle_002C3150 *p = func_002B9030(0x3C);
    typeof(p->motion) *m;

    if (p != 0) {
        m = &p->motion;
        p->position = *position;
        m->velocity = *velocity;
        m->mode = mode;
        if (mode == 1) {
            m->offset[0] = ((float *)&p->position)[0] - ((HeroView_002C3150 *)D_00189E20)->position[0];
            m->offset[1] = ((float *)&p->position)[1] - ((HeroView_002C3150 *)D_00189E20)->position[1];
            m->offset[2] = ((float *)&p->position)[2] - ((HeroView_002C3150 *)D_00189E20)->position[2];
        }
        p->unk04 = arg2;
        p->unk09 = func_00283CF0(1.0f) + 0x20;
        p->unk03 = 0x48;
        p->unk08 = arg4;
        p->unk01 = 0;
        p->unk0C = scale * 210000.0f;
        p->unk02 = *D_001F2614;
        p->unk0A = arg3;
    }
    return p;
}
