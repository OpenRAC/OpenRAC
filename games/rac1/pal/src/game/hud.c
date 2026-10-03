#include "common.h"
#include "structs.h"

/*
 * hud.cpp in the original source; text 0x1FF668-0x201D58.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char D_0013E650[];
extern int D_0015F694;
extern void func_001F9A98(void *, void *, int);
extern char D_00189310[];
extern char D_001899D0[];
extern void *D_001871C0 NOT_SDA;
typedef struct {
    char unk_00[8];
    void (*fn_08)(void *);
    char unk_0C[4];
    void (*fn_10)(void *);
} DispatchRec;
extern DispatchRec D_001E8F80[];
extern int D_0018A3B0[];
extern void func_001F99B0();
extern void func_001F2BC8(void);
extern int D_0018C434 NOT_SDA;
extern char D_001940C0[];
extern long D_00151888[3];
extern int D_0015F6FC;
extern short D_0015F534;
extern void func_001FB530(void);
extern void func_001F3D78(void);
extern int D_0015F564;
extern int D_0018DD40[];
extern int D_0018DC40[];
extern short D_0015F59C;
extern int func_001F65B0(unsigned char *arg0, int arg1, void *arg2);
extern unsigned char D_001DF3D0[];
extern unsigned char D_001DF770[];
extern unsigned char D_001DFB10[];
extern void func_001F6668(void *, void *, void *, void *, void *, int,
                          unsigned char *);
extern int func_001F6600(unsigned char *, int);
extern int func_001F6620(unsigned char *, int);
extern int func_001F4868(int);
extern void func_001F7070(void *, void *, void *, void *, int, unsigned char *);
extern void func_001FB498(void);
extern void func_001F3008(void);
extern void func_001F3140(void);
extern int D_0018E840[];
extern long D_00152178 NOT_SDA;
extern int func_001FE4D0(void);
extern char D_00199A68[];
extern short D_0015F780;
extern int func_001F6FD8(int a, int b, int c, int d, int e);

typedef struct {
    unsigned short id;
    char pad[6];
} HudIconRec;
extern HudIconRec *D_0019A504;

/* Finds arg0 in the table D_0019A504 points to (ended by id 0xFFFF).
   The older decode read D_0019A504 as the table itself. */
int func_001FF668(int arg0) {
    HudIconRec *tab = D_0019A504;
    int i = 0;

    while (tab[i].id != 0xFFFF && tab[i].id != arg0) {
        i++;
    }
    return i;
}

extern int D_0019A4E8 NOT_SDA;
extern int func_001FFB38(int, int, int, int, int, int, int);
extern char *func_001FFAB8_d(int, int, char *, int) __asm__("func_001FFAB8");
extern void func_001F99D8(void *, int);
extern char D_0015F7B8[];
extern unsigned char *D_0015FAB8 MACRO_ADDR;
extern unsigned char *D_0015FABC MACRO_ADDR;
extern unsigned char *D_0015FAC0 MACRO_ADDR;
extern unsigned char *D_0015FAC4 MACRO_ADDR;
typedef struct {
    int unk00, unk04;
    char pad08[0x18];
    int unk20, unk24;
    char pad28[0x3C];
    int unk64, unk68, unk6C;
    char pad70[0xC];
    int unk7C;
    char pad80[0x10];
} HudBank;
extern HudBank D_00199C60_b[] __asm__("D_00199C60") NOT_SDA;

/* HUD bank init: clears the first two words of the D_0019A4E8 arena,
   resets the 13 0x90-byte bank records at D_00199C60 (+0x64 = -1, the
   pending +0x20 selector = 0x10000, func_001FFB38(i, 0xFFFF, 0, 0, 0, 0,
   1), then +0x7C = 0, +0x6C = -6, +0x04 = +0x24 = 0), allocates the two
   HUD buffers once through the debug allocator (size, 0, "hud.cpp",
   line 277/278), sets D_0015FAC0/D_0015FABC from the first, clears
   0x2800 bytes of it and sets its byte +0x20 to 0xFF. The records are
   indexed through a struct array: each field access is its own giv,
   and loop.c's pick (the +0x24 one) is retail's loop pointer. */
