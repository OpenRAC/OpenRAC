#include "common.h"
#include "structs.h"

/*
 * movie/strfile.cpp in the original source; text 0x23CE18-0x23CEC8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* StrFile: only the fields these functions touch are known. */
typedef struct StrFile {
    int second;
    int first;
} StrFile;

/* No recovered name. Stores its two arguments in the object (first argument at offset 4)
 * and returns 1. */
int func_0023CE18(StrFile *f, int first, int second) {
    f->first = first;
    f->second = second;
    return 1;
}
/* No recovered name. Nothing to free, returns 1. */
int func_0023CE28(StrFile *f) {
    return 1;
}
INCLUDE_ASM("asm/nonmatchings/text", func_0023CE30);
