#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * core_text object 0x11D700-0x11D960. Boundaries are retail's linker fill
 * (0xCDCDCDCD) between objects; see docs/reference/DECOMP_PROGRESS.md.
 *
 * Sony's EE kernel library (libkernl), tlbtrap.o: the TLB/debug
 * exception handlers, hand-written assembly (so no compiler applies).
 * Listed with the 2.9-ee SDK objects for consistency.
 */


ASM_FUNC("asm/handwritten/core_text", func_0011D700);
