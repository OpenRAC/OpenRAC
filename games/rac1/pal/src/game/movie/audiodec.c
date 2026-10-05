#include "common.h"
#include "structs.h"

/*
 * movie/audiodec.cpp in the original source; text 0x23BFA0-0x23C5E0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023BFA0); /* audioDecCreate(_AudioDec *, unsigned char *, int, sceMpegStrType) */
extern int func_0012F220(void);

/* audio_dec_delete - calls func_0012F220 */
int func_0023C060(void) {
    func_0012F220();
    return 1;
}
LINKER_REMNANT("asm/remnants/text", func_0023C080);
INCLUDE_ASM("asm/nonmatchings/text", func_0023C088); /* audioDecStart */
INCLUDE_ASM("asm/nonmatchings/text", func_0023C0E0); /* audioDecReset(_AudioDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023C128); /* audioDecBeginPut(_AudioDec *, unsigned char **, int *, unsigned char **, int *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023C1F8); /* audioDecEndPut(_AudioDec *, int) */
/* IsStateFieldLarge - returns !((*(int*)(a+0x50)) < 0x1000) */
int func_0023C2B0(int *a) {
    int v = a[0x14];
    return !(v < 0x1000);
}
extern void func_0023C390(void);

/* audio_dec_send - calls func_0023C390 if *a is non-zero */
void func_0023C2C0(int *a) {
    int v = *a;
    if (v) {
        func_0023C390();
    }
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023C2E8); /* sendToSPU(_AudioDec *, unsigned char *, int, int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023C390); /* sendADPCM(_AudioDec *) */
