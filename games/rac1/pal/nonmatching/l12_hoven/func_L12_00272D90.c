/* NON_MATCHING func_L12_00272D90 -- src/overlays/l12_hoven/mobyutil_00272D90.c
 * Best so far: BYTES 17/148 (88.5% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   the moves and the address computation, with the volatile qcopy asm as the
 *   barrier after them. The spills here are native `sq`, so
 *   tools/fix_core_spills.py is not the cause (that is the core_text residual in
 *   docs/DECOMP_PROGRESS.md). Not tried: swapping the stack slots (result before
 *   position). That moves the copy's destination off sp+0, which retail fixes, so
 *   I expect it to break more than the prologue. Note for the lead: the header
 *   of nonmatching/l12_hoven/func_L12_00272D90.c is stale ("SIZE ours 144"); the
 *   staged body is the V128 variant, which is worse than p7.c. Re-stage from p7.c.
 */
#include "common.h"

extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_00259B88(void *, void *, void *, void *, float);

// Projects a point ahead of the moby and tests movement toward it.
void func_L12_00272D90(char *moby, void *target) {
    float position[4];
    char result[16];
    qcopy(position, moby + 0x10);
    position[0] += func_001F9F90(*(float *)(moby + 0x48)) * 100.0f;
    position[1] += func_001F9FA8(*(float *)(moby + 0x48)) * 100.0f;
    func_L00_00259B88(moby, target, position, result, 1.0f);
}
