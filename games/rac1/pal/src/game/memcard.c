#include "common.h"
#include "structs.h"

/*
 * memcard.cpp in the original source; text 0x209A60-0x20C210.
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
extern int D_001A0468[];
extern void func_00205830(int a, int b);
typedef struct {
    int _pad0[0x9E];
    int use[5];   /* +0x278 */
    int flags[5]; /* +0x28C */
    int sel;      /* +0x2A0 -- index of the active slot, -1 for none */
    int size[5];  /* +0x2A4 */
} PadSlots;
extern PadSlots D_001A01F0_slots __asm__("D_001A01F0");
extern int D_001A01F0[];
extern int *D_001602E0;
extern unsigned char D_0013D49C NOT_SDA;
extern unsigned char D_0013D49D NOT_SDA;
extern unsigned char D_0013D4A5 NOT_SDA;
extern short D_0015FE24;
extern unsigned char D_0013D4AC NOT_SDA;
extern unsigned char D_0013D4AD NOT_SDA;
extern unsigned char D_0013D4AE NOT_SDA;
extern unsigned char D_0013D4AF NOT_SDA;
extern unsigned char D_0013D4B5 NOT_SDA;
extern int D_001A04B4 NOT_SDA;
extern unsigned char D_0013D4C5 NOT_SDA;
extern int D_001414DC NOT_SDA;
extern unsigned char D_0013D4C0 NOT_SDA;
extern unsigned char D_0013D4C1 NOT_SDA;
extern unsigned char D_0013D4C2 NOT_SDA;
extern unsigned char D_0013D4D3 NOT_SDA;
extern unsigned char D_0013D4D4 NOT_SDA;
extern unsigned char D_0013D4D5 NOT_SDA;
extern unsigned char D_0013D4E0;
extern unsigned char D_0013D4DC NOT_SDA;
extern unsigned char D_0013D4DD NOT_SDA;
extern unsigned char D_0013D4DE NOT_SDA;
extern unsigned char D_0013D4DF NOT_SDA;
extern unsigned char D_0013D4E1 NOT_SDA;
extern unsigned char D_0013D4E9 NOT_SDA;
extern unsigned char D_0013D502 NOT_SDA;
extern unsigned char D_0013D503 NOT_SDA;
extern unsigned char D_0013D504 NOT_SDA;
extern unsigned char D_0013D505 NOT_SDA;
extern unsigned char D_0013D50F NOT_SDA;
extern int D_0013D668[];
extern void func_00209040(void);
extern int func_001FAA28(void *dst, int size, int a, int b);
extern void func_00208860(void *dst);
extern short D_0015EE84;
extern int D_0015EE84_far __asm__("D_0015EE84") NOT_SDA;
extern int D_001A0218[] NOT_SDA;
extern void func_00208458(void *, unsigned char *, int);
extern void func_00208688(void *, unsigned char *);
extern char D_0013D390[];
extern short D_0015EFB0;
extern int D_0015EFB4;

extern char D_0013D2D0[];
extern char D_0013D2E8[];
extern char D_0013D300[];
extern char D_0013D320[];
extern char D_0013D340[];
extern char D_0013D370[];
extern void func_00116B00(void *, void *, int);

/* Builds a 13-byte name/version stamp in D_0013D2D0 from the record's
   region byte (arg0[0x12]) and the record's date/version fields, then
   copies that stamp into the six on-disk name buffers. */
void func_00209A60(void *arg0) {
    unsigned char *a = (unsigned char *)arg0;
    unsigned char region = a[0x12];
    int i;

    if (region == 'E') {
        D_0013D2D0[2] = region;
    } else if (region == 'P') {
        D_0013D2D0[2] = 'I';
    }
    for (i = 3; i < 7; i++) {
        D_0013D2D0[i] = a[i + 0xD];
    }
    for (i = 8; i < 0xB; i++) {
        D_0013D2D0[i] = a[i + 0xD];
    }
    for (i = 0xB; i < 0xD; i++) {
        D_0013D2D0[i] = a[i + 0xE];
    }
    func_00116B00(D_0013D2E8, D_0013D2D0, 0xD);
    func_00116B00(D_0013D300, D_0013D2D0, 0xD);
    func_00116B00(D_0013D320, D_0013D2D0, 0xD);
    func_00116B00(D_0013D340, D_0013D2D0, 0xD);
    func_00116B00(D_0013D340 + 0x14, D_0013D2D0, 0xD);
    func_00116B00(D_0013D370, D_0013D2D0, 0xD);
}

