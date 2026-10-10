/* NON_MATCHING func_L05_00317B68 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: BYTES 7/368 (98.1% of the bytes match), checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   UpdateMoby_895: state machine (0 init: set flag, rotate angle via func_001FA790/748 by p[0]; 1 wait on level t
 *   Best p1.c (49 diff words, same size 368): all logic matches; only scheduling differs: retail loads p[2] before
 *   Tried ternary arg, local f, reordered b/d locals, struct-typed wording, index orderings: same bytes or worse. 
 *   mini61: Corrected staged data layout: target at 0x08, angle at 0x0c. Mixed float/pointer easing signature from
 */
#include "common.h"
extern float func_001FA790(float, float);
extern float func_001FA748(float, float);

typedef struct { char pad[0xBC]; unsigned char flag; char pad2[0x43]; } RotorPoolMoby17B68;
extern unsigned char *D_L05_00160098_pool __asm__("D_L05_00160098") NOT_SDA;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

typedef struct { int flag; float velocity; int target; float angle; } RotorData17B68;
typedef struct { char pad[0x20]; unsigned char state; char pad1[0x13]; unsigned short flags; char pad2[0x12]; float rot; char pad3[0x2c]; RotorData17B68 *data; } RotorMoby17B68;

/* UpdateMoby_895 */
void func_L05_00317B68(RotorMoby17B68 *moby) {
    RotorData17B68 *p = moby->data;
    float f;
    switch (moby->state) {
    case 0:
        if (p->flag != 0) {
            moby->flags |= 0x8000;
        }
        moby->state = 1;
        p->angle = moby->rot;
        if (p->flag != 0) {
            moby->rot = func_001FA790(moby->rot, 1.5707964f);
        } else {
            moby->rot = func_001FA748(moby->rot, 1.5707964f);
        }
        /* fallthrough */
    case 1:
        if (D_L05_00160098_pool[p->target * 0x100 + 0xBC] != 0) {
            func_0022ED80_i(0, 0, moby);
            moby->state = 2;
        }
        break;
    case 2:
        if (p->flag != 0) {
            f = func_001FA790(p->angle, 0.6981317f);
        } else {
            f = func_001FA748(p->angle, 0.6981317f);
        }
        func_L00_0025CE58(&moby->rot, f, &p->velocity, D_0015EE70 * 12.566371f, D_0015EE70 * 12.566371f, D_0015EE6C * 12.566371f);
        break;
    }
}
