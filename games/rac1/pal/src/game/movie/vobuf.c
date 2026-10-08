#include "common.h"
#include "structs.h"

/*
 * movie/vobuf.cpp in the original source; text 0x23E560-0x23E730.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

INCLUDE_ASM("asm/nonmatchings/text", func_0023E560);
typedef struct VoBuf {
    char pad0[4];
    char *data;     /* ring of 0x138C0-byte entries */
    int wr;         /* 0x8 */
    int count;      /* 0xC */
    int cap;        /* 0x10 */
} VoBuf;

/* voBufDelete(VoBuf *) -- nothing to free. */
void func_0023E5B0(VoBuf *vb) {
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E5B8); /* voBufReset(VoBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E5C8); /* voBufIsFull(VoBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E5E0); /* voBufIncCount(VoBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E658); /* voBufGetData(VoBuf *) */
/* voBufIsEmpty -- true when the entry count is zero. */
int func_0023E698(VoBuf *vb) {
    return vb->count == 0;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023E6A8); /* voBufGetTag(VoBuf *) */
INCLUDE_ASM("asm/nonmatchings/text", func_0023E710); /* voBufDecCount(VoBuf *) */