extern int func_00124068(int, int, int *, int *, int *);
extern int func_00123F30(int, int *, int *);
extern void func_00122598(int);
extern int func_001241F0(int, int, char *, int, int, void *);
extern char D_0013D2D0[];
extern int D_0013D390_i[] __asm__("D_0013D390");

/* The comparison is bound to a local before the shift:
   `(a < b) << 1` folds into `a < b ? 2 : 0` (li/slt/movn), not retail's
   slti/sll. */
int func_00209BB8(void) {
    int type, free, format, cmd, result;

    result = func_00124068(D_0013D390_i[0], D_0013D390_i[1], &type, &free, &format);
    while (func_00123F30(1, &D_0013D390_i[0x30], &D_0013D390_i[0x31]) == 0) {
        func_00122598(0);
    }
    if (D_0013D390_i[0x31] == -5 || D_0013D390_i[0x31] < -9 || type != 2) {
        return 1;
    }
    if (D_0013D390_i[0x31] != -2 && format != 0) {
        result = func_001241F0(D_0013D390_i[0], D_0013D390_i[1], D_0013D2D0, 0, -1, 0);
        while (func_00123F30(1, &cmd, &result) == 0) {
            func_00122598(0);
        }
        if (result <= 0) {
            int t = free < 350;
            return t << 1;
        }
    }
    return 0;
}

extern int D_001A05C0[];
extern int D_001A08C0[];
extern int func_0020BAD8(int *p);
extern int func_0020BD70(void *src, int i, int *table);
extern int func_001E9730();
extern char D_001E8500[];

/* The counterpart of func_0020BA00 below: both descriptor sizes are
   recomputed and checked against the ones stored in the blob, then the
   blobs are read back in the same order they were written. The read
   cursor is the parameter, which retail keeps in $s2. */
/* memcard_RestoreGame */
void func_00209CE8(int arg0) {
    char *p = (char *)arg0;
    int a;
    int b;
    int i;

    a = memcard_GetDataSize(D_001A05C0);
    b = memcard_GetDataSize(D_001A08C0);
    if (*(int *)p != a || *(int *)(p + 4) != b) {
        STUB_printf(D_001E8500);
        return;
    }
    p += 8;
    memcard_RestoreData(p, 0, D_001A05C0);
    p += a;
    for (i = 0; i < 0x14; i++) {
        memcard_RestoreData(p, i, D_001A08C0);
        p += b;
    }
}

/*
 * 3/168: the only residual is the allocator holding the loaded pointer
 * in $2 where retail uses $3. Everything else, including saving the
 * three globals across func_00209CE8 and the $at store of D_0015EE84,
 * is instruction-for-instruction retail.
 */
extern int D_00137C80[];
extern void func_001FDF10(int, void *, void *);
extern void func_00217748(int);
extern void func_002176C8(int, int, int);
extern void func_00209CE8(int);
extern int D_0015EEE8 MACRO_ADDR;
extern int D_0015EEEC MACRO_ADDR;
extern int D_0015EEF0 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;

extern int func_002176C8_i(int, int, int) __asm__("func_002176C8");

/* func_002176C8 returns int and takes three arguments (stream.c). */
void func_00209DC0(void) {
    int a;
    int b;
    int s0;
    int s1;
    int s2;

    func_001FDF10(D_00137C80[5] << 11, &a, &b);
    func_00217748(1);
    func_002176C8_i(a, D_00137C80[4], D_00137C80[5]);
    s2 = D_0015EEF0;
    s1 = D_0015EEEC;
    s0 = gStereo;
    memcard_RestoreGame(a + *(int *)(a + 0x10));
    D_0015EEF0 = s2;
    D_0015EEEC = s1;
    gStereo = s0;
    D_0015EE84_m = 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00209E68);

extern int D_001A05C0[];
extern int D_001A08C0[];
extern int func_0020BAD8(int *p);
extern int func_0020BBC8(void *dst, int i, int *table);

/* Serialise both descriptor tables into the caller's buffer: the two
   sizes first, then one blob for D_001A05C0 and twenty for D_001A08C0,
   each call returning how far to advance. The write cursor is the
   parameter itself -- retail keeps both in $s2. */
