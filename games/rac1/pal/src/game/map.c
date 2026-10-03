#include "common.h"
#include "structs.h"

/*
 * map.cpp in the original source; text 0x205520-0x2071A8.
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
extern int D_001941CC NOT_SDA;
extern int D_0019A4E8 NOT_SDA;
extern int func_001FF668(int);
typedef struct {
    char b[0x13];
} Cfg13;
extern Cfg13 D_0019A540 NOT_SDA;
extern Cfg13 D_001E7DD8 NOT_SDA;
extern int func_00116810(void);
extern void func_001166FC(Cfg13 *, void *);
extern short D_0015F9D0;
extern void func_00201960(int, int, int, int, int);
extern void func_002023E0(int);
extern void func_002027C0(int);
extern void func_00204FC0(void *);
extern int D_0018CC20 NOT_SDA;
extern int D_001941C8 NOT_SDA;
extern int D_0016100C;

extern int func_001F9968(int);
extern int D_0015EF74 MACRO_ADDR;
extern int D_0015F558 MACRO_ADDR;
typedef struct {
    int addr;            /* +0 */
    short unk4;          /* +4 */
    short cbp;           /* +6 */
    int unk8;            /* +8 */
    unsigned char tw;    /* +C */
    unsigned char th;    /* +D */
    short tbp;           /* +E */
} TexSlot;
extern TexSlot D_0018D140[];
typedef struct {
    char *clut;          /* 0x00 */
    char *pix;           /* 0x04 */
    char pad08[0xC];
    int clutSize;        /* 0x14 */
    char pad18[0x34];
    int tw;              /* 0x4C */
    int th;              /* 0x50 */
    char pad54[0xC];
} TexDesc;

/* Loads a 256-colour texture file p (width at +8, height at +0xC, CLUT
   at +0x20, pixels after it): the CLUT and the pixels take the next VRAM
   space at D_0015EF74 (cbp, then tbp 0x400 later, then the allocator
   moves on by 1 << (tw + th)), and the texture is registered and its GS
   TEX0 value returned as in func_00205660. The allocator update comes
   before the TEX0 value and the buffer width is shifted back into w:
   both decide which of tw and the constant 1 local-alloc serves first. */
long func_00205520(char *p) {
    TexDesc t;
    int w;
    int cbp;
    int tbp;
    int addr;
    long reg;

    t.clut = p + 0x20;
    t.clutSize = 0x400;
    t.tw = log2dim(*(int *)(p + 8));
    t.th = log2dim(*(int *)(p + 0xC));
    t.pix = p + 0x20 + t.clutSize;
    addr = D_0015EF74;
    cbp = addr >> 8;
    addr += 0x400;
    tbp = addr >> 8;
    D_0015EF74 = addr + (1 << (t.tw + t.th));
    w = t.tw - 6;
    if (w < 0) {
        w = 0;
    }
    w = 1 << w;
    reg = (long)tbp | ((long)w << 14) | ((long)0x13 << 20)
        | ((long)t.tw << 26) | ((long)t.th << 30) | ((long)1 << 34)
        | ((long)cbp << 37) | ((long)4 << 61);
    if (D_0015F558 < 0x40) {
        D_0018D140[D_0015F558].addr = (int)t.clut;
        D_0018D140[D_0015F558].cbp = cbp;
        D_0018D140[D_0015F558].unk4 = 0;
        D_0018D140[D_0015F558].unk8 = (int)t.pix;
        D_0018D140[D_0015F558].tw = t.tw;
        D_0018D140[D_0015F558].th = t.th;
        D_0018D140[D_0015F558].tbp = tbp;
        D_0015F558++;
    }
    return reg;
}

/* Registers an 8-bit texture: returns its GS TEX0 value (tbp = tex >> 8,
   tbw = 1 << max(tw - 6, 0), PSMT8, tw, th, tcc 1, cbp = clut >> 8,
   cld 4) and, while there is room (64 entries), records it in the next
   D_0018D140 slot. The buffer width is shifted back into w itself: the
   shift then writes w's register and the constant 1 keeps its own. */
