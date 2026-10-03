#include "common.h"
#include "structs.h"

/*
 * actuator.cpp in the original source; text 0x1E9080-0x1E94E8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

LINKER_REMNANT("asm/remnants/text", func_001E9080);

INCLUDE_ASM("asm/nonmatchings/text", func_001E9088); /* actuator_CalcPower */

LINKER_REMNANT("asm/remnants/text", func_001E94A0);

extern int D_0015EF8C MACRO_ADDR;
extern int D_0015F020 MACRO_ADDR;
extern short D_0015EF74;

void func_001E94A8(void) {
    D_0015F020 = 0;
    *(int *)&D_0015EF74 = D_0015EF8C;
}
__asm__(".section .text
	nop
	nop
");

INCLUDE_ASM("asm/nonmatchings/text", func_001E94C8);