void func_001FF6B8(void) {
    int *arena = &D_0019A4E8;
    int i;
    unsigned char *buf;

    arena[0] = 0;
    arena[1] = 0;
    for (i = 0; i < 13; i++) {
        D_00199C60_b[i].unk64 = -1;
        D_00199C60_b[i].unk20 = 0x10000;
        queue_animation_update(i, 0xFFFF, 0, 0, 0, 0, 1);
        D_00199C60_b[i].unk7C = 0;
        D_00199C60_b[i].unk6C = -6;
        D_00199C60_b[i].unk04 = 0;
        D_00199C60_b[i].unk24 = 0;
    }
    if (D_0015FAB8 == 0) {
        D_0015FAB8 = (unsigned char *)func_001FFAB8_d(0x2800, 0, D_0015F7B8, 277);
        D_0015FAC4 = (unsigned char *)func_001FFAB8_d(0x1400, 0, D_0015F7B8, 278);
    }
    buf = D_0015FAB8;
    D_0015FAC0 = buf + 0x2800;
    D_0015FABC = buf;
    FastMemZero16(buf, 0x2800);
    D_0015FAB8[0x20] = 0xFF;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

typedef struct {
    int offset;
    int unk4;
} HudEntry;

typedef struct {
    char pad0[0x14];
    int limits1[1];
    char pad18[0x1C];
    int limits2[1];
    char pad38[0x3C];
    int banks[1];
} HudBankHeader;

typedef struct {
    char pad0[0x18];
    HudBankHeader *header;
    char pad1C[8];
    HudEntry *table2;
    HudEntry *table1;
} HudArena;

extern HudArena D_0019A4E8_arena __asm__("D_0019A4E8");

/* LinkHudBank(int, char *) */
void func_001FF7F0(int bank, int addr) {
    int *banks;
    int *slot;
    int start;
    int limit;
    int i;

    banks = (int *)((char *)D_0019A4E8_arena.header + 0x74);
    slot = &banks[bank];
    if (*slot != 0) {
        return;
    }

    addr = (addr + 0xF) & 0xFFFFFFF0U;
    *slot = addr;

    if (bank != 0) {
        start = D_0019A4E8_arena.header->limits1[bank - 1];
    } else {
        start = 0;
    }
    limit = D_0019A4E8_arena.header->limits1[bank];

    for (i = start; i < limit; i++) {
        D_0019A4E8_arena.table1[i].offset &= 0x7FFFFFFF;
        D_0019A4E8_arena.table1[i].offset += addr;
    }

    if (bank != 0) {
        start = D_0019A4E8_arena.header->limits2[bank - 1];
    } else {
        start = 0;
    }
    limit = D_0019A4E8_arena.header->limits2[bank];

    for (i = start; i < limit; i++) {
        D_0019A4E8_arena.table2[i].offset &= 0x7FFFFFFF;
        D_0019A4E8_arena.table2[i].offset += addr;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001FF950);

INCLUDE_ASM("asm/nonmatchings/text", func_001FF958); /* Hud_SendResidentBank(int, char *, bool) */

extern int D_001941CC NOT_SDA;
extern int D_0019A4E8 NOT_SDA;

/* Hud_HeapReset(void) */
void func_001FFA90(void) {
    int *p = &D_0019A4E8;
    int v = D_001941CC;
    p[5] = v + 0x64000;
    p[4] = v;
}

/* Bump allocator out of the D_0019A4E8 arena: p[4] is the cursor,
   p[5] the limit. Rounds the request up to 16 bytes. */
/* Hud_HeapAlloc(unsigned int, char *, char *, int) */
int func_001FFAB8(int size) {
    int *p = &D_0019A4E8;
    int cur;

    if (p[4] == 0) {
        Hud_HeapReset();
    }
    if (p[5] - p[4] < size) {
        return 0;
    }
    cur = p[4];
    size = (size + 15) & 0xFFFFFFF0;
    p[4] = cur + size;
    return cur;
}

/*
 * 12 bytes of nop padding follow func_001FFAB8 in retail, after
 * `endlabel` in asm/nonmatchings/text/func_001FFAB8.s -- the same trap
 * documented above func_001F6668. Dropping it shifted the rest of the
 * segment by -8 (the next .align 3 only recovered 4 of the 12) and broke
 * func_00202790 and func_002208F8, both of which were exact and are
 * instruction-for-instruction identical apart from their jal targets.
 */
__asm__(".section .text
	nop
	nop
	nop
");

extern char D_00199C60_raw[] __asm__("D_00199C60") NOT_SDA;
extern int D_0015F6E8 MACRO_ADDR;
extern void func_001FFC48(void *arg0);

/* Finds or creates the residency record for arg0's slot (idx = arg0 & 0xF)
   in D_00199C60 (0x90-byte records), matching arg1..arg6 against the
   record's cached selectors; returns the record's +0x64 bank id, calling
   func_001FFC48 to reload the bank when a bit of (arg0's high nibbles &
   the record's +0x04 flags) is set. Sibling of func_001FFCB0/func_001FFD30
   in this file, which use the same record layout. */
int func_001FFB38(int arg0, int arg1, int arg2, int arg3, int arg4,
                 int arg5, int arg6) {
    int idx = arg0 & 0xF;
    char *p = D_00199C60_raw + idx * 0x90;
    int masked0 = arg0 & 0xFFF0;
    int r3;
    int cur;

    if (D_0015F6E8 == 5 && idx != 2 && idx != 0) {
        return 0;
    }
    if (*(int *)(p + 0x2C) == arg5 && *(int *)(p + 0x28) == arg6
        && *(int *)(p + 0x20) == arg1 && *(int *)(p + 0x24) == masked0
        && *(int *)(p + 0x30) == arg2 && *(int *)(p + 0x34) == arg3
        && *(int *)(p + 0x38) == arg4) {
        return *(int *)(p + 0x64);
    }
    cur = D_0019A4E8;
    *(int *)(p + 0x2C) = arg5;
    r3 = masked0 & *(int *)(p + 4);
    *(int *)(p + 0x64) = cur;
    r3 = r3 & 0x20;
    cur = cur + 1;
    *(int *)(p + 0x28) = arg6;
    D_0019A4E8 = cur;
    *(int *)(p + 0x20) = arg1;
    *(int *)(p + 0x30) = arg2;
    *(int *)(p + 0x34) = arg3;
    *(int *)(p + 0x38) = arg4;
    *(int *)(p + 0x68) = 1;
    *(int *)(p + 0x24) = masked0;
    *(int *)(p + 0x7C) = 0;
    *(int *)(p + 0x70) = 0;
    if (r3 != 0) {
        apply_pending_animation(p);
    }
    return *(int *)(p + 0x64);
}

extern void func_001FFD30(void *, int);

/* Copies the six pending words at +0x24..+0x38 into +0x04..+0x18 after
   func_001FFD30 has seen +0x20, calls the handler now at +0x10 if there
   is one, and clears +0x68. The copies are in retail's store order, and
   the +0x10 copy is unconditional (retail stores it in the beqz's delay
   slot), which also makes the scheduler load +0x30 first. */
void func_001FFC48(void *arg0) {
    char *p = (char *)arg0;

    load_animation_definition(arg0, *(int *)(p + 0x20));
    *(int *)(p + 4) = *(int *)(p + 0x24);
    *(int *)(p + 0x14) = *(int *)(p + 0x34);
    *(int *)(p + 0x18) = *(int *)(p + 0x38);
    *(int *)(p + 0xC) = *(int *)(p + 0x2C);
    *(int *)(p + 8) = *(int *)(p + 0x28);
    *(int *)(p + 0x10) = *(int *)(p + 0x30);
    if (*(int *)(p + 0x10) != 0) {
        (*(void (**)(void *))(p + 0x10))(arg0);
    }
    *(int *)(p + 0x68) = 0;
}

/* The 13 0x90-byte records at D_00199C60, looked up by their +0x64. */
typedef struct {
    int unk00, unk04;
    char pad08[0x1C];
    int unk24;
    char pad28[0x3C];
    int unk64, unk68;
    char pad6C[0x24];
} HudRec90;
extern HudRec90 D_00199C60[] NOT_SDA;
/* func_001FFB38 returns the bank id; this caller ignores it, and its
   match was made against a void view. */
extern void func_001FFB38_v(int, int, int, int, int, int, int) __asm__("func_001FFB38");

/* Calls func_001FFB38(i, 0xFFFF, 0, 0, 0, 0, 0) on the record whose
   +0x64 is arg0 and returns 1, or returns 0 when none is. The nop in
   the search loop is ps2eeas's short-loop padding. */
int func_001FFCB0(int arg0) {
    int i;
    for (i = 0; i < 13; i++) {
        if (D_00199C60[i].unk64 == arg0) {
            break;
        }
    }
    if (i < 13) {
        func_001FFB38_v(i, 0xFFFF, 0, 0, 0, 0, 0);
        return 1;
    }
    return 0;
}

extern int func_001FF668(int);

/* tbl[7] is volatile: retail re-loads that pointer field before each of
   the three record accesses rather than caching it, while keeping the
   r*8 offset in a register. */
void func_001FFD30(void *arg0, int arg1) {
    char *self = (char *)arg0;
    volatile int *tbl;
    int r;
    unsigned short v;

    r = Hud_GetIconIndex(arg1);
    /* base materialized only after the call, so it lands in a temp
       register rather than a callee-saved one; tbl[7] is volatile because
       retail re-loads that pointer field before each record access. */
    tbl = (volatile int *)&D_0019A4E8;
    v = *(unsigned short *)((char *)tbl[7] + r * 8);
    *(short *)(self + 0x40) = r;
    *(int *)(self + 0) = v;
    *(char *)(self + 0x42) = *(unsigned char *)((char *)tbl[7] + r * 8 + 6);
    *(int *)(self + 0x44) = *(unsigned short *)((char *)tbl[7] + r * 8 + 4);
}

LINKER_REMNANT("asm/remnants/text", func_001FFD98);

/* Stores arg1 into +0x24 of the record whose +0x64 is arg0, and into
   +0x04 when its +0x68 is 0. Indexing the extern array at each access,
   not through a cached base pointer, gives retail's per-access %lo. */
void func_001FFDA0(int arg0, int arg1) {
    int i;
    for (i = 0; i < 13; i++) {
        if (D_00199C60[i].unk64 == arg0) {
            break;
        }
    }
    if (i < 13) {
        D_00199C60[i].unk24 = arg1;
        if (D_00199C60[i].unk68 == 0) {
            D_00199C60[i].unk04 = arg1;
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_001FFE18);

typedef struct {
    char b[0x13];
} Cfg13;

extern Cfg13 D_0019A540 NOT_SDA;
extern Cfg13 D_001E7DD8 NOT_SDA;
extern int func_00116810(void);
extern void func_001166FC(Cfg13 *, void *);

/* The struct copy is a plain assignment: at alignment 1 this compiler
   expands the 0x13 bytes as unaligned ldl/ldr + sdl/sdr pairs with the
   trailing three bytes done singly, which is exactly retail's shape. */
void func_001FFE88(void *arg0) {
    if ((unsigned int)func_00116810() < 0x50) {
        D_0019A540 = D_001E7DD8;
    }
    func_001166FC(&D_0019A540, arg0);
}

INCLUDE_ASM("asm/nonmatchings/text", func_001FFF08);

extern short D_0015F9D0;   /* SDA (gp -0x7330) */

void func_001FFFA0(void) {
    int v = *(int *)&D_0015F9D0;
    if (v != 0) {
        *(int *)&D_0015F9D0 = v - 1;
    }
}

/* D_00199C60's 0x90-byte bank records, seen through their +0x18 update
   callback. */
typedef struct HudBankCb {
    char pad00[0x18];
    void (*update)(struct HudBankCb *);
    char pad1C[0x74];
} HudBankCb;

extern char D_0019A4E8_raw[] __asm__("D_0019A4E8");
extern HudBankCb D_00199C60_cb[] __asm__("D_00199C60") NOT_SDA;
extern int D_0015F544 MACRO_ADDR;
extern int D_0015F6E8 MACRO_ADDR;
extern int D_0015F760 MACRO_ADDR;
extern int D_0015F764 MACRO_ADDR;
extern int D_0015F768 MACRO_ADDR;
extern char D_001994D8[];
extern unsigned char D_00199528[];
extern int func_001F98C0(int);
extern void func_00201A38(int, int, int, int);

/* Per-frame HUD update: unless the arena's +0x30 skip flag (cleared
   here) or D_0015F544 is set, reset the arena's +0xC colour, run each
   bank's +0x18 callback, then fade the D_0015F764 alpha (0..128) in
   while the D_0015F760 timer runs, or out once it stops, and draw
   the overlay text D_001994D8 (plus D_00199528 above 1000) with it. */
void func_001FFFB8(void) {
    char *arena = D_0019A4E8_raw;
    int i;
    HudBankCb *p;
    int col;

    if (*(int *)(arena + 0x30) != 0 || D_0015F544 != 0) {
        *(int *)(arena + 0x30) = 0;
        return;
    }
    *(int *)(arena + 0xC) = 0xFFFFF0;
    p = D_00199C60_cb;
    for (i = 0; i < 13; i++) {
        if (p->update != 0) {
            p->update(p);
        }
        p++;
    }
    if ((D_0015F760 != 0 || D_0015F764 != 0) && D_0015F6E8 == 0) {
        if (D_0015F760 != 0) {
            D_0015F764 += 128 / func_001F98C0(8);
            if (D_0015F764 > 128) {
                D_0015F764 = 128;
            }
        } else {
            D_0015F764 -= 128 / func_001F98C0(8);
            if (D_0015F764 < 0) {
                D_0015F764 = 0;
            }
        }
        col = (D_0015F764 << 24) + 0xF0F0F0;
        func_00201A38(0x100, D_0015F768, col, (int)D_001994D8);
        if (D_00199528[0] != 0 && D_0015F760 > 1000) {
            func_00201A38(0x100, D_0015F768, col, (int)D_00199528);
        }
        if (D_0015F760 != 0) {
            D_0015F760--;
        }
        if (D_0015F760 == 1000) {
            D_0015F760 = 0;
        }
    } else {
        D_0015F768 = 100;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00200190);

extern char D_0019A4E8_raw[] __asm__("D_0019A4E8");
extern int func_001FF668(int);

/* GetIconFrame(iconId, sub): finds the icon's {id, count, base} row
   (func_001FF668 gives its index) in the arena's +0x1C table; frame =
   base + sub. Returns frame when the id is known, sub < count, and
   neither the +0x28 entry (for the frame's first index) nor the +0x24
   entry (for its second) has the top bit set; else 0. Row and pair
   are int sums so their addu takes the index first. Both flag tests
   are `& 0x80000000`: the first becomes a bltz, but the constant it
   loaded is reused by the second test, as in retail. */
int func_00200198(int iconId, int sub) {
    int idx;
    char *arena;
    unsigned short *row;
    int frame;
    short *pair;

    idx = Hud_GetIconIndex(iconId);
    arena = D_0019A4E8_raw;
    row = (unsigned short *)(idx * 8 + *(int *)(arena + 0x1C));
    if (row[0] == 0xFFFF) {
        return 0;
    }
    if (sub >= row[1]) {
        return 0;
    }
    frame = row[2] + sub;
    pair = (short *)(frame * 4 + *(int *)(arena + 0x20));
    if (*(int *)(*(char **)(arena + 0x28) + pair[0] * 8) & 0x80000000) {
        return 0;
    }
    if ((*(int *)(*(char **)(arena + 0x24) + pair[1] * 8) & 0x80000000) == 0) {
        return frame;
    }
    return 0;
}

__asm__(".section .text\n\tnop\n");

INCLUDE_ASM("asm/nonmatchings/text", func_00200248); /* GetFrameTex(int) */

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];
extern long func_00200248(int);

/* Draw HUD texture tex as a sprite at (x, y), w x h, with alpha: a
   1-tag + 5-quadword PACKED GIF packet as func_00200CA0's, with the
   texture's TEX0 from func_00200248 and its full size (1 << the
   entry's +6/+7 log2 sizes) as the far UV. The entry is found through
   the arena's +0x20 index table into its +0x24 texture table. */
void func_00200468(int tex, int x, int y, int w, int h, int alpha) {
    char *arena = D_0019A4E8_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << e[6];
    th = 1 << e[7];
    D_00161000[0] = 0x10000005;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000005;

    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = 0;
    p[6] = (x * 16 + D_0013E600[4] - 8)
         | ((long)(y * 16 + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (th << 20) + tw * 16;
    p[8] = ((x + w) * 16 + D_0013E600[4] - 8)
         | ((long)((y + h) * 16 + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x50);
}

/* func_00200468 drawn as a four-vertex strip (PRIM 0x154) instead of a
   sprite, the texture turned a quarter: corners (x, y+h), (x, y),
   (x+w, y+h), (x+w, y) take UVs (tw, 0), (tw, th), (0, 0), (0, th). */
void func_00200650(int tex, int x, int y, int w, int h, int alpha) {
    char *arena = D_0019A4E8_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << e[6];
    th = 1 << e[7];
    D_00161000[0] = 0x10000007;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000007;

    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    p[0] = 0xB400000000008001L;
    p[1] = 0x53535353106L;
    p[2] = GetFrameTex(tex);
    p[3] = 0x154;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = tw * 16;
    p[6] = (x * 16 + D_0013E600[4] - 8)
         | ((long)((y + h) * 16 + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (th << 20) + tw * 16;
    p[8] = (x * 16 + D_0013E600[4] - 8)
         | ((long)(y * 16 + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    p[10] = ((x + w) * 16 + D_0013E600[4] - 8)
          | ((long)((y + h) * 16 + D_0013E600[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[11] = th << 20;
    p[12] = ((x + w) * 16 + D_0013E600[4] - 8)
          | ((long)(y * 16 + D_0013E600[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[13] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x70);
}

LINKER_REMNANT("asm/remnants/text", func_002008B0);

/* func_00200468 with the position and size already in 16ths of a pixel. */
void func_002008B8(int tex, int x, int y, int w, int h, int alpha) {
    char *arena = D_0019A4E8_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << e[6];
    th = 1 << e[7];
    D_00161000[0] = 0x10000005;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000005;

    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = 0;
    p[6] = (x + D_0013E600[4] - 8)
         | ((long)(y + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (th << 20) + tw * 16;
    p[8] = (x + w + D_0013E600[4] - 8)
         | ((long)(y + h + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x50);
}

/* func_002008B8 with an explicit texture window: UVs run from (u, v) to
   (u, v) plus the texture's size in 16ths. */
void func_00200A90(int tex, int x, int y, int w, int h, int u, int v, int alpha) {
    char *arena = D_0019A4E8_raw;
    unsigned char *e = (unsigned char *)(*(char **)(arena + 0x24)
        + *(short *)(*(char **)(arena + 0x20) + tex * 4 + 2) * 8);
    int th;
    int tw;
    int *base;
    long *p;

    tw = 1 << (e[6] + 4);
    th = 1 << (e[7] + 4);
    D_00161000[0] = 0x10000005;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000005;

    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = u | ((long)v << 16);
    p[6] = (x + D_0013E600[4] - 8)
         | ((long)(y + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (u + tw) | ((long)(v + th) << 16);
    p[8] = (x + w + D_0013E600[4] - 8)
         | ((long)(y + h + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x50);
}

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];

/* Appends a 1-tag + 5-quadword PACKED GIF packet to D_00161000: a GIFtag
   (0x10000005 / 0 / 0 / 0x50000005), fixed GS register data, arg0
   verbatim, a colour of 0x7F7F7F with alpha arg9, then a UV/XYZ pair
   per corner: UV (arg7, arg8) and (arg7 + (1 << (arg3 + 4)),
   arg8 + (1 << (arg4 + 4))); XY (arg1, arg2) and (arg1 + arg5,
   arg2 + arg6), offset by D_0013E600[4]/[5] - 8, with Z from the HUD
   arena's +0xC word. The payload is written through the advanced
   D_00161000 (retail stores relative to it), and the arena is reached
   through a local pointer so its address is built in a register. */
void func_00200CA0(long arg0, int arg1, int arg2, int arg3, int arg4,
                    int arg5, int arg6, int arg7, int arg8, int arg9) {
    int *base;
    long *p;
    char *arena = D_0019A4E8_raw;

    D_00161000[0] = 0x10000005;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000005;

    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    p[0] = 0x7400000000008001L;
    p[1] = 0x5353106;
    p[2] = arg0;
    p[3] = 0x156;
    p[4] = ((long)arg9 << 24) | 0x7F7F7F;
    p[5] = arg7 | ((long)arg8 << 16);
    p[6] = (arg1 + D_0013E600[4] - 8)
         | ((long)(arg2 + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = (arg7 + (1 << (arg3 + 4)))
         | ((long)(arg8 + (1 << (arg4 + 4))) << 16);
    p[8] = (arg1 + arg5 + D_0013E600[4] - 8)
         | ((long)(arg2 + arg6 + D_0013E600[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x50);
}

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];
extern char D_0019A4E8_raw[] __asm__("D_0019A4E8");
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(float *, float *, float *);
extern void func_001F9BF0(float *, float *, float *);

/* func_00200CA0's rotated sibling: a 1-tag + 6-quadword PACKED GIF
   packet for a w x h sprite at (x, y) turned by rot. The four corners
   are pos +- a +- b with a = h * (sin, cos) and b = w * (-cos, sin)
   (func_001F9BD8 adds, func_001F9BF0 subtracts vectors), each packed as
   XYZ2 with D_0013E600[4]/[5] - 8 and the HUD arena's +0xC depth; UVs
   run from (0, 0) to (u * 16, v << 20). */
void func_00200E38(int u, int v, long tex, float x, float y, float w, float h, float rot) {
    float a[4];
    float b[4];
    float pos[4];
    float c0[4];
    float c1[4];
    float c2[4];
    float c3[4];
    int *base;
    long *p;
    int *vp;
    char *arena;

    pos[0] = x;
    pos[1] = y;
    a[0] = h * FastSin(rot);
    a[1] = h * FastCos(rot);
    b[0] = -w * FastCos(rot);
    b[1] = w * FastSin(rot);
    FastVecAdd(c0, pos, a);
    FastVecSub(c0, c0, b);
    FastVecAdd(c1, pos, a);
    FastVecAdd(c1, c1, b);
    FastVecSub(c2, pos, a);
    FastVecSub(c2, c2, b);
    FastVecSub(c3, pos, a);
    FastVecAdd(c3, c3, b);

    D_00161000[0] = 0x10000007;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000007;
    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    vp = D_0013E600;
    arena = D_0019A4E8_raw;
    p[0] = 0xB400000000008001L;
    p[1] = 0x53535353106L;
    p[2] = tex;
    p[3] = 0x154;
    p[4] = 0x807F7F7FL;
    p[5] = u * 16;
    p[6] = ((int)c0[0] + vp[4] - 8)
         | ((long)((int)c0[1] + vp[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[7] = v * 0x100000 + u * 16;
    p[8] = ((int)c1[0] + vp[4] - 8)
         | ((long)((int)c1[1] + vp[5] - 8) << 16)
         | ((long)*(int *)(arena + 0xC) << 32);
    p[9] = 0;
    p[10] = ((int)c2[0] + vp[4] - 8)
          | ((long)((int)c2[1] + vp[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[11] = v * 0x100000;
    p[12] = ((int)c3[0] + vp[4] - 8)
          | ((long)((int)c3[1] + vp[5] - 8) << 16)
          | ((long)*(int *)(arena + 0xC) << 32);
    p[13] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x70);
}

typedef struct {
    char pad0[0xC];
    int depth;
} HudDepthView;

/* HUD sprite packet with explicit UV corners, adapted from Lombyte (MIT). */
void func_00201190(int tex, int x0, int y0, int x1, int y1,
                   int u0, int v0, int u1, int v1, int alpha) {
    int *base;
    long *p;
    HudDepthView *arena;

    D_00161000[0] = 0x10000005;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000005;
    base = D_00161000;
    D_00161000 = base + 4;
    p = (long *)D_00161000;
    p[0] = ((long)0xE800 << 47) | 0x8001;
    p[1] = 0x5353106;
    p[2] = GetFrameTex(tex);
    p[3] = 0x156;
    p[4] = ((long)alpha << 24) | 0x7F7F7F;
    p[5] = u0 | ((long)v0 << 16);
    arena = (HudDepthView *)D_0019A4E8_raw;
    p[6] = (x0 + D_0013E600[4] - 8)
         | ((long)(y0 + D_0013E600[5] - 8) << 16)
         | ((long)arena->depth << 32);
    p[7] = u1 | ((long)v1 << 16);
    p[8] = (x1 + D_0013E600[4] - 8)
         | ((long)(y1 + D_0013E600[5] - 8) << 16)
         | ((long)arena->depth << 32);
    p[9] = 0;
    D_00161000 = (int *)((char *)D_00161000 + 0x50);
}

extern int *D_00161000 MACRO_ADDR;
extern void func_00122630(void *, int, int, int, int, int, int, int);
extern void func_00118D80(int);
extern void func_00122958(void *, void *);

/* Hud_sendTexture(char *tex, int x, int y, int levelShift, int sizeShift,
   int useStackBuf): builds a mip level count (clamped to >=1) and a
   packed GIF NLOOP for the closing tag, then either (useStackBuf==0)
   opens a GS packet in D_00161000 and writes straight into it, or
   builds into a stack buffer; either way calls func_00122630 to fill it
   with (x, levels, y, 1<<levelShift, 1<<sizeShift), then either closes
   the GS packet in place or flushes the cache and hands the stack
   buffer to func_00122958.

   p7 is set before the branch, as retail does in its delay slot, but
   truncated to short after it, with the other short arguments. */
void func_00201348(char *arg0, int arg1, int arg2, int arg3, int arg4, int arg5) {
    char buf[0x60];
    int nloop2;
    int levels;
    int p7;
    short p5, p6, p10, p11;
    void *dst;
    int *base;

    nloop2 = 1 << (arg3 + arg4 - 4);
    levels = (1 << arg3) >> 6;
    if (levels <= 0) {
        levels = 1;
    }

    p7 = arg2;
    if (arg5 == 0) {
        D_00161000[0] = 0x10000006;
        D_00161000[1] = 0;
        D_00161000[2] = 0;
        D_00161000[3] = 0x50000006;
        base = D_00161000;
        dst = base + 4;
        D_00161000 = base + 0x1C;
    } else {
        dst = buf;
    }

    p5 = arg1;
    p6 = levels;
    p7 = (short)p7;
    p10 = 1 << arg3;
    p11 = 1 << arg4;

    func_00122630(dst, p5, p6, p7, 0, 0, p10, p11);

    if (arg5 == 0) {
        int *cur = D_00161000;
        cur[0] = 0x30000000 | nloop2;
        D_00161000[1] = (int)arg0;
        D_00161000[2] = 0;
        D_00161000[3] = 0x50000000 | nloop2;
        D_00161000 = D_00161000 + 4;
    } else {
        func_00118D80(0);
        func_00122958(dst, arg0);
    }
}

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];

/* func_002017C8's sibling: same 1-tag + 3-quadword PACKED GIF packet, but
   register-id constant 0x41 (not 0x46) at +0x20, and a FIXED mask
   0x00FFFFF000000000 in the vertices' upper 32 bits instead of a Z
   parameter -- one fewer int argument than func_002017C8. */
void func_002014B8(int arg0, int arg1, int arg2, int arg3, long arg4,
                    int arg5) {
    int *base;

    D_00161000[0] = 0x10000003;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000003;

    base = D_00161000;
    D_00161000 = base + 4;
    *(long *)((char *)base + 0x10) = 0x4400000000000001UL;
    *(long *)((char *)base + 0x18) = 0x4410;
    *(long *)((char *)base + 0x20) = 0x41;
    *(long *)((char *)base + 0x28) = arg4;
    if (arg5 != 0) {
        *(long *)((char *)base + 0x30) =
            (arg0 + D_0013E600[4] - 8)
            | ((long)(arg1 + D_0013E600[5] - 8) << 16)
            | 0x00FFFFF000000000L;
        *(long *)((char *)base + 0x38) =
            (arg2 + D_0013E600[4] - 8)
            | ((long)(arg3 + D_0013E600[5] - 8) << 16)
            | 0x00FFFFF000000000L;
    } else {
        *(long *)((char *)base + 0x30) =
            (arg0 * 16 + D_0013E600[4] - 0x10)
            | ((long)(arg1 * 16 + D_0013E600[5] - 0x10) << 16)
            | 0x00FFFFF000000000L;
        *(long *)((char *)base + 0x38) =
            (arg2 * 16 + D_0013E600[4] - 0x10)
            | ((long)(arg3 * 16 + D_0013E600[5] - 0x10) << 16)
            | 0x00FFFFF000000000L;
    }
    D_00161000 = (int *)((char *)D_00161000 + 0x30);
}

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];

/* func_002014B8's sibling (register-id constant 0x46, like
   func_002017C8's, instead of 0x41): same 1-tag + 3-quadword PACKED GIF
   packet with a FIXED mask 0x00FFFFF000000000 in the vertices' upper 32
   bits instead of a Z parameter. */
void func_00201640(int arg0, int arg1, int arg2, int arg3, long arg4,
                    int arg5) {
    int *base;

    D_00161000[0] = 0x10000003;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000003;

    base = D_00161000;
    D_00161000 = base + 4;
    *(long *)((char *)base + 0x10) = 0x4400000000000001UL;
    *(long *)((char *)base + 0x18) = 0x4410;
    *(long *)((char *)base + 0x20) = 0x46;
    *(long *)((char *)base + 0x28) = arg4;
    if (arg5 != 0) {
        *(long *)((char *)base + 0x30) =
            (arg0 + D_0013E600[4] - 8)
            | ((long)(arg1 + D_0013E600[5] - 8) << 16)
            | 0x00FFFFF000000000L;
        *(long *)((char *)base + 0x38) =
            (arg2 + D_0013E600[4] - 8)
            | ((long)(arg3 + D_0013E600[5] - 8) << 16)
            | 0x00FFFFF000000000L;
    } else {
        *(long *)((char *)base + 0x30) =
            (arg0 * 16 + D_0013E600[4] - 0x10)
            | ((long)(arg1 * 16 + D_0013E600[5] - 0x10) << 16)
            | 0x00FFFFF000000000L;
        *(long *)((char *)base + 0x38) =
            (arg2 * 16 + D_0013E600[4] - 0x10)
            | ((long)(arg3 * 16 + D_0013E600[5] - 0x10) << 16)
            | 0x00FFFFF000000000L;
    }
    D_00161000 = (int *)((char *)D_00161000 + 0x30);
}

extern int *D_00161000 MACRO_ADDR;
extern int D_0013E600[];

/* Appends a 1-tag + 3-quadword PACKED GIF packet to D_00161000: a GIFtag
   (0x10000003 / 0 / 0 / 0x50000003), then TEX-ish GS register data at
   +0x10/+0x18/+0x20, arg4 verbatim at +0x28, then two packed XYZ2-style
   vertices at +0x30/+0x38: X = argX(+D_0013E600[4])-8, Y =
   argY(+D_0013E600[5])-8, Z = arg5<<32, when arg6 != 0 (raw coordinates);
   or the same with argX/argY scaled by 16 and offset -0x10 when arg6==0
   (tile coordinates). D_00161000 is re-read at every use (never cached
   across a store to it), matching this file's other packet builders. */
void func_002017C8(int arg0, int arg1, int arg2, int arg3, long arg4,
                    int arg5, int arg6) {
    int *base;

    D_00161000[0] = 0x10000003;
    D_00161000[1] = 0;
    D_00161000[2] = 0;
    D_00161000[3] = 0x50000003;

    base = D_00161000;
    D_00161000 = base + 4;
    *(long *)((char *)base + 0x10) = 0x4400000000000001UL;
    *(long *)((char *)base + 0x18) = 0x4410;
    *(long *)((char *)base + 0x20) = 0x46;
    *(long *)((char *)base + 0x28) = arg4;
    if (arg6 != 0) {
        *(long *)((char *)base + 0x30) =
            (arg0 + D_0013E600[4] - 8)
            | ((long)(arg1 + D_0013E600[5] - 8) << 16)
            | ((long)arg5 << 32);
        *(long *)((char *)base + 0x38) =
            (arg2 + D_0013E600[4] - 8)
            | ((long)(arg3 + D_0013E600[5] - 8) << 16)
            | ((long)arg5 << 32);
    } else {
        *(long *)((char *)base + 0x30) =
            (arg0 * 16 + D_0013E600[4] - 0x10)
            | ((long)(arg1 * 16 + D_0013E600[5] - 0x10) << 16)
            | ((long)arg5 << 32);
        *(long *)((char *)base + 0x38) =
            (arg2 * 16 + D_0013E600[4] - 0x10)
            | ((long)(arg3 * 16 + D_0013E600[5] - 0x10) << 16)
            | ((long)arg5 << 32);
    }
    D_00161000 = (int *)((char *)D_00161000 + 0x30);
}

INCLUDE_ASM("asm/nonmatchings/text", func_00201948);

extern int func_00200198(int, int);
extern void func_00200468(int, int, int, int, int, int);
extern void func_00200650(int, int, int, int, int, int);

/* Draw a 3-part stretchable bar: left cap, stretched middle, right cap
 * (mirrored via func_00200650), all using the same GetIconFrame glyph
 * in its two variants. */
void func_00201960(int a0, int a1, int a2, int a3, int a4) {
    int v0 = GetIconFrame(0x7580, 0);
    int v1 = GetIconFrame(0x7580, 1);

    HudSprite(v1, a0, a1, 0x20, a3, a4);
    HudSprite(v0, a0 + 0x20, a1, a2 - 0x40, a3, a4);
    a0 = a0 + a2;
    func_00200650(v1, a0 - 0x20, a1, 0x20, a3, a4);
}

/*
 * Close, not exact (32/168, 19%), same size so harmless to anything
 * after it -- kept on the func_002094E0 precedent (13/64, 20%). Logic is
 * certain: clamp the top byte of `c` to 0x50, draw once with the colour
 * masked to its alpha byte, then a second pass offset from the first
 * call's return, then draw again unmasked. Five-argument calls -- EABI
 * passes the first eight integer args in $4-$11.
 *
 * Residual is the known allocator/constant-scheduling question, not
 * source shape: retail hoists the `lui $6,0xFF00` mask in among the
 * register spills and assigns $17-$20 to a,b,c,d in argument order,
 * where this compiler schedules the `slti` into that slot and picks a
 * different arg-to-saved-register mapping. Hoisting the mask into its
 * own local was tried and changed nothing at all.
 */
void func_00201A38(int a, int b, int c, int d) {
    int hi = c >> 24;
    int m = c & 0xFF000000;
    int t;
    if (hi >= 0x51) hi = 0x50;
    t = FontPrintCenterLarge(a + 1, b + 1, m, d, -1) - 0x20;
    draw_stretchable_ui_frame(t, b - 8, (a - t) * 2, 0x20, hi);
    FontPrintCenterLarge(a, b, c, d, -1);
}

INCLUDE_ASM("asm/nonmatchings/text", func_00201AE0);

INCLUDE_ASM("asm/nonmatchings/text", func_00201AF0);