long func_00205660(int tw, int th, int a2, int a3, int clut, int tex) {
    int w = tw - 6;
    long reg;
    int tbp = tex >> 8;
    int cbp = clut >> 8;

    if (w < 0) {
        w = 0;
    }
    w = 1 << w;
    reg = (long)tbp | ((long)w << 14) | ((long)0x13 << 20)
        | ((long)tw << 26) | ((long)th << 30) | ((long)1 << 34)
        | ((long)cbp << 37) | ((long)4 << 61);
    if (D_0015F558 < 0x40) {

        D_0018D140[D_0015F558].addr = a2;
        D_0018D140[D_0015F558].cbp = cbp;
        D_0018D140[D_0015F558].unk4 = 0;
        D_0018D140[D_0015F558].unk8 = a3;
        D_0018D140[D_0015F558].tw = tw;
        D_0018D140[D_0015F558].th = th;
        D_0018D140[D_0015F558].tbp = tbp;
        D_0015F558++;
    }
    return reg;
}
__asm__(".section .text\n\tnop\n");

extern int D_001A0468[];

int func_00205728(int arg0) {
    int *a = D_001A0468;
    int *b = D_001A0468 + 5;
    int i = 0;
    do {
        int idx = 4 - i;
        if (arg0 == 0) idx = i;
        if (a[idx] != 0 && b[idx] == -1) {
            return idx;
        }
        i++;
    } while (i < 5);
    return -1;
}

extern void func_00205830(int a, int b);

/* D_001A01F0 typed as the object it is, so that the two 5/6-entry int
   arrays at +0x278 and +0x28C are members rather than constants added
   to an index. See the note on func_00205790 -- this is what makes it
   match. Aliased rather than renamed because the rest of this file
   still reaches the same symbol as a flat int array. */
typedef struct {
    int _pad0[0x9E];
    int use[5];   /* +0x278 */
    int flags[5]; /* +0x28C */
    int sel;      /* +0x2A0 -- index of the active slot, -1 for none */
    int size[5];  /* +0x2A4 */
} PadSlots;
extern PadSlots D_001A01F0_slots __asm__("D_001A01F0");

/*
 * Allocate a slot. func_00205728(1) gets first refusal; if it returns
 * nonzero that IS the answer. Otherwise scan slots 1..4 for one that is
 * neither flagged 0x1000 nor empty, and hand it to func_00205830, which
 * compacts entry `i` down onto entry 0. Falling out of the loop at i==5
 * still calls func_00205830(0, 5) -- retail shares that call site with
 * the break, so the source must too.
 *
 * Byte-exact, registers included, and it took three things:
 *
 * 1. ONE variable for the callee's result and the loop counter. Retail
 *    keeps both in $16 and pays for a `daddu $16,$2,$0` that separate
 *    locals would not need. Two locals is 4 bytes SHORT.
 *
 * 2. The peeled first test is not a peeled iteration in the source: it
 *    is gcc's while -> do-while rotation, whose entry guard `1 < 5`
 *    folds away, leaving the i==1 addresses as the constants 0x290 and
 *    0x27C. A plain `while` starting at i = 1 produces it for free.
 *
 * 3. TYPE THE BASE AS A STRUCT. Three spellings, against retail's 160:
 *      D_001A01F0[0xA3 + i]              172  adds the constant to the
 *                                             index and THEN shifts,
 *                                             once per array
 *      int *p = D_001A01F0 + i; p[0xA3]  164  right inside the loop,
 *                                             but the constant-folded
 *                                             i==1 peel then needs its
 *                                             own `addu $3,$3,4`
 *                                             instead of folding into
 *                                             the lw displacement
 *      struct member .flags[i]           160  EXACT
 *    Only the struct gives base-first `sll`/`addu` with the array's
 *    byte offset left in the load, in BOTH the loop and the folded
 *    peel. This widens the existing "type the table, don't rewrite the
 *    addition" lever from a stride to a base: where retail shows
 *    `sll idx,2` / `addu base` / `lw CONST(reg)`, that CONST is a
 *    member offset, so declare the member.
 *
 *    It does not contradict the base-pointer rule on func_00205830
 *    below, where three arrays come off one live base in straight-line
 *    code and naming the base wins. The rule covering both is: describe
 *    the memory, not the arithmetic.
 *
 * Watch the 164-byte middle spelling: its extra word was not an extra
 * instruction at all. The body was 40 words either way, but the odd
 * word count ahead of the loop label made gcc's `.p2align 3` emit a
 * real nop -- and internal alignment padding sits INSIDE the .ent/.end
 * pair, so it counts toward the symbol size and toward the bytes. A
 * size miss of exactly 4 with an otherwise correct instruction stream
 * means a misaligned block, not a missing instruction.
 */
