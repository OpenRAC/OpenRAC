#include "common.h"
#include "structs.h"

/*
 * movie/videodec.cpp in the original source; text 0x23DE98-0x23E560.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023DE98); /* videoDecCreate(VideoDec *, unsigned char *, int, unsigned long long *, unsigned long long *, int, TimeStamp *, int) */
LINKER_REMNANT("asm/remnants/text", func_0023DF98);
extern int func_0012B008(void);

/* video_dec_set_stream - calls func_0012B008 */
int func_0023DFA0(void) {
    func_0012B008();
    return 1;
}
extern void func_0023D1F0(int);

/* video_dec_begin_put - calls func_0023D1F0 with offset */
void func_0023DFC0(int *a) {
    func_0023D1F0(a + 0x12);
}
extern void func_0023D2E8(int);

/* video_dec_end_put - calls func_0023D2E8 with offset */
void func_0023DFE0(int *a) {
    func_0023D2E8(a + 0x12);
}
/* ClearStateField - clears a field at offset 0xA8 */
void func_0023E000(int *a) {
    a[0x2A] = 0;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E008); /* videoDecDelete(VideoDec *) */
/* SetStateField - sets field at offset 0xA8 to 1 */
void func_0023E040(int *a) {
    a[0x2A] = 1;
}
/* GetStateField - returns field at offset 0xA8 */
int func_0023E050(int *a) {
    return a[0x2A];
}
/* ReplaceStateField - returns old value and sets new value at offset 0xA8 */
int func_0023E058(int *a, int val) {
    int old = a[0x2A];
    a[0x2A] = val;
    return old;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E068); /* videoDecPutTs(VideoDec *, long, long, unsigned char *, int) */
extern int func_0023D9E0(int);

/* video_dec_input_count - calls func_0023D9E0 with offset */
int func_0023E0B0(int *a) {
    return func_0023D9E0(a + 0x12);
}
LINKER_REMNANT("asm/remnants/text", func_0023E0D0);
INCLUDE_ASM("asm/nonmatchings/text", func_0023E0D8); /* videoDecFlush(VideoDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E1B0); /* videoDecIsFlushed(VideoDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E1F8); /* videoDecMain(void *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E298); /* decBs0(VideoDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E450); /* mpegError(sceMpeg *, sceMpegCbDataError *, void *) */
extern void func_0023BB40(void);
extern int func_0023D340(int);
extern int D_0016130C MACRO_ADDR;

/* process_video_stream - calls func_0023BB40 then func_0023D340 */
int func_0023E478(void) {
    func_0023BB40();
    func_0023D340(D_0016130C + 0xD9090);
    return 1;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E4B0);
INCLUDE_ASM("asm/nonmatchings/text", func_0023E4E0);
INCLUDE_ASM("asm/nonmatchings/text", func_0023E510);
