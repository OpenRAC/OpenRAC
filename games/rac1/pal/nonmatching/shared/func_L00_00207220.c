/* NON_MATCHING func_L00_00207220 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 244 / retail 240, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00207220: loops over 8 slots (ints at D_0013F450+0x2218, owners at +0x2238); if the owner (or the wor
 *   Logic and two separate lui bases match (use D_0013F450 for g so it is not merged with D_0013E633+0x1D); differ
 *   Would unblock: the source form that leaves owner[i] unreduced while the slot walks as a pointer (best was p7.c
 */
#include "common.h"
extern unsigned char D_0013E633[] NOT_SDA;
extern unsigned char D_0013F450[] NOT_SDA;
extern void func_L00_0028EBF0(void);

/* for each of 8 slots, calls the handler when its owner still matches, then frees it */
void func_L00_00207220(void) {
    char *g = (char *)D_0013F450;
    char *h = (char *)D_0013E633 + 0x1D;
    int *slot = (int *)(g + 0x2218);
    int *owner = (int *)(g + 0x2238);
    int i;
    for (i = 0; i < 8; i++) {
        int s = slot[i];
        int v = owner[i];
        if (v != 0) {
            if (s != -1) {
                if (*(int *)(h + s * 0x70 + 0x88) == v && *(unsigned char *)(h + s * 0x70 + 0x74) != 0) {
                    func_L00_0028EBF0();
                }
            }
        } else {
            if (s != -1) {
                if (*(int *)(h + s * 0x70 + 0x88) == *(int *)(g + 0x2080) && *(unsigned char *)(h + s * 0x70 + 0x74) != 0) {
                    func_L00_0028EBF0();
                }
            }
        }
        slot[i] = -1;
    }
}
