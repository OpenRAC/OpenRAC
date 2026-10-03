#include "common.h"
#include "structs.h"

/*
 * movie/read.cpp in the original source; text 0x23C9C0-0x23CD10.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023C9C0); /* videoCallback */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CAF8); /* pcmCallback(sceMpeg *, sceMpegCbDataStr *, void *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CBE0); /* cpy2area(unsigned char *, int, unsigned char *, int, unsigned char *, int, unsigned char *, int) */
