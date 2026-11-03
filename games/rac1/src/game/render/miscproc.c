#include "common.h"
#include "callees.h"
#include "structs/records.h"
#include "structs/misc.h"

/*
 * miscproc.cpp in the original source; text 0x20C210-0x20C7A0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/reference/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/*
 * Not a decompile target: hand-written assembly, like the rest of this
 * DMAC block (InitDma/flush_pending_texture_uploads are explicitly marked
 * "Handwritten function" by spimdisasm; this one and DmaToSprSync use
 * the same idioms but escaped that heuristic). It keeps the base
 * address 0x1000D400 live in $1/$at across all four stores -- $at is
 * assembler-reserved on MIPS and GCC will never allocate it -- and
 * materializes 0x100 with `ori $2,$0,0x100` where a compiler emits
 * `addiu`. An earlier revision of this comment read the same
 * disassembly as compiler output and invented a "redundant address
 * reload for constant addresses" codegen category to explain it; that
 * category was never real.
 */
ASM_FUNC("asm/handwritten/text", func_0020C210);

ASM_FUNC("asm/handwritten/text", func_0020C230);

ASM_FUNC("asm/handwritten/text", func_0020C268);

ASM_FUNC("asm/handwritten/text", func_0020C2F8);

ASM_FUNC("asm/handwritten/text", func_0020C468);

ASM_FUNC("asm/handwritten/text", func_0020C738);

ASM_FUNC("asm/handwritten/text", func_0020C758);
