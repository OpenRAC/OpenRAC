/* NON_MATCHING func_L00_002657B8 -- src/overlays/shared/mobyutil_00265558.c
 * Best so far: SIZE ours 264 / retail 260, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   `-mno-split-addresses`), so that lever doesn't apply here: the mismatch
 *   is a branch-likely/delay-slot-fill choice, not a saved-register or
 *   `lui` count difference. Not attempted.
 *   Everything else in the function -- the list-build loop, the sentinel
 *   checks, the `func_L00_002514B8` call and its own duplicated pointer
 *   load, the final list walk and both epilogue paths -- matches retail
 *   exactly.
 *   Stopping here per the three-tie rule with 5 of 10 runs used.
 */
#include "common.h"

/*
 * Builds the frame's moby update list from the level's moby array, then
 * walks that list calling each moby's per-frame update hooks.
 */

typedef struct MobyRecord {
    char pad0[0x20];
    signed char state; /* +0x20 */
    char pad1[0x28 - 0x21];
    struct MobyRecord *next; /* +0x28 */
    char pad2[0x34 - 0x2C];
    unsigned short flags; /* +0x34 */
    char pad3[0x74 - 0x36];
    void (*updateFn)(struct MobyRecord *); /* +0x74 */
    char pad4[0x100 - 0x78];
} MobyRecord;

extern int D_L00_001ABD80;
extern MobyRecord *D_L00_001600A4;
extern MobyRecord *D_L00_00160098;

extern void func_L00_002514B8(MobyRecord *);
extern void func_L00_00251E30(MobyRecord *);

void func_L00_002657B8(void)
{
    MobyRecord *cur;
    MobyRecord *prev;

    D_L00_001ABD80 = 0;
    D_L00_001600A4 = 0;

    cur = D_L00_00160098;
    prev = 0;
    if ((unsigned char)cur->state != 0xFF) {
        do {
            if (!((unsigned char)cur->state & 0x80)) {
                if (!(cur->flags & 0x2)) {
                    if (prev == 0)
                        D_L00_001600A4 = cur;
                    else
                        prev->next = cur;
                    prev = cur;
                }
            }
            cur = (MobyRecord *)((char *)cur + 0x100);
        } while ((unsigned char)cur->state != 0xFF);
    }
    if (prev != 0)
        prev->next = 0;

    cur = D_L00_001600A4;
    if (cur != 0) {
        do {
            if (cur->state >= 0) {
                if (!(cur->flags & 0x40)) {
                    func_L00_002514B8(cur);
                }
                if (cur->updateFn == 0) {
                } else {
                    cur->updateFn(cur);
                }
                if (!(cur->flags & 0x4)) {
                    func_L00_00251E30(cur);
                }
            }
            cur = cur->next;
        } while (cur != 0);
    }
}