int func_00205790(void) {
    int i;

    i = func_00205728(1);
    if (i != 0) {
        return i;
    }
    i = 1;
    while (i < 5) {
        if ((D_001A01F0_slots.flags[i] & 0x1000) == 0 &&
            D_001A01F0_slots.use[i] != 0) {
            break;
        }
        i++;
    }
    move_map_entry_slot(0, i);
    return i;
}

extern int D_001A01F0[];

extern void func_001F9A98(void *, void *, int);
extern int D_001A01F0[];

/* Same three parallel arrays func_002054E0 walks: 0x278/4 = 0x9E,
   0x28C/4 = 0xA3 and 0x2A4/4 = 0xA9 into D_001A01F0. Moves entry `b`
   onto entry `a` and frees `b`.

   Where the pointers are derived is load-bearing. Naming the unoffset
   `base` keeps the symbol's address live and derives each array with
   its own addiu (`&D_001A01F0[0x9E]` folds the offset into %lo and
   loses an instruction). `flags` is derived only after the call, as in
   retail: deriving it up front with the others swaps a*4 and b*4
   between $s1 and $s2. */
void func_00205830(int a, int b) {
    int *base = D_001A01F0;
    int *dst = base + 0x9E;
    int *size = base + 0xA9;
    int *flags;

    FastMemCopy((void *)dst[a], (void *)dst[b], size[b] << 4);

    flags = base + 0xA3;
    flags[a] = flags[b];
    size[a] = size[b];
    flags[b] = -1;
}

extern int D_001A01F0[];

int func_002058D0(int arg0) {
    int i;
    for (i = 0; i < 5; i++) {
        if (D_001A01F0[0x9E + i] != 0 && D_001A01F0[0xA3 + i] == arg0) {
            return i;
        }
    }
    return -1;
}

extern unsigned char D_0013D5E9 NOT_SDA;
/* The MACRO_ADDR view declared further down, needed here already. */
extern int *D_001602E0_m __asm__("D_001602E0") MACRO_ADDR;

/* Picks the map to load next: the current map id (D_001A01F0+0x224,
   +0x100 when D_0013D5E9 is set) unless func_002058D0 finds it already
   in a slot, otherwise the nearest entry of the 20-id list D_001602E0
   around the current one (+1, -1, +2, -2, +3, -3) that is not loaded;
   -1 if none. The two bounds tests are nested so that fold does not
   merge them into one unsigned compare, and the step update is the
   ternary that gives retail's select (slti/movn). */
int func_00205918(void) {
    char *m = (char *)D_001A01F0;
    int off = gHaveMapOMatic ? 0x100 : 0;
    int key;
    int i;
    int step;

    key = *(int *)(m + 0x224) + off;
    if (findMapSlot(key) == -1) {
        return key;
    }
    i = 0;
    if (*(int *)(m + 0x224) < 20) {
        while (D_001602E0_m[i] != *(int *)(m + 0x224)) {
            i++;
        }
    }
    step = 1;
    do {
        int j = i + step;
        if (j >= 0) {
            if (j < 20 && D_001602E0_m[j] != 0) {
                key = D_001602E0_m[j] + off;
                if (findMapSlot(key) == -1) {
                    return key;
                }
            }
        }
        step = (step > 0) ? -step : 1 - step;
    } while (step != 4);
    return -1;
}

extern int *D_001602E0;

extern int *D_001602E0_m __asm__("D_001602E0") MACRO_ADDR;

/* The global is read at both uses in the loop, giving retail's two
   pointers, and through a MACRO_ADDR alias. */
int func_00205A50(int arg0) {
    int i;

    for (i = 0; D_001602E0_m[i] != 0 && i < 20; i++) {
        if (D_001602E0_m[i] == arg0) {
            return i;
        }
    }
    return -1;
}

extern int func_001F9B70(int); /* abs */

