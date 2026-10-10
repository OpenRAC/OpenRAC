/* NON_MATCHING func_L12_0030A880 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 136 / retail 152, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   variant tried (p6 to p10) moved it.
 *   3. Address at the call. Retail keeps the `lui` high part in `$4` (`daddu $4,$2,
 *   $0`) and rebuilds `addiu $2,$4,%lo` at the call; ours keeps the full address
 *   in `$6`. Two extra instructions in retail, from the same allocator tie.
 *   Verdict for the next worker: stop here unless a source form that keeps the
 *   state constant away from the index compare shows up. Three variants (p7, p8,
 *   p9) compiled to the same bytes, so this is the allocator tie the protocol
 *   describes. Not a known wall in LEVERS.md.
 */
#include "common.h"
extern int D_L12_0015F6A8 MACRO_ADDR;
extern void func_L00_00264870(int);
typedef struct {
    char pad0[0x30];
    int current;
    char pad34[0x10];
    short count;
    char pad46[0x132];
    char *mobys[1];
} Level12MobyList;
extern Level12MobyList D_L12_0016CD60;

/* Runs a selected callback while the level is in mode two. */
void func_L12_0030A880(unsigned char *moby) {
    int state = moby[0x20];
    switch (state) {
    case 0:
        moby[0x30] = 0xFF;
        moby[0x20] = 1;
        break;
    case 1:
        if (D_L12_0015F6A8 == 2) {
            Level12MobyList *g = &D_L12_0016CD60;
            int cur = g->current;
            if (cur == state || cur == 7) {
                int index = 0;
                if (cur == state) index = 2;
                func_L00_00264870((int)g->mobys[index]);
            }
        }
        break;
    }
}
