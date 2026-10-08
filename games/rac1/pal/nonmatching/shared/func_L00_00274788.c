/* NON_MATCHING func_L00_00274788 -- src/overlays/shared/partupd_00272158.c
 * Best so far: BYTES 4/384 (99.0% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   PartType64Spawn: allocs a particle (type 0x40), copies pos, sprite byte from distance to hero (<80), copies ve
 *   Best p8.c: same instructions, only register assignment differs (life/a4 in s5/s6, f1/f2 in f22/f21 swapped). A
 *   Would unblock: a wording that puts a4 and f1 earlier in priority without moving the store order (perhaps diffe
 *   z07 round: fresh budget used (p9-p14). Best p12/p8 (10 bytes). Retail has a4 in s5 and life in s6 (a4 higher a
 *   mini14 a03: type-64 particle spawner; ABI-equivalent parameter interleavings p15-p18 preserve argument order w
 *   Best p17.c BYTES 4/384 fixes life/a4 saved registers; remaining f1/f2 saved registers swapped at +3c/+44 and c
 *   Unblock requires a natural form combining p17 integer allocation with p15 float allocation; no documented stor
 */
#include "common.h"
extern unsigned char *func_00218928(int);
extern float func_001F9D48(void *, void *);
extern int func_001FA898(float);
extern float func_002140F8(float, float);
extern unsigned char D_0013E633[] NOT_SDA;
extern unsigned char *D_L00_001B2500;

/* Spawns a type-64 particle at pos moving with vel, sprite size chosen by distance to the hero. */
unsigned char *func_L00_00274788(void *pos, void *vel, float scale, int life, int col, float f1, float f2, int a4, int big) {
    unsigned char *r = func_00218928(0x40);
    unsigned char *u;
    if (r != 0) {
        u = r + 0x20;
        qcopy(r + 0x10, pos);
        if (func_001F9D48(D_0013E633 + 0xE9D, pos) < 80.0f) {
            r[9] = func_001FA898(1.0f) + 0x30;
        } else {
            r[9] = func_001FA898(1.0f) + 0x20;
        }
        r[3] = big ? 0x48 : 0x44;
        r[1] = 0;
        r[2] = D_L00_001B2500[1];
        *(int *)(r + 4) = col;
        *(float *)(r + 0xC) = scale * 210000.0f;
        r[8] = (int)func_002140F8(0.0f, 255.0f);
        *(short *)(r + 0xA) = life;
        qcopy(u, vel);
        *(float *)(u + 0x10) = f1;
        *(float *)(u + 0x14) = f2;
        *(int *)(u + 0x18) = a4;
    }
    return r;
}