/* Picks the map slot to reuse (D_001A01F0's five slots: in use at
   [0x9E + i], map id at [0xA3 + i], active slot [0xA8]). With no current
   map (+0x224 == 0) a used, inactive slot holding no map (-1) is looked
   for from the top first. Then the slots are scanned from the bottom:
   such a slot is taken, or else the one whose map is furthest from the
   current map in the D_001602E0 list (func_00205A50) is freed; 1 if the
   current map is not in the list.
   The slots are indexed off the global itself, as in func_002058D0, so
   that loop.c keeps one pointer per loop; the final store goes through
   the struct view, which adds the base first. */
int func_00205AA8(void) {
    int best = 0;
    int bestIdx = -1;
    int cur;

    if (D_001A01F0[0x89] == 0) {
        int i;
        for (i = 4; i >= 0; i--) {
            if (D_001A01F0[0x9E + i] != 0 && i != D_001A01F0[0xA8]
                && D_001A01F0[0xA3 + i] == -1) {
                return i;
            }
        }
    }
    {
        int i;
        cur = find_id_in_terminated_table(D_001A01F0[0x89]);
        if (cur == -1) {
            return 1;
        }
        for (i = 0; i < 5; i++) {
            if (D_001A01F0[0x9E + i] != 0 && i != D_001A01F0[0xA8]) {
                int d;
                if (D_001A01F0[0xA3 + i] == -1) {
                    return i;
                }
                d = func_001F9B70(find_id_in_terminated_table(D_001A01F0[0xA3 + i] & 0xFF) - cur);
                if (best < d) {
                    best = d;
                    bestIdx = i;
                }
            }
        }
    }
    if (bestIdx == -1) {
        bestIdx = 0;
    }
    ((PadSlots *)D_001A01F0)->flags[bestIdx] = -1;
    return bestIdx;
}

void func_00205C08(unsigned char *dst, unsigned char *a, unsigned char *b,
                   unsigned char *mask) {
    int i;
    int j;
    int bit;
    unsigned char *next;

    for (i = 0; i <= 0x7FFF; i++) {
        bit = 1;
        next = mask + 1;
        for (j = 7; j >= 0; j--) {
            if (*mask & bit) {
                *dst = *a;
            } else {
                *dst = *b;
            }
            bit <<= 1;
            a++;
            b++;
            dst++;
        }
        mask = next;
    }
}

extern void func_00209040(void);
extern char D_0013CA40[];

/* Map screen pan/zoom update: returns 1 while the pad word (D_0013CA40
   +0x1A4) has 0x500 set, 0 when the map object D_001A01F0 has nothing open
   (+0x24) or no slot (+0x228 < 0). Otherwise scales the slot's zoom (+0xB4
   floats) by 1 - speed*0.02 and clamps it to [0.65, 4], pans x/y (+0x104 /
   +0x154, 16.15 fixed point) by the pad velocities times 3000000/zoom, and
   clamps them to [lo, 0x10000000 - lo] with lo = (A/zoom) << 15, A = 0 for
   x and 1280 for y. The slot is re-read at every use, the three array
   bases are locals set where retail first builds them, and the x bound is
   computed before the y bound (the scheduler emits them in reverse). */
