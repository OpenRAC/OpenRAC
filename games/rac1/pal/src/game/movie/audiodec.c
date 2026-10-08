#include "common.h"
#include "structs.h"

/*
 * movie/audiodec.cpp in the original source; text 0x23BFA0-0x23C5E0.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023BFA0); /* audioDecCreate(_AudioDec *, unsigned char *, int, sceMpegStrType) */
/* AudioDec: only the fields these functions touch are known. */
typedef struct AudioDec {
    int pending;        /* non-zero while data waits for the SPU; audioDecStart sets it to 2 */
    int mode;           /* 0x4 */
    char pad8[0xC];
    int f14;            /* 0x14 */
    int f18;            /* 0x18 */
    char pad1C[0x14];
    int fill;           /* 0x30 */
    unsigned char *buf; /* 0x34 */
    int rd;             /* 0x38 */
    int cnt;            /* 0x3C */
    int size;           /* 0x40 */
    int f44;            /* 0x44 */
    int f48;            /* 0x48 */
    int f4C;            /* 0x4C, rounded down to a multiple of 0x400 for the sound call */
    int bytes;          /* 0x50 */
    char pad54[8];
    int f5C;            /* 0x5C */
} AudioDec;

extern int func_0012F220(void);

/* audioDecDelete(_AudioDec *) -- calls func_0012F220 and returns 1. */
int func_0023C060(AudioDec *dec) {
    func_0012F220();
    return 1;
}
LINKER_REMNANT("asm/remnants/text", func_0023C080);
extern void func_0012F248(int, int, int, int, int);   /* snd_StartMovieSound */

/* audioDecStart -- starts the movie sound with the decoder's parameters (size rounded down to
 * 0x400) and marks the decoder as started. */
void func_0023C088(AudioDec *dec) {
    func_0012F248(dec->f48, dec->f4C / 0x400 * 0x400, dec->f5C, dec->f14, dec->f18);
    dec->pending = 2;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023C0E0); /* audioDecReset(_AudioDec *) */
/* audioDecBeginPut(_AudioDec *, unsigned char **, int *, unsigned char **, int *) -- hands out
 * the free part of the ring as up to two (pointer, length) spans. */
void func_0023C128(AudioDec *a, unsigned char **p1, int *n1, unsigned char **p2, int *n2) {
    if (a->pending == 0) {
        if (a->mode != 4) {
            *p1 = (unsigned char *)a + (a->fill + 8);
            *n1 = 0x28 - a->fill;
            *p2 = a->buf;
            *n2 = a->size;
            return;
        }
        *p1 = a->buf;
        *n1 = a->size;
    none:
        *p2 = 0;
        *n2 = 0;
        return;
    } else {
        int avail = a->size - a->cnt;
        if (a->size - a->rd >= avail) {
            *p1 = a->buf + a->rd;
            *n1 = avail;
            goto none;
        }
        *p1 = a->buf + a->rd;
        *n1 = a->size - a->rd;
        *p2 = a->buf;
        *n2 = avail - (a->size - a->rd);
        return;
    }
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023C1F8); /* audioDecEndPut(_AudioDec *, int) */
/* No recovered name. True once 0x1000 bytes or more are queued. */
int func_0023C2B0(AudioDec *dec) {
    return dec->bytes >= 0x1000;
}
extern void func_0023C390(AudioDec *);

/* audioDecSend -- sendADPCM while data is pending. */
void func_0023C2C0(AudioDec *dec) {
    if (dec->pending) {
        func_0023C390(dec);
    }
}
typedef struct { int src; int dst; int size; int mode; } SpuDma;

extern void func_00118D80(int);
extern unsigned int func_00118E20(SpuDma *, int);
extern int func_00118E10(int);
extern int func_0012F288(int, int);

/* sendToSPU(_AudioDec *, unsigned char *, int, int) -- queues one transfer of len bytes to the
 * address in the decoder at 0x48, waits for it to finish, then calls func_0012F288(len, flag). */
void func_0023C2E8(AudioDec *dec, unsigned char *src, int len, int flag) {
    SpuDma dma;
    unsigned int id;
    func_00118D80(0);
    dma.src = (int)src;
    dma.dst = dec->f48;
    dma.size = len;
    dma.mode = 0;
    while ((id = func_00118E20(&dma, 1)) == 0) {
    }
    while (func_00118E10(id) >= 0) {
    }
    func_0012F288(len, flag);
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023C390); /* sendADPCM(_AudioDec *) */
