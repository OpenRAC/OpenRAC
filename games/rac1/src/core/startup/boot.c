#include "common.h"
#include "callees.h"
#include "structs/misc.h"
#include "asm_functions/executable.h"
#include "game/system/bmain.h"

/*
 * boot.cpp (0x12DA38-0x12DB68): ParseBin and main. crt0 calls func_0012DB18
 * as main. From 0x12DB18 on, retail's game compiler spills s-registers with
 * sq, as v1.14 does (docs/reference/TOOLCHAIN.md). Name from the NTSC split.
 */

extern int *level_root MACRO_ADDR;

typedef void (*BinFn)(void);

struct Chunk {
    unsigned char *dst;
    int size;
    int pad8;
    BinFn id;
};

/* ParseBin: copy each boot-binary chunk to its destination while chunk ids agree; return the id. Adapted from Lombyte's project (GPL-3.0) for PAL: gameplay/state/parse_bin.c, parse_bin. */
BinFn ParseBin(void) {
    unsigned char *p;
    struct Chunk *c;
    unsigned char *dst;
    unsigned char *src;
    BinFn id;

    id = 0;
    p = (unsigned char *)(level_root[0] + (int)level_root);
    while (c = (struct Chunk *)p, p = (unsigned char *)(c + 1), dst = c->dst, src = p,
           (id == 0) ? (id = c->id, 1) : (id == c->id)) {
        if (((c->size & 7) == 0) && (((unsigned int)p & 7) == 0) && (((unsigned int)dst & 7) == 0)) {
            unsigned char *d = dst;
            unsigned char *s = p;
            unsigned char *e = dst + c->size;
            while (d != e) {
                *(long *)d = *(long *)s;
                d += 8;
                s += 8;
            }
        } else {
            unsigned char *e = dst + c->size;
            while (dst != e) {
                *(int *)dst = *(int *)src;
                dst += 4;
                src += 4;
            }
        }
        p += c->size;
    }
    return id;
}

typedef void (*EntryFn)(void);

extern EntryFn ParseBin(void); /* ParseBin(void) */

void func_0012DB18(void) { /* main */
    EntryFn entry;

    func_0011DFC8();
    entry = startlevel;
    do {
        entry();
        entry = ParseBin();
        flush_cache(0);
        flush_cache(2);
    } while (1);
}