int func_00205C70(void) {
    char *pad;
    char *m;
    float *zoom;
    int *xs;
    int *ys;

    func_00209040();
    pad = D_0013CA40;
    if (*(int *)(pad + 0x1A4) & 0x500) {
        return 1;
    }
    m = (char *)D_001A01F0;
    if (*(int *)(m + 0x24) == 0) {
        return 0;
    }
    if (*(int *)(m + 0x228) < 0) {
        return 0;
    }
    zoom = (float *)(m + 0xB4);
    zoom[*(int *)(m + 0x228)] *= 1.0f - *(float *)(pad + 0x104) * 0.02f;
    if (zoom[*(int *)(m + 0x228)] > 4.0f) {
        zoom[*(int *)(m + 0x228)] = 4.0f;
    }
    if (zoom[*(int *)(m + 0x228)] < 0.65f) {
        zoom[*(int *)(m + 0x228)] = 0.65f;
    }
    xs = (int *)(m + 0x104);
    ys = (int *)(m + 0x154);
    {
        float scale = 3000000.0f / zoom[*(int *)(m + 0x228)];
        xs[*(int *)(m + 0x228)] += (int)(*(float *)(pad + 0x108) * scale);
        ys[*(int *)(m + 0x228)] += (int)(*(float *)(pad + 0x10C) * scale);
    }
    {
        float z = zoom[*(int *)(m + 0x228)];
        int xlo = (int)(0.0f / z) << 15;
        int ylo = (int)(1280.0f / z) << 15;
        int xhi = 0x10000000 - xlo;
        int yhi = 0x10000000 - ylo;

        if (xs[*(int *)(m + 0x228)] < xlo) {
            xs[*(int *)(m + 0x228)] = xlo;
        }
        if (xs[*(int *)(m + 0x228)] > xhi) {
            xs[*(int *)(m + 0x228)] = xhi;
        }
        if (ys[*(int *)(m + 0x228)] < ylo) {
            ys[*(int *)(m + 0x228)] = ylo;
        }
        if (ys[*(int *)(m + 0x228)] > yhi) {
            ys[*(int *)(m + 0x228)] = yhi;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00205E70); /* UNK_NoMapAvailable */

extern void func_00207090(unsigned char *dst, int row, unsigned char *src,
                          short *offs);

/* Builds the map's 1-bit coverage mask: each of the 256 rows is decoded
   into the scratchpad (func_00207090), then every 4-bit pixel that is
   not 0 sets its bit, 32 pixels to a word, one word every 16 words of
   the output, 16 rows side by side (dst + row % 16 + row / 16 * 512).
   The remainder is computed before the quotient (retail's surviving
   copy of the quotient), and the scratchpad is a pointer local (retail
   adds the base first). */
void func_00206F40(int *dst, unsigned char *src, short *offs) {
    int row;
    unsigned char *scr = (unsigned char *)0x70000000;

    for (row = 0; row < 0x100; row++) {
        int *p;
        int j;

        func_00207090(scr, row, src, offs);
        {
            int r = row % 16;
            int q = row / 16;
            p = dst + (r + q * 512);
        }
        for (j = 0; j < 32; j++) {
            int bits = 0;
            int k;

            for (k = 0; k < 32; k++) {
                unsigned char b = scr[j * 16 + (k >> 1)];
                int v;

                if (k & 1) {
                    v = b >> 4;
                } else {
                    v = b & 0xF;
                }
                if (v != 0) {
                    bits |= 1 << k;
                }
                if ((k & 31) == 31) {
                    *p++ = bits;
                    bits = 0;
                }
            }
            p += 15;
        }
    }
}

/* Unpacks run-length coded 4-bit pixels (the map image) into dst: row's
   codes run from src + offs[row - 1] (src + 0x200 for row 0) to src +
   offs[row], three nibbles each, a pixel value and a count byte (0 means
   0x100). Pixels are packed two to a byte, low nibble first, and `half`
   carries an odd run's last pixel into the next run. The count starts
   at 0x100 in its own statement so that it stays a plain set in the loop
   (a cmove constant would be hoisted); the two bytes are read low byte
   first and `half` is cleared before the count drops, retail's order. */
void func_00207090(unsigned char *dst, int row, unsigned char *src, short *offs) {
    int half = 0;
    unsigned char *p;
    int n;
    int i;

    if (row != 0) {
        p = src + offs[row - 1];
    } else {
        p = src + 0x200;
    }
    n = (src + offs[row] - p) * 2 / 3;
    for (i = 0; i < n; i++) {
        int nib = i * 3;
        unsigned char *q = p + (nib >> 1);
        unsigned char v = q[0];
        unsigned char c = q[1];
        unsigned char pair;
        int count;

        if (nib & 1) {
            v >>= 4;
        } else {
            c = (c << 4) | (v >> 4);
            v &= 0xF;
        }
        pair = v | (v << 4);
        count = 0x100;
        if (c != 0) {
            count = c;
        }
        if (half) {
            *dst |= v << 4;
            dst++;
            half = 0;
            count--;
        }
        if (count != 0) {
            do {
                *dst = pair;
                count -= 2;
                dst++;
            } while (count > 0);
            if (count != 0) {
                dst--;
                half = 1;
                *dst = v;
            }
        }
    }
}
