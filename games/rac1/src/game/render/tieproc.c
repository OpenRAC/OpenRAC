#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"

/*
 * tieproc.cpp in the original source; text 0x236F00-0x238D88.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

ASM_FUNC("asm/handwritten/text", func_00236F00);

ASM_FUNC("asm/handwritten/text", func_002383D8);

ASM_FUNC("asm/handwritten/text", func_00238688);
