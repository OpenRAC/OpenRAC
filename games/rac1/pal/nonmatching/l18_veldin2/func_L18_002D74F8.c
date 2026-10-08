/* NON_MATCHING func_L18_002D74F8 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 1/136 (99.3% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - GCC's reoptimizer predicts the branch is taken
 *   The `(char)` cast prevents GCC from merging the two reads of `moby[0x20]`, but causes one to use signed load (
 *   1. Keeps the two reads distinct in RTL (preventing merge)
 *   2. Both reads use unsigned load (`lbu`)
 *   3. Matches the reoptimizer's branch prediction behavior
 *   ## Related
 *   - Function checks moby state and updates it based on result from func_L00_002DCDA8
 *   - The function is small (136 bytes) and leaf-like
 */
#include "common.h"

extern int func_L00_002DCDA8(void *, int, int);
typedef struct { char pad[0x20]; unsigned char state; } MobyState;

int func_L18_002D74F8(unsigned char *moby, int b, int c) {
    char *data = *(char **)(moby + 0x78);
    int result = func_L00_002DCDA8(moby, b, c);
    if (result != 0) {
        unsigned char state = ((MobyState *)moby)->state;
        if (state >= 3 && state <= 4) {
            if (state != 4) {
                moby[0xBC] = state;
                moby[0x20] = 4;
            }
            result = 2;
        } else {
            *(short *)(data + 0xC8) = 0;
            result = 0;
        }
    } else if ((char)moby[0x20] == 4) {
        moby[0x20] = moby[0xBC];
    }
    return result;
}
