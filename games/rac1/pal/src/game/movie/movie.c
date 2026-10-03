#include "common.h"
#include "structs.h"

/*
 * movie/movie.cpp in the original source; text 0x23B670-0x23BFA0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023B670);
INCLUDE_ASM("asm/nonmatchings/text", func_0023B740);
INCLUDE_ASM("asm/nonmatchings/text", func_0023BB40); /* switchThread */
INCLUDE_ASM("asm/nonmatchings/text", func_0023BB60); /* isAudioOK */
INCLUDE_ASM("asm/nonmatchings/text", func_0023BB90); /* initAll(int, int, int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023BE38); /* termAll(void) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023BF48); /* ErrMessage */
INCLUDE_ASM("asm/nonmatchings/text", func_0023BF70); /* proceedAudio(void) */
