#include "common.h"
#include "structs.h"

/*
 * movie/videodec.cpp in the original source; text 0x23DE98-0x23E560.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023DE98); /* videoDecCreate(VideoDec *, unsigned char *, int, unsigned long long *, unsigned long long *, int, TimeStamp *, int) */
LINKER_REMNANT("asm/remnants/text", func_0023DF98);
/* VideoDec: only the fields these functions touch are known. */
typedef struct VideoDec {
    char pad0[0x48];
    char viBuf[0x60];   /* ViBuf, the video input buffer */
    int state;          /* 0xA8 */
} VideoDec;

extern int func_0012B008(void *, int, int, int, int);
extern void func_0023D1F0(void *, unsigned char **, int *, unsigned char **, int *);
extern void func_0023D2E8(void *);
extern int func_0023D9E0(void *);

/* videoDecSetStream(VideoDec *, int, int, int (*)(sceMpeg *, sceMpegCbData *, void *), void *)
 * -- hands its five arguments to func_0012B008 and returns 1. */
int func_0023DFA0(VideoDec *dec, int a1, int a2, int (*cb)(void *, void *, void *), void *arg) {
    func_0012B008(dec, a1, a2, (int)cb, (int)arg);
    return 1;
}
/* videoDecBeginPut(VideoDec *, unsigned char **, int *, unsigned char **, int *)
 * -- viBufBeginPut on the decoder's input buffer. */
void func_0023DFC0(VideoDec *dec, unsigned char **p1, int *n1, unsigned char **p2, int *n2) {
    func_0023D1F0(dec->viBuf, p1, n1, p2, n2);
}
/* videoDecEndPut(VideoDec *) -- viBufEndPut on the decoder's input buffer.
 * The symbol table gives viBufEndPut a second int argument; retail sets none up here. */
void func_0023DFE0(VideoDec *dec) {
    func_0023D2E8(dec->viBuf);
}
/* videoDecReset(VideoDec *) -- clears the decoder state. */
void func_0023E000(VideoDec *dec) {
    dec->state = 0;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E008); /* videoDecDelete(VideoDec *) */
/* videoDecAbort(VideoDec *) -- sets the decoder state to 1. */
void func_0023E040(VideoDec *dec) {
    dec->state = 1;
}
/* videoDecGetState -- returns the decoder state. */
int func_0023E050(VideoDec *dec) {
    return dec->state;
}
/* videoDecSetState(VideoDec *, unsigned int) -- stores the new state, returns the old one. */
int func_0023E058(VideoDec *dec, unsigned int state) {
    int old = dec->state;
    dec->state = state;
    return old;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E068); /* videoDecPutTs(VideoDec *, long, long, unsigned char *, int) */
/* videoDecInputCount(VideoDec *) -- viBufCount of the input buffer. */
int func_0023E0B0(VideoDec *dec) {
    return func_0023D9E0(dec->viBuf);
}
LINKER_REMNANT("asm/remnants/text", func_0023E0D0);
INCLUDE_ASM("asm/nonmatchings/text", func_0023E0D8); /* videoDecFlush(VideoDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E1B0); /* videoDecIsFlushed(VideoDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E1F8); /* videoDecMain(void *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E298); /* decBs0(VideoDec *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E450); /* mpegError(sceMpeg *, sceMpegCbDataError *, void *) */
extern void func_0023BB40(void);
extern int func_0023D340(char *);
extern char *D_0016130C MACRO_ADDR;

/* mpegNodata(sceMpeg *, sceMpegCbData *, void *) -- switchThread, then viBufAddDMA on the
 * buffer at offset 0xD9090 of the movie state. Returns 1. */
int func_0023E478(void *mpeg, void *cbdata, void *arg) {
    func_0023BB40();
    func_0023D340(D_0016130C + 0xD9090);
    return 1;
}
extern int func_0023D540(char *);   /* viBufStopDMA */

/* No recovered name. viBufStopDMA on the
 * ViBuf at offset 0xD9090 of the movie state. Returns 1. */
int func_0023E4B0(void) {
    func_0023D540(D_0016130C + 0xD9090);
    return 1;
}
extern int func_0023D650(char *);   /* viBufRestartDMA */

/* No recovered name. viBufRestartDMA on the ViBuf at
 * offset 0xD9090 of the movie state. Returns 1. */
int func_0023E4E0(void) {
    func_0023D650(D_0016130C + 0xD9090);
    return 1;
}
typedef struct { long first, second; long pad[2]; } TimeStamp;   /* 0x20 bytes: retail reserves that much */
extern void func_0023DCF0(char *, TimeStamp *);   /* viBufGetTs */

/* No recovered name. Reads the
 * timestamp of the ViBuf at offset 0xD9090 of the movie state with viBufGetTs and stores it
 * at out+8. Returns 1. */
int func_0023E510(int unused, char *out) {
    TimeStamp ts;
    func_0023DCF0(D_0016130C + 0xD9090, &ts);
    *(long *)(out + 8) = ts.first;
    *(long *)(out + 0x10) = ts.second;
    return 1;
}
