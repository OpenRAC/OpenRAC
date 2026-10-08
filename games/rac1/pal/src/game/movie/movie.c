#include "common.h"
#include "structs.h"

/*
 * movie/movie.cpp in the original source; text 0x23B670-0x23BFA0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023B670);
INCLUDE_ASM("asm/nonmatchings/text", func_0023B740);
extern int func_00118BC0(int);

/* switchThread -- calls func_00118BC0(1). */
void func_0023BB40(void) {
    func_00118BC0(1);
}
extern int func_0023C2B0(char *);
extern char *D_0016130C MACRO_ADDR;

/* isAudioOK -- true when the audio decoder at offset 0xD9100 of the movie state holds 0x1000
 * bytes or more. */
int func_0023BB60(void) {
    return func_0023C2B0(D_0016130C + 0xD9100);
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023BB90); /* initAll(int, int, int) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023BE38); /* termAll(void) */
extern void func_001E9730(char *, ...);
extern char D_001612F8[];

/* ErrMessage -- passes the message to func_001E9730 (STUB_printf) with the string at
 * D_001612F8 as its first argument. */
void func_0023BF48(char *fmt) {
    func_001E9730(D_001612F8, fmt);
}
extern void func_0023C2C0(char *);

/* proceedAudio(void) -- audioDecSend on the audio decoder at offset 0xD9100 of the movie state. */
void func_0023BF70(void) {
    func_0023C2C0(D_0016130C + 0xD9100);
}
