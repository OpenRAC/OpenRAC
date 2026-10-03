#include "common.h"
#include "structs.h"

/*
 * movie/vibuf.cpp in the original source; text 0x23CEC8-0x23DE98.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023CEC8); /* getFIFOindex(ViBuf *, void *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CF10); /* setD3_CHCR(unsigned int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CF80); /* setD4_CHCR(unsigned int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023CFF0); /* scTag2 */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D018); /* viBufCreate */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D090); /* viBufReset(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D1F0); /* viBufBeginPut(ViBuf *, unsigned char **, int *, unsigned char **, int *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D2E8); /* viBufEndPut(ViBuf *, int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D340); /* viBufAddDMA(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D540); /* viBufStopDMA(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D650); /* viBufRestartDMA(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D988); /* viBufDelete(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023D9E0); /* viBufCount(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023DA30); /* viBufFlush(ViBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023DA88); /* viBufModifyPts(ViBuf *, TimeStamp *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023DBE0); /* viBufPutTs(ViBuf *, TimeStamp *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023DCF0); /* viBufGetTs(ViBuf *, TimeStamp *) */
