/* SPDX-License-Identifier: MIT
 *
 * Near miss: func_0032AE30 (SpawnZodiacNovaTrailNode), 284 bytes in the retail boot and in
 * every level, 28 places.
 *
 * Profile: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier, in a source unit
 * named 00327038-qcopy.c (the grouped lq/sq copy of the position needs the quadword-copy
 * profile; the default profile splits the pair across the call's delay slot).
 *
 * Result: 284/284 bytes, 0/28 places in USA v1.01. The first difference is at +0xac: retail
 * keeps the second float sample in the delay slot of the func_002A7820 call, this source
 * leaves it two instructions earlier and moves the argument setup into the delay slot.
 * Everything else matches: the slot scan (bnel with the delay-slot increment), the free
 * slot test, the two quadword copies, the 0x800000 flag, the two random floats, the
 * 180..360 degree sample scaled by 0.00029088824f (bits 0x3998825C) and the counter.
 *
 * The remaining difference is the scheduler's choice for one delay slot: the same
 * statement order, the same declarations and the -frac2-qcopy profile with and without
 * the barrier all place the store differently. Reopen with a source shape that changes the
 * scheduler's priority for that store (the arguments of the call are constants in every
 * form tried).
 */

#include "common.h"

/* The 128-bit integer mode is what reaches retail's lq/sq copies. */
typedef int TI128 __attribute__((mode(TI)));

extern char D_002329A0[];

extern void func_00282C88(s8 *p);

extern float func_002A7878(float, float);

extern int func_002A7820(int, int);

/* The helper returns a float in $f0; retail keeps its integer register. */
extern int func_002A7940(void);

extern float func_002A78D8(float, float);

/* Fills the first free slot of the resident trail-node array with a new node. */
void func_0032AE30(s32 owner, s8 *pos, s32 value)
{
    char *base = D_002329A0;
    s32 flag = 0x800000;
    s32 *count = *(s32 *)(owner + 0x68);
    char *p = base + 0x10;
    char *slot = base;
    char *end = slot + 0x2800;
    float v;

    do {
        if (*(s32 *)(slot + 0x44) == 0) {
            *(s32 *)(slot + 0x44) = owner;
            *(s32 *)(slot + 0x30) = value;
            *(TI128 *)slot = *(TI128 *)pos;
            func_00282C88(p);
            *(s32 *)(slot + 0x38) = flag;
            *(float *)(slot + 0x20) = func_002A7878(0.1f, 0.3f);
            *(float *)(slot + 0x24) = func_002A7878(0.4f, 0.5f);
            *(s32 *)(slot + 0x40) = func_002A7820(0x19, 0x28);
            *(s32 *)(slot + 0x3C) = func_002A7940();
            v = func_002A78D8(180.0f, 360.0f);
            *(float *)(slot + 0x2C) = v;
            *(float *)(slot + 0x28) = v * 0.00029088824f;
            *count += 1;
            break;
        }
        slot += 0x50;
        p += 0x50;
    } while ((long)slot < (long)end);
}
