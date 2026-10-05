#include "common.h"
#include "structs.h"

/*
 * movie/disp.cpp in the original source; text 0x23C5E0-0x23C9C0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023C5E0); /* setImageTag */
ASM_FUNC("asm/handwritten/text", func_0023C7A8); /* vblankHandler */
INCLUDE_ASM("asm/nonmatchings/text", func_0023C910); /* handler_endimage */
INCLUDE_ASM("asm/nonmatchings/text", func_0023C960); /* startDisplay(int) */
/* ClearStageStateFlag - clears state flag at D_001612E0 */
void func_0023C9B0(void) {
    *(int *)0x001612E0 = 0;
}