/* memcard_MakeWholeSave */
void func_0020BA00(char *out) {
    int i;

    *(int *)out = memcard_GetDataSize(D_001A05C0);
    *(int *)(out + 4) = memcard_GetDataSize(D_001A08C0);
    out += 8;
    out += memcard_PrepData(out, 0, D_001A05C0);
    for (i = 0; i < 0x14; i++) {
        out += memcard_PrepData(out, i, D_001A08C0);
    }
}

extern int func_001236F0(void);
/* Unprototyped deliberately: this is the varargs function, so callers
   legitimately pass different argument counts/types, and two agents
   declared it incompatibly. Both callers are byte-exact and must not
   be edited; an unprototyped declaration satisfies all call sites
   with identical codegen. (Only varargs *definitions* need stdarg.h;
   calling one is fine.) */
extern int func_001E9730();
extern char D_001E8690[];

/* memcard_Init */
void func_0020BAA8(void) {
    if (func_001236F0()) {
        STUB_printf(D_001E8690);
    }
}

/* memcard_GetDataSize */
int func_0020BAD8(int *p) {
    int n = 8;
    while (p[0] != 0) {
        n += 8;
        n += p[1];
        p += 4;
        n = (n + 3) & ~3;
    }
    return n + 8;
}

/* memcard_Checksum: 0 for more than 0x1800 bytes, else a 16-bit
   shift-register checksum (polynomial 0x1F45). Only the low half of
   the 0xEDB88320 seed (the CRC-32 polynomial) reaches the result. */
