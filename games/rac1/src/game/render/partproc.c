#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"

/*
 * partproc.cpp in the original source; text 0x218928-0x219780.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/*
 * This stays asm. In C it is `alloc_part_slot()` as a tail call, but retail's delay slot is the trapping
 * `addi $5,$0,0`, and GCC emits `addiu`. No source shape reaches that.
 */
ASM_FUNC("asm/handwritten/text", func_00218928);

ASM_FUNC("asm/handwritten/text", func_00218930);

LINKER_REMNANT("asm/remnants/text", func_00218A78);

ASM_FUNC("asm/handwritten/text", func_00218A80);

ASM_FUNC("asm/handwritten/text", func_00218B10);

ASM_FUNC("asm/handwritten/text", func_00219704);

LINKER_REMNANT("asm/remnants/text", func_00219778);
