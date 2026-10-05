#include "common.h"
#include "structs.h"

/*
 * movie/strfile.cpp in the original source; text 0x23CE18-0x23CEC8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* InitializeStateFields - initializes state fields */
int func_0023CE18(int *a, int val1, int val2) {
    a[1] = val1;
    a[0] = val2;
    return 1;
}
/* GetStateCallbackResult - returns 1 */
int func_0023CE28(void) {
    return 1;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023CE30);
