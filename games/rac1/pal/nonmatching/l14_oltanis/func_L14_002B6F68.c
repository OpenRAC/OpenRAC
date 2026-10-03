/* NON_MATCHING func_L14_002B6F68 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 192 / retail 188, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Resets a moby's state block (zeroes, float from gp global * 1.5), picks a waypoint from D_L14_001B0F30 by one 
 *   Matches except allocation/schedule of the qcopy operands: retail computes src (w+0x10) into $v1 before dst ($a
 *   Unblock: a source form that creates src's pseudo before dst's (allocation order tie).
 */
#include "common.h"
extern short D_L14_00161608;
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern void func_001F9BC0(void *);

/* reset a moby's state block and aim it at its first waypoint */
void func_L14_002B6F68(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int i;
    float v[4];
    char *w;
    *(float *)(data + 0x160) = *(float *)&D_L14_00161608 * 1.5f;
    *(int *)(data + 0xE8) = 0;
    *(int *)(data + 0xEC) = 0;
    *(int *)(data + 0x164) = 0;
    *(int *)(data + 0x168) = 0;
    *(int *)(data + 0x16C) = 0;
    *(int *)(data + 0xC0) = 0;
    *(int *)(data + 0xC4) = 0;
    if (*(int *)(data + 0x10C) == 0) {
        i = *(int *)(data + 0xD0);
    } else {
        i = *(int *)(data + 0x150);
    }
    qcopy(moby + 0x10, D_L14_001B0F30[i] + 0x10);
    func_001F9BF0(v, D_L14_001B0F30[i] + 0x20, moby + 0x10);
    *(float *)(moby + 0x48) = func_L00_001FF860(v[0], v[1]);
    *(int *)(data + 0xE4) = 0;
    func_001F9BC0(data + 0xF0);
}
