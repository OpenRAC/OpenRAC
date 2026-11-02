/* Tier: opus-1 (v0922), USA v1.01. No strict match is claimed.
 * Best check.py, default profile -O2 -G0 -ffunction-sections (gcse on):
 * 792/772 bytes, 0 of 28 places. The same C with -fno-gcse: 776/772 bytes,
 * 0 of 28 places, about five aligned instructions off.
 * What still differs:
 * - gcse on: PRE makes the second p->origin (func_00282D08's second argument)
 *   redundant with the origin qcopy's destination address and keeps p + 0x20
 *   in a saved register across every call; retail recomputes addiu a1,s1,0x20.
 *   That one extra saved register shifts owner/pool-high to s6/s7 and grows the
 *   frame to 0x80. Everything else matches structurally, including the
 *   then-branch's s0 temp plus s3 position copy (position assigned after the
 *   func_00282CC0 call) and the else-branch qcopy.
 * - gcse off: retail's split pool address (lui s6 hoisted, addiu a0,s6,%lo in
 *   the loop) becomes a fully hoisted base (lui v0; addiu s6,v0) and a nop
 *   before the mult; nothing else differs.
 * Tried without success: array-indexed origin copy (reassociates to
 * base+0x20+i*0xA0 and hoists the full base), TI copy with -frac2-qcopy (same),
 * a second pointer copy (gcse cprop folds it back), -frac2-gcse-unsplit-edges,
 * -frac2-keep-block-order. Next: a source shape where the origin copy's
 * address and the later &p->origin are different expressions at gcse time but
 * the record base is still one register, or a gcse model that leaves the
 * second address computed.
 * Declarations needed (also used by func_00316400 in src/game/0030F7B0.c):
 */
#include "common.h"

typedef struct {
    float offset[4], rotation[4], origin[4], position[4];
    float angle[3], speed[3], amplitude[3];
    float angle2[3], speed2[3], amplitude2[3];
    int fade;
    void *owner;
    char *object;
    int timer, duration, expired;
} PoolEffect;
extern PoolEffect D_0022AA10[3];
extern char D_001B2520[] MACRO_ADDR;
extern char *func_002AB1F0(void *);
extern void func_002B00B0(void *, void *, int);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern float func_00283B98(float, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern void func_00282D08(void *, void *, void *, float);
extern float func_00283CE0(int);
extern int func_00282948(void *);

void func_003166C8(void *owner)
{
    int i;
    PoolEffect *p;
    char *info, *position;
    char local[16] __attribute__((aligned(16)));
    float ratio;
    for (i = 0; i < 3; i++) {
        p = &D_0022AA10[i];
        if (p->owner != owner) continue;
        qcopy(p->origin, D_001B2520);
        if (p->timer) {
            p->angle[0] = func_00283B98(p->angle[0], p->speed[0] * 100.0f);
            p->angle[1] = func_00283B98(p->angle[1], p->speed[1] * 100.0f);
            p->angle[2] = func_00283B98(p->angle[2], p->speed[2] * 100.0f);
            p->angle2[0] = func_00283B98(p->angle2[0], p->speed2[0] * 100.0f);
            p->angle2[1] = func_00283B98(p->angle2[1], p->speed2[1] * 100.0f);
            p->angle2[2] = func_00283B98(p->angle2[2], p->speed2[2] * 100.0f);
        } else {
            p->angle[0] = func_00283B98(p->angle[0], p->speed[0] * 50.0f);
            p->angle[1] = func_00283B98(p->angle[1], p->speed[1] * 50.0f);
            p->angle[2] = func_00283B98(p->angle[2], p->speed[2] * 50.0f);
            p->angle2[0] = func_00283B98(p->angle2[0], p->speed2[0] * 50.0f);
            p->angle2[1] = func_00283B98(p->angle2[1], p->speed2[1] * 50.0f);
            p->angle2[2] = func_00283B98(p->angle2[2], p->speed2[2] * 50.0f);
        }
        p->offset[0] = func_00283180(p->angle[0]) * p->amplitude[0];
        p->offset[1] = func_00283198(p->angle[1]) * p->amplitude[1];
        p->offset[2] = -func_00283198(p->angle[2]) * p->amplitude[2];
        p->rotation[0] = func_00283180(p->angle2[0]) * p->amplitude2[0];
        p->rotation[1] = func_00283198(p->angle2[1]) * p->amplitude2[1];
        p->rotation[2] = -func_00283198(p->angle2[2]) * p->amplitude2[2];
        if (p->object) {
            info = func_002AB1F0(p->object);
            if (info) {
                func_002B00B0(p->object, local, 1);
                func_00282D30(local, local, *(float *)(info + 16));
                func_00282CC0(p->position, local, p->object + 16);
                position = (char *)p->position;
            } else {
                qcopy(p->position, p->object + 16);
                position = (char *)p->position;
            }
            ratio = func_00283CE0(p->timer);
            ratio /= func_00283CE0(p->duration);
            func_00282D08(position, p->origin, position, 1.0f - ratio);
        }
        if (func_00282948(&p->timer)) {
            if (!p->expired) p->expired = 1;
            if (func_00282948(&p->fade)) p->owner = 0;
        }
    }
}
