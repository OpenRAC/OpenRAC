/* NON_MATCHING func_L18_002D74F8 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 1/136 (99.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Retail calls `func_L00_002DCDA8` (which reads $4, $5 and $6) with only `$4`
 *   live, so $a1 and $a2 are this function's **own** incoming arguments passed
 *   straight through: `int func_L18_002D74F8(unsigned char *moby, int b, int c)`
 *   calling `func_L00_002DCDA8(moby, b, c)` emits no argument setup at all, which
 *   is what retail has. Same byte count, but the prototype is now right.
 *   Retail's two `lbu`s of moby[0x20] are one load in the RTL plus the copy the
 *   delay-slot filler puts in the annulled `beql` slot, so the source needs two
 *   non-mergeable reads. `-fstrict-aliasing` does not stop the merge either.
 */
#include "common.h"

extern int func_L00_002DCDA8(void *, int, int);
typedef struct { char pad[0x20]; unsigned char state; } MobyState;

int func_L18_002D74F8(unsigned char *moby, int b, int c) {
    char *data = *(char **)(moby + 0x78);
    int result = func_L00_002DCDA8(moby, b, c);
    unsigned char state = ((MobyState *)moby)->state;
    unsigned char moby_state = moby[0x20];
    if (result != 0) {
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
    } else if (moby_state == 4) {
        moby[0x20] = moby[0xBC];
    }
    return result;
}
