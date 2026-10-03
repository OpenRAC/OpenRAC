/* NON_MATCHING func_L05_00317B68 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: BYTES 49/368 (86.7% of the bytes match), checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   UpdateMoby_895: state machine (0 init: set flag, rotate angle via func_001FA790/748 by p[0]; 1 wait on level t
 *   Best p1.c (49 diff words, same size 368): all logic matches; only scheduling differs: retail loads p[2] before
 *   Tried ternary arg, local f, reordered b/d locals, struct-typed wording, index orderings: same bytes or worse. 
 */
#include "common.h"
extern float func_001FA790(float, float);
extern float func_001FA748(float, float);
extern int func_0022ED80(int, int, int);
extern float func_L00_0025CE58(float *p, float *v, float a, float b, float c, float d);
typedef struct { char pad[0xBC]; unsigned char flag; char pad2[0x43]; } Ent;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

typedef struct { int flag; float pos[3]; int ent; } Data;
typedef struct { char pad[0x20]; unsigned char state; char pad1[0x13]; unsigned short flags; char pad2[0x12]; float rot; char pad3[0x2c]; Data *data; } Moby;

/* UpdateMoby_895 */
void func_L05_00317B68(Moby *moby) {
    Data *p = moby->data;
    float f;
    switch (moby->state) {
    case 0:
        if (p->flag != 0) {
            moby->flags |= 0x8000;
        }
        moby->state = 1;
        p->pos[2] = moby->rot;
        if (p->flag != 0) {
            moby->rot = func_001FA790(moby->rot, 1.5707964f);
        } else {
            moby->rot = func_001FA748(moby->rot, 1.5707964f);
        }
        /* fallthrough */
    case 1:
        if (D_L05_00160098[p->ent].flag != 0) {
            func_0022ED80(0, 0, (int)moby);
            moby->state = 2;
        }
        break;
    case 2:
        if (p->flag != 0) {
            f = func_001FA790(p->pos[2], 0.6981317f);
        } else {
            f = func_001FA748(p->pos[2], 0.6981317f);
        }
        func_L00_0025CE58(&moby->rot, p->pos, f, D_0015EE70 * 12.566371f, D_0015EE70 * 12.566371f, D_0015EE6C * 12.566371f);
        break;
    }
}
