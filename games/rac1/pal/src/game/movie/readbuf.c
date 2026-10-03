#include "common.h"
#include "structs.h"

/*
 * movie/readbuf.cpp in the original source; text 0x23CD10-0x23CE18.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023CD10);
INCLUDE_ASM("asm/nonmatchings/text", func_0023CD28);
INCLUDE_ASM("asm/nonmatchings/text", func_0023CD30); /* readBufBeginPut(ReadBuf *, unsigned char **) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CD60); /* readBufEndPut(ReadBuf *, int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CDA8); /* readBufBeginGet(ReadBuf *, unsigned char **) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CDF0); /* readBufEndGet(ReadBuf *, int) */