int func_0020BB10(void *data, int len) {
    unsigned char *p = data;
    unsigned char *end;
    int crc;
    int j;

    if (len > 0x1800) {
        return 0;
    }
    end = p + len;
    crc = 0xEDB88320;
    while (p < end) {
        crc ^= *p++ << 8;
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1F45;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc & 0xFFFF;
}

/* memcard_TestChecksum. `len` is read before the test: retail's load sits
   in the beqz slot, and a load that may trap is never taken from the
   fall-through. */
int func_0020BB88(char *buf) {
    int *p = (int *)buf;
    int len = p[0];
    int sum = p[1];
    int result = 0;
    if (sum != 0) result = memcard_Checksum(p + 2, len) == sum;
    return result;
}

extern void func_001F9A00(void *dst, void *src, int len);
extern int func_0020BB10(void *data, int len);

/* Walks a descriptor table (four words per entry: base, element size,
   tag, unused) and writes one { tag, size, data } record per entry into
   the caller's buffer, four-byte aligned, terminated by { -1, 0 }. The
   header it leaves at the front is { payload length, checksum }, and the
   return value is the total including that header. */
/*
 * Byte mismatch, correct size (0x E8), 39 of 232 bytes: the allocator puts
 * the table cursor in $s1 and the write cursor in $s0, where retail has
 * them the other way round, and it initialises the cursor before the
 * `table[0] != 0` test instead of after it. Everything else is retail's
 * instruction sequence.
 *
 * Spellings tried, all identical output: a separate `int *t = table` local
 * versus advancing the parameter itself; the source address computed at
 * the call versus in a local. That local IS load-bearing for the rest --
 * computing it before the two header stores is what makes the compiler
 * reload table[1] for them, as retail does (1807 -> 517 differing words).
 * The remaining residual is allocator destination choice, the recorded
 * dead end.
 */
struct SaveBlock {
    unsigned char *base;
    int size;
    int id;
    int pad;
};

struct SaveHeader {
    int size;
    int checksum;
};

/* memcard_PrepData: pack the save blocks (id, size, data, 4-aligned) after
   an 8-byte header, end with an id -1 record, checksum it. Adapted from
   Lombyte (MIT) for PAL. */
int func_0020BBC8(void *dst, int slot, int *table) {
    struct SaveHeader *out = dst;
    struct SaveBlock *blk = table;
    unsigned char *p;
    int total;
    unsigned char *src;

    total = 0;
    p = (unsigned char *)(out + 1);
    if (blk->base != 0) {
        struct SaveBlock *block = blk;
        int mask = -4;

        do {
            src = block->base + slot * block->size;
            total += 8;
            ((int *)p)[0] = block->id;
            ((int *)p)[1] = block->size;
            p += 8;
            func_001F9A00(p, src, block->size);
            p += block->size;
            total += block->size;
            p = (unsigned char *)(((int)p + 3) & mask);
            total = (total + 3) & mask;
            block++;
        } while (block->base != 0);
    }
    total += 8;
    ((int *)p)[1] = 0;
    ((int *)p)[0] = -1;
    out->checksum = memcard_Checksum(out + 1, total);
    out->size = total;
    return total + 8;
}

typedef struct {
    int a;        /* 0x00 */
    int b;        /* 0x04 */
    int c;        /* 0x08 */
    int d;        /* 0x0C */
    char name[8]; /* 0x10 */
    int valid;    /* 0x18 */
} McEntry;        /* 0x1C */
typedef struct {
    char hdr[0x20];
    McEntry e[5];
    char pad[0x14];
} McSlot;         /* 0xC0 */
extern McSlot D_0013D390_s[] __asm__("D_0013D390");
extern int func_0020BB88(char *); /* memcard_TestChecksum */

/* memcard_RestoreInfo(char *, int, int). Advancing the buf parameter
   itself and copying the name with memcpy both matter for retail's
   registers; re-indexing the entry per store keeps its daddu copies. */
void func_0020BCB0(char *buf, int slot, int idx) {
    D_0013D390_s[slot].e[idx].valid = memcard_TestChecksum(buf) == 0;
    buf += 0x10;
    D_0013D390_s[slot].e[idx].a = *(int *)buf;
    buf += 0xC;
    D_0013D390_s[slot].e[idx].b = *(int *)buf;
    buf += 0xC;
    D_0013D390_s[slot].e[idx].c = *(int *)buf;
    buf += 0xC;
    D_0013D390_s[slot].e[idx].d = *(int *)buf;
    buf += 0xC;
    memcpy(D_0013D390_s[slot].e[idx].name, buf, 8);
}

struct RestoreEntry {
    unsigned char *data;
    int size;
    int id;
    int status;
};
struct RestoreBlock {
    int id;
    int size;
    unsigned char data[1];
};
typedef struct {
    char pad0[0xB4];
    int errors;           /* 0xB4 */
    char padB8[8];
} RestoreCardSlot;
typedef struct {
    RestoreCardSlot slot[1];
    char padC0[0xC];
    int cur;              /* 0xCC */
} RestoreCardState;
extern RestoreCardState D_0013D390_c __asm__("D_0013D390");
extern short D_0015FF50;               /* SDA, gp -0x6DB0: mismatch count */
extern int func_001151B4(const void *, const void *, int); /* memcmp */

/* memcard_RestoreData: unpack the save blocks after buf's 8-byte header
   into the table entries with the same id (slot selects the element),
   marking each entry 1 (same size), -1 (block shorter) or -2 (longer),
   and count blocks with no entry, a wrong total and entries left
   unrestored as errors, stored in the current card slot. Adapted from
   Lombyte (MIT) for PAL. */
int func_0020BD70(void *src, int slot, int *table) {
    unsigned char *buf = src;
    struct RestoreEntry *tbl = (struct RestoreEntry *)table;
    struct RestoreEntry *e;
    int errors;
    int total;
    int i;
    int n;
    unsigned char *dst;

    if (memcard_TestChecksum((char *)buf) == 0) {
        return 1;
    }
    buf += 8;
    errors = 0;
    total = 8;
    for (i = 0; tbl[i].data != 0; i++) {
        tbl[i].status = 0;
    }
    while (((struct RestoreBlock *)buf)->id != -1) {
        i = 0;
        if (tbl[0].data == 0) {
            goto missing;
        }
        while (tbl[i].id != ((struct RestoreBlock *)buf)->id) {
            i++;
            if (tbl[i].data == 0) {
                goto missing;
            }
        }
        e = (struct RestoreEntry *)(((unsigned int)i << 4) + (unsigned int)tbl);
        if (e->data != 0) {
            int bsize = ((struct RestoreBlock *)buf)->size;

            dst = e->data + slot * e->size;
            if (bsize == e->size) {
                n = e->size;
                e->status = 1;
            } else if (bsize < e->size) {
                n = bsize;
                e->status = -1;
            } else {
                n = e->size;
                e->status = -2;
            }
            if (func_001151B4(dst, ((struct RestoreBlock *)buf)->data, n) != 0) {
                (*(int *)&D_0015FF50)++;
            }
            func_001F9A00(dst, ((struct RestoreBlock *)buf)->data, n);
            total += ((n + 3) & ~3) + 8;
        } else {
        missing:
            errors++;
        }
        buf += ((((struct RestoreBlock *)buf)->size + 3) & ~3) + 8;
    }
    total += 8;
    if (total != memcard_GetDataSize((int *)tbl)) {
        errors++;
    }
    buf += 8;
    for (i = 0; tbl[i].data != 0 && tbl[i].id != ((struct RestoreBlock *)buf)->id; i++) {
        if (tbl[i].status <= 0) {
            errors++;
        }
    }
    D_0013D390_c.slot[D_0013D390_c.cur].errors = errors;
    return errors;
}

extern void func_00121A80(void *);
extern void func_0012D818(void *);
extern void func_00208FA0(void);
extern void func_00208338(void *);
extern char D_0015EF98[] MACRO_ADDR;
extern char D_00141FC0[];
extern unsigned char D_0013DE60[];
extern int D_0015EE98 MACRO_ADDR;
extern int D_0015EF24 MACRO_ADDR;
extern int D_0015EF20 MACRO_ADDR;
extern int D_0015EFB4_m __asm__("D_0015EFB4") MACRO_ADDR;
extern char D_0014EFD0[];
extern char D_001507D0[];

/* memcard_Save(slot, flags): starts the save (func_00121A80/func_0012D818
   on D_0015EF98, func_00208FA0, func_00208338 on the D_00141FC0 page for
   D_0015EE84). With no valid card slot (D_0013D390+0xC8, its record's
   +0x14) it returns whether `slot` is 0. Otherwise it ORs `slot` into the
   pending mask (+0xFC); unless nothing is pending, a save is running
   (+0xDC >= 3) or a result is waiting (+0xE4 >= 0), it stamps +0xD0 with
   D_0015EE84 (temporarily switching D_0015EE84 to `flags` and setting that
   D_0013DE60 byte), fills entry e[+0x14] (four globals and the 8-byte
   name), serialises both tables (func_0020BBC8), restores the byte and
   D_0015EE84, and sets result 0xF. Returns whether the result is 0xF.
   D_0015EF98 is MACRO_ADDR (retail rebuilds its address at each use), the
   name field is reached from a `q + 0x30` base, and each block reads
   D_0013D390 through its own `char *` local (retail keeps only the %hi). */
int func_0020BFC8(int slot, int flags) {
    unsigned char saved;

    func_00121A80(D_0015EF98);
    func_0012D818(D_0015EF98);
    func_00208FA0();
    func_00208338(D_00141FC0 + (D_0015EE84_m << 11));
    {
        char *p = D_0013D390;
        int active = *(int *)(p + 0xC8);
        int mask;

        if (active == -1 || *(int *)(p + active * 0xC0 + 0x14) < 0) {
            return slot == 0;
        }
        mask = *(int *)(p + 0xFC) | slot;
        *(int *)(p + 0xFC) = mask;
        if (mask == 0) {
            goto done;
        }
        if (slot == 0) {
            D_0015EFB4_m |= 0x200;
        }
        if (*(int *)(p + 0xDC) >= 3) {
            goto done;
        }
        if (*(int *)(p + 0xE4) >= 0) {
            goto done;
        }
        *(int *)(p + 0xD0) = D_0015EE84_m;
        saved = 0;
        if (flags >= 0) {
            D_0015EE84_m = flags;
            saved = D_0013DE60[flags];
            if (saved == 0) {
                D_0013DE60[flags] = 1;
            }
        }
    }
    {
        char *q = D_0013D390;
        char *names = q + 0x30;

        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x24) = gBolts;
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x20) = D_0015EE84_m;
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x2C) = D_0015EF24;
        memcpy(names + *(int *)(q + 0x14) * 0x1C, D_0015EF98, 8);
        *(int *)(q + *(int *)(q + 0x14) * 0x1C + 0x28) = D_0015EF20;
        memcard_PrepData(D_0014EFD0, 0, D_001A05C0);
        memcard_PrepData(D_001507D0, *(int *)(q + 0xD0), D_001A08C0);
        if (flags >= 0) {
            D_0013DE60[D_0015EE84_m] = saved;
            D_0015EE84_m = *(int *)(q + 0xD0);
        }
        if (*(int *)(q + 0xE4) < 0) {
            *(int *)(q + 0xE4) = 0xF;
            *(int *)(q + 0xE8) = *(int *)(q + 0xC8);
        }
    }
done:
    {
        char *r = D_0013D390;
        return *(int *)(r + 0xE4) == 0xF;
    }
}
__asm__(".section .text\n\tnop\n");
