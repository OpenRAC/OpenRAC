#include "common.h"
#include "structs.h"

/*
 * pause.cpp in the original source; text 0x219C08-0x228A58.
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
/* MACRO_ADDR: lui/lw where retail has them, $gp-relative in a delay slot
   (func_0021F7D0, func_002229B0). */
extern int D_0015EE84 MACRO_ADDR;
extern int D_001A0218[] NOT_SDA;
extern void func_00208458(void *, unsigned char *, int);
extern void func_00208688(void *, unsigned char *);
extern char D_0013D390[];
extern short D_0015EFB0;
extern int D_0015EFB4;
extern int D_001A05C0[];
extern int D_001A08C0[];
extern int func_0020BAD8(int *p);
extern int func_0020BBC8(void *dst, int i, int *table);
extern int func_001236F0(void);
extern int func_001E9730();
extern char D_001E8690[];
extern int D_0013D844 NOT_SDA;
extern unsigned char D_0013D4A8 NOT_SDA;
extern int D_0013D9B4 NOT_SDA;
extern unsigned char D_0013D490[];
extern unsigned char D_0013D5CA NOT_SDA;
extern int D_0013D6B8 NOT_SDA;
extern int D_0013DAE4 NOT_SDA;
extern unsigned char D_0013D4E5 NOT_SDA;
extern int D_0013DB24 NOT_SDA;
extern unsigned char D_0013D4F1 NOT_SDA;
extern int D_0013DC34 NOT_SDA;
extern unsigned char D_0013D605 NOT_SDA;
extern int D_0013D5C8 NOT_SDA;
extern unsigned char D_0013D4B0 NOT_SDA;
extern unsigned char D_0013DE55 NOT_SDA;
extern unsigned char D_0013D5DD NOT_SDA;
extern unsigned char D_0013D5E7 NOT_SDA;
extern int D_001B2F40[];
extern void func_001FA460_2(void *, void *) __asm__("func_001FA460");
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_002116A0(void *, int, int *, void *);
extern void func_001FA540(void *, void *, void *);
extern void func_00211548(void *, int, void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern int D_001414D0 NOT_SDA;
extern float D_001CAE00[] NOT_SDA;
extern void func_0020E360(void *, void *);
extern float func_001FA058(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_00118D80(int);
extern void func_00212578(int, int);
extern char D_00165600[];
extern int D_0015F718;
extern short D_0015F71C;
extern char D_001B3200[];
extern int func_001160D8(void);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9DC0(void *, void *, float);
extern void func_001FA460(void *);
extern void func_002150B0(void *, void *);
extern void func_001FA480(void *, void *);
extern float func_0020D830(void);
extern float func_00215A98(int, float);
extern unsigned char D_0014BFC0[];
extern unsigned char D_0013E620[];
extern unsigned char D_0013D510[];
extern void func_0012F068(void *);
extern void func_002177F0(int);
extern short D_001517D0[];
extern void func_0012EDE0(void *);
extern void func_0012EFE8(void);
extern char D_001E8980[];
extern int func_0012EE98(int, int, int, void *);
extern void func_001F9978(void);
extern int func_00217628_v(void) __asm__("func_00217628");
extern void func_00122598(int);
extern void func_00217130(void);
extern void func_0012EC40(void);
extern void func_0012DDC0(void);
extern void func_0012EC30(void);
extern int func_0012F030(void);
extern void func_002167C0(short, short, short);
extern void func_002169B8(short, short, short);
extern short D_001517F0 NOT_SDA;
extern char D_0013CA40[];
extern int D_001CDAE0 NOT_SDA;
extern void func_00124650(void);
extern void func_00124B88(int);
extern int func_00124BC8(void *, void *);
extern void func_00217F68(void *);
extern void func_001F68E8(void *a, void *b, void *c, void *d, void *e);
extern void *func_001FE540(void);
extern int func_00205790(void);
extern void func_0020BA00(char *out);
extern void func_00217588(void);

struct S {
    u8 pad_0[0x140];
    float f140, f144, f148;
    u8 pad_14C[0x204];
    float f350;
    u8 pad_354[0x10];
    float f364;
    u8 pad_368[0x10];
    float f378, f37C;
};
extern struct S D_00187040_19C08 __asm__("D_00187040");
extern s32 D_001873A0;
extern s32 D_001873B0;

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/rendering/fun_00218d10.c, FUN_00218d10. */
void func_00219C08(void) {
    D_00187040_19C08.f140 = 256.0f;
    D_00187040_19C08.f148 = 64.0f;
    D_00187040_19C08.f144 = 256.0f;
    qzero(&D_00187040_19C08.f350);
    qzero(&D_001873A0);
    qzero(&D_001873B0);
    D_00187040_19C08.f350 = 1.0f;
    D_00187040_19C08.f364 = 1.0f;
    D_00187040_19C08.f378 = 1.0f;
    D_00187040_19C08.f37C = 1.0f;
}

extern void func_0012E528(int);
extern void func_00216EF0(int);
extern int D_0018C42C;
extern int D_0015F754 MACRO_ADDR;
extern int D_00141760;
extern unsigned char D_0014171B NOT_SDA;
extern int D_0015EFA0 MACRO_ADDR;
extern int D_0015EF20 MACRO_ADDR;
extern char D_001CE938[];
extern char D_001CEAC8[];
extern char D_001CEB18[];
extern int D_001A0414;
extern int D_001D0718;
extern int func_0020C7A0_i(void) __asm__("func_0020C7A0");
extern int func_0012DDC0_i(void) __asm__("func_0012DDC0");
extern void func_00228160(void);
extern char D_001D5F70[] NOT_SDA;
extern int D_0015F6E8 MACRO_ADDR;

/* PauseAllSounds: pause entry. Unless D_0018C42C is set (then only
   D_0015F754 = 1), it computes the pause object's flags (0x134, 0x138,
   0xD8 "loaded", 0xDC "arg0 is 0x23"), points the D_001CE938/D_001CEAC8
   records' +0x38/+0x3C at the loaded or unloaded variants, resets the
   object for arg0 (func_00219E60's body) and D_001A0414, then calls
   func_0020C7A0 and func_00228160. Each group reads D_001D5F70 through
   its own `char *` local (retail's saved %hi); func_0012DDC0 and
   func_0020C7A0 return values, which moves the next temporary to $v1. */
void func_00219C70(int arg0) {
    func_0012E528(0x1D);
    music_Pause(0);
    func_0012DDC0_i();
    if (D_0018C42C != 0) {
        D_0015F754 = 1;
        return;
    }
    if (D_00141760 == 0x24) {
        D_00141760 = 0;
    }
    {
        char *g = D_001D5F70;
        *(int *)(g + 0x134) = D_0015EE84 == 0xD || D_0014171B != 0;
    }
    {
        char *g = D_001D5F70;
        *(int *)(g + 0x138) = D_0015EE84 == 0 || D_0015EE84 == 0xE;
    }
    {
        char *g = D_001D5F70;
        *(int *)(g + 0xD8) = D_0015EFA0 != 0 || D_0015EF20 != 0
                             || *(int *)(g + 0xF8) != 0;
    }
    {
        char *g = D_001D5F70;
        char *a = D_001CE938;
        *(int *)(g + 0xDC) = arg0 == 0x23;
        *(char **)(a + 0x38) = *(int *)(g + 0xD8) ? D_001CEB18 : D_001CEAC8;
    }
    {
        char *g = D_001D5F70;
        char *b = D_001CEAC8;
        *(char **)(b + 0x3C) = *(int *)(g + 0xD8) ? D_001CEB18 : D_001CE938;
    }
    {
        char *g = D_001D5F70;
        D_0015F6E8 = 3;
        *(int *)g = arg0;
        *(int *)(g + 0xC) = 0;
        *(int *)(g + 0x10) = 0;
        *(int *)(g + 0x110) = 0;
    }
    if (D_0015EE84 < 0x13) {
        D_001A0414 = D_0015EE84;
    } else {
        D_001A0414 = 0;
    }
    func_0020C7A0_i();
    D_001D0718 = 0;
    func_00228160();
    {
        char *g = D_001D5F70;
        *(int *)(g + 0x13C) = 1;
        *(int *)(g + 0x140) = 0;
    }
}

LINKER_REMNANT("asm/remnants/text", func_00219E48);

void func_00219E60(void) {
    char *p = D_001D5F70;
    D_0015F6E8 = 3;
    *(int *)p = 0x2D;
    *(int *)(p + 0xC) = 0;
    *(int *)(p + 0x10) = 0;
    *(int *)(p + 0x110) = 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00219E90);

extern int D_0015EF78 MACRO_ADDR;
extern int func_002267C0(int);
extern int D_001D6120[];

typedef struct { char pad[0x44]; char *items[14]; } ObjList;

/* Pause teardown: calls each object's +0xC handler, then refreshes the
   14 D_001D6120 handles. Retail's register copies come from gcse, so the
   base is re-read inside each block (and inside the loop body). */
void func_0021A0B0(void) {
    char *g = D_001D5F70;

    if (*(int *)(g + 0x110) < 10) {
        return;
    }
    if (*(int *)(g + 4) != 0) {
        int i;
        for (i = 0; i < 14; i++) {
            char *g2 = D_001D5F70;
            char *obj = ((ObjList *)*(char **)(g2 + 4))->items[i];
            if (obj != 0) {
                void (*fn)(void *, int) = *(void (**)(void *, int))(obj + 0xC);
                if (fn != 0) {
                    fn(obj, 0);
                }
            }
        }
        {
            char *g5 = D_001D5F70;
            *(int *)(g5 + 4) = 0;
        }
    }
    {
        char *g3 = D_001D5F70;
        int j;
        D_0015EF78 = *(int *)(g3 + 0x18);
        for (j = 0; j < 14; j++) {
            D_001D6120[j] = func_002267C0(D_001D6120[j]);
        }
    }
    {
        char *g4 = D_001D5F70;
        *(int *)g4 = 0x14;
        *(int *)(g4 + 0x14) = 2;
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021A1A0);

INCLUDE_ASM("asm/nonmatchings/text", func_0021A610);

/*
 * Pad handler for the pause page arg0 (g->x4->x40): 1 on the 0xD00
 * buttons while g->x124 is 0; on 0x10, switch to the page's +0x38
 * target, or return -1 when it has none and g->x124 is 0. Written in
 * its family's shape (func_0021F7D0, func_00222DB0): early returns and
 * one `char *` local per block that reads a global.
 */
int func_0021ACD8(int arg0) {
    char *g = D_001D5F70;

    if (*(int *)(*(char **)(g + 4) + 0x40) != arg0) {
        return 0;
    }
    {
        char *pad = D_0013CA40;
        if (*(int *)(pad + 0x1C4) & 0xD00) {
            if (*(int *)(g + 0x124) == 0) {
                return 1;
            }
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x10) {
            char *g2 = D_001D5F70;
            int t = *(int *)(*(char **)(g2 + 4) + 0x38);
            if (t != 0) {
                *(int *)(g2 + 8) = t;
                return 0;
            }
            if (*(int *)(g2 + 0x124) == 0) {
                return -1;
            }
        }
    }
    return 0;
}

extern int D_001D6094;
extern void func_0022ED80(int, int, int);

/* Pause list handler (func_0021ACD8's family: one `char *` pad local per
   block). The cursor at arg0+0x3C moves on 0x1000/0x4000 within
   arg0+0x40, with a func_0022ED80 notify on change; then the scroll
   (arg0+0x60) is clamped around the cursor and the bar position at
   arg0+0x5C derived from it, or pinned to 0x52. The re-reads of 0x3C and
   0x60 are how retail's reloads come out; the page size q - 2 is formed
   after the clamp, and each arm stores the bar position itself. */
int func_0021AD68(char *arg0) {
    {
        char *pad = D_0013CA40;
        if (*(int *)(pad + 0x1C4) & 0xD00) {
            if (D_001D6094 == 0) {
                return 1;
            }
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x10) {
            char *g = D_001D5F70;
            int t = *(int *)(*(char **)(g + 4) + 0x38);
            if (t != 0) {
                *(int *)(g + 8) = t;
                return 0;
            }
            if (*(int *)(g + 0x124) == 0) {
                return -1;
            }
        }
    }
    {
        char *pad3 = D_0013CA40;
        int v = *(int *)(pad3 + 0x1C4);
        int old = *(int *)(arg0 + 0x3C);
        if ((v & 0x1000) && old != 0) {
            *(int *)(arg0 + 0x3C) = old - 1;
        }
        if (v & 0x4000) {
            int c = *(int *)(arg0 + 0x3C) + 1;
            if (c < *(int *)(arg0 + 0x40)) {
                *(int *)(arg0 + 0x3C) = c;
            }
        }
        if (*(int *)(arg0 + 0x3C) != old) {
            func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
        }
    }
    {
        int t = *(int *)(arg0 + 0x24) * 16;
        if (t / 0x252 >= *(int *)(arg0 + 0x40)) {
            *(int *)(arg0 + 0x5C) = 0x52;
        } else {
            int q = (t - 0x28) / 0x252;
            int n;
            if (*(int *)(arg0 + 0x60) >= *(int *)(arg0 + 0x3C)) {
                *(int *)(arg0 + 0x60) = *(int *)(arg0 + 0x3C) - 1;
                if (*(int *)(arg0 + 0x60) < 0) {
                    *(int *)(arg0 + 0x60) = 0;
                }
            }
            n = q - 2;
            if (*(int *)(arg0 + 0x60) < *(int *)(arg0 + 0x3C) - n) {
                *(int *)(arg0 + 0x60) = *(int *)(arg0 + 0x3C) - n;
            }
            *(int *)(arg0 + 0x5C) = 0x182 - *(int *)(arg0 + 0x60) * 0x252;
        }
    }
    return 0;
}

extern short D_001602B0;              /* SDA, gp -0x6A50 */
extern void func_00201640(int, int, int, int, long, long);
extern int func_00200198(int, int);
extern void func_00200468(int, int, int, int, int, int);
extern void func_00200650(int, int, int, int, int, int);
extern void func_002008B8(int, int, int, int, int, int);
extern int D_0015F538 MACRO_ADDR;
extern int func_001F9B70(int); /* abs */
extern void func_001F4630(int);
extern void func_001F4748(void);

/* Vertical picture list: each entry (10 bytes at +0x48, count at +0x40)
   is a 0x200-square image at x, stepping 0x252 down from the scroll
   position at +0x5C; the selected one (+0x3C) gets a pulsing frame. Up
   and down arrows show when the list runs off the top or the bottom. */
int func_0021AEF8(char *arg0) {
    int x = (*(int *)(arg0 + 0x20) * 16 - 0x200) >> 1;
    int y = *(int *)(arg0 + 0x5C);
    int top = y;
    int i;

    SetupGifPaging(0);
    for (i = 0; i < *(int *)(arg0 + 0x40); i++) {
        char *e = *(char **)(arg0 + 0x48) + i * 10;

        if (*(int *)(arg0 + 0x3C) == i) {
            func_00201640(x - 0x30, y - 0x30, x + 0x230, y + 0x230,
                          (unsigned int)((func_001F9B70((D_0015F538 & 0x3F) - 0x20) + 0x40)
                                         * 0x10202 | 0x80000000), 1);
            func_00201640(x - 0x10, y - 0x10, x + 0x210, y + 0x210,
                          *(int *)&D_001602B0, 1);
        }
        func_002008B8(GetIconFrame(*(unsigned short *)e, *(short *)(e + 2)), x, y,
                      0x200, 0x200, 0x80);
        y += 0x252;
    }
    if (top < 0) {
        func_00201640(0, 0, *(int *)(arg0 + 0x20), 0x14, *(int *)&D_001602B0, 0);
        HudSprite(GetIconFrame(0xE99E, 6), x >> 4, 2, 0x20, 0x10, 0x80);
    }
    if (*(int *)(arg0 + 0x24) * 16 < y) {
        func_00201640(0, *(int *)(arg0 + 0x24) - 0x14, *(int *)(arg0 + 0x20),
                      *(int *)(arg0 + 0x24), *(int *)&D_001602B0, 0);
        func_00200650(GetIconFrame(0xE99E, 6), x >> 4, *(int *)(arg0 + 0x24) - 0x12,
                      0x20, 0x10, 0x80);
    }
    DoGifPaging();
    return 2;
}

extern int D_0015EF90;
extern char D_001D4B90[];
extern char D_001D4BC0[];

extern int D_0015EF90_m __asm__("D_0015EF90") MACRO_ADDR;

/* D_0015EF90 is read through a MACRO_ADDR alias: retail's one-register
   load. */
int func_0021B108(void *arg0) {
    *(char **)((char *)arg0 + 0x34) =
        (D_0015EF90_m != 0) ? D_001D4B90 : D_001D4BC0;
    return 0;
}

/* The two initializer tables of the local arrays below. The first is
   copied as a char block (retail's ldl/ldr for all 32 bytes), the second
   as an int block (ldl/ldr, then lw/sw for the last word). */
typedef struct { char b[0x20]; } Blk32;
typedef struct { int w[7]; } Blk28;
extern Blk32 D_001E8A58;
extern Blk28 D_001E8A78;
/* MACRO_ADDR: retail builds each address in one register (la). */
extern unsigned char D_0015EEC0[] MACRO_ADDR;
extern unsigned char D_0015EEB0[] MACRO_ADDR;
typedef struct {
    int val;
    void *addr;
    int c1;
    int c2;
    int zero;
} Entry14;
extern Entry14 D_001D3E90[];

/* Builds the D_001D3E90 list: for each id in {1, 3, 0, 7, 4, 6, 2} (up
   to the -1) whose D_0015EEC0 flag is set, an entry with its text id
   (0x501A...), &D_0015EEB0[id] and the constants 0x4F5A/0x4F5B; a zero
   val ends the list. A plain indexed for loop: strength reduction gives
   retail's pointer walk, its end test against ids + 12, and the id read
   twice per iteration (exit test and body). */
int func_0021B138(void) {
    int ids[8];
    int names[7];
    int i, n;

    *(Blk32 *)ids = D_001E8A58;
    *(Blk28 *)names = D_001E8A78;
    n = 0;
    for (i = 0; i < 12 && ids[i] != -1; i++) {
        if (D_0015EEC0[ids[i]] != 0) {
            D_001D3E90[n].val = names[i];
            D_001D3E90[n].addr = &gCheats[ids[i]];
            D_001D3E90[n].c1 = 0x4F5A;
            D_001D3E90[n].c2 = 0x4F5B;
            D_001D3E90[n].zero = 0;
            n++;
        }
    }
    D_001D3E90[n].val = 0;
    return 0;
}

int func_0021B278(void) {
    return 0;
}

int func_0021B280(void) {
    return 0;
}

int func_0021B288(void *arg0) {
    *(int *)((char *)arg0 + 0x44) = -1;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021B298);

INCLUDE_ASM("asm/nonmatchings/text", func_0021BB90);

typedef struct {
    s16 text_id;
    s16 enabled;
    s32 action_value;
    s16 secondary_text_id;
    s16 fade_timer;
} MenuItem_1C1B0;
typedef struct {
    u8 pad0[0x20];
    s32 width;
    s32 height;
    u8 pad28[8];
    s32 flags;
    MenuItem_1C1B0 *items;
    u8 pad38[8];
    s32 selected_entry;
} MenuDescriptor;
typedef struct {
    u8 pad0[0x40];
    MenuDescriptor *focus;
} MenuPage;
extern MenuPage * D_001D5F74_1C1B0[] __asm__("D_001D5F74");
extern u8 D_001DF3D0[];
extern u8 D_001DF770[];
extern u8 D_001DFB10[];
extern short D_001602B8;
extern short D_001602BC;
extern void func_00234C98_1C1B0(s32, long) __asm__("func_00234C98");
extern void func_001F4630(s32);
extern s32 func_001F4868(s32);
extern void func_001F6598(void);
extern void func_001F65A8(void);
extern s32 func_001F65B0_1C1B0(char *, s32, u8 *) __asm__("func_001F65B0");
extern void func_001F6668_1C1B0(s32, s32, long, char *, s32, s32, u8 *) __asm__("func_001F6668");
extern char *func_001FE540_1C1B0(s32) __asm__("func_001FE540");
extern s32 func_0021C6C0(s32, s32, s32);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/textbin/render_localized_ui_entry_list.c, render_localized_ui_entry_list. */
s32 func_0021C1B0(MenuDescriptor *menu) {
    s32 focused;
    u8 *glyphs;
    s32 font_texture_index;
    s32 row_height;
    s32 half_width;
    s32 maximum_text_width;
    s32 selection_index;
    s32 font_height;
    s32 entry_count;
    MenuItem_1C1B0 *entry;
    MenuItem_1C1B0 *scan_entry;
    s32 entry_index;
    s32 selected_entry;
    s32 enabled;
    s32 color;
    char *text_id;
    char *secondary_text;
    s32 x;
    s32 y;
    s32 text_width;
    s32 flags;
    s32 menu_width;
    MenuItem_1C1B0 *selected_item;
    s32 shadow_y;
    s32 shadow_x;
    font_height = 12;
    font_texture_index = 1;
    glyphs = D_001DF3D0;
    focused = D_001D5F74_1C1B0[0]->focus == menu;
    if (menu->flags & 4) {
        font_height = 14;
        font_texture_index = 3;
        glyphs = D_001DFB10;
    }
    if (menu->flags & 8) {
        font_height = 10;
        font_texture_index = 2;
        glyphs = D_001DF770;
    }
    func_00234C98_1C1B0(0x42, 0x44);
    func_00234C98_1C1B0(0x47, 0x2004B);
    func_001F4630(0);
    entry_count = 0;
    entry = menu->items;
    while (entry->text_id != 0) {
        entry++;
        entry_count++;
    }

    if (menu->flags & 0x10) {
        row_height = font_height + 3;
    } else {
        row_height = menu->height / (entry_count + 1);
    }
    half_width = menu->width >> 1;
    maximum_text_width = 0;
    y = (row_height - (font_height / 2)) - 1;
    if (menu->flags & 0x4000) {
        scan_entry = menu->items;
        if (scan_entry[0].text_id != 0) {
            entry_index = 0;
            selection_index = 0;
            do {
                text_width = func_001F65B0_1C1B0(
                    func_001FE540_1C1B0(scan_entry[entry_index].text_id), -1, glyphs);
                entry_index++;
                selection_index++;
                maximum_text_width =
                    (maximum_text_width < text_width) ? (text_width) : (maximum_text_width);
                scan_entry = menu->items;
            } while (menu->items[selection_index].text_id != 0);
        }
    }
    flags = menu->flags;
    menu_width = menu->width;
    if (flags & 0x20000) {
        if (menu_width < (maximum_text_width + 6)) {
            if (!(flags & 8)) {
                menu->flags = flags | 8;
                return 1;
            }
        }
    }
    entry_index = (selection_index = 0);
    maximum_text_width = (menu_width < maximum_text_width) ? (menu_width) : (maximum_text_width);
    if (menu->items[0].text_id != 0) {
        do {
            selected_entry = 0;
            if (focused) {
                selected_entry = menu->selected_entry == selection_index;
            }
            selected_item = (MenuItem_1C1B0 *)((u32)(entry_index * sizeof(MenuItem_1C1B0)) + (u32)menu->items);
            enabled = selected_item->enabled != 0;
            if (menu->flags & 2) {
                color = 0x80FFA888;
            } else if (selected_entry) {
                if (enabled) {
                    goto fade_timer;
                }
                color = 0x80006060;
            } else if (enabled) {
            fade_timer:
                color = func_0021C6C0(selected_item->fade_timer, -1, -1);

            } else {
                color = 0x80303030;
            }
            text_id = func_001FE540_1C1B0(menu->items[entry_index].text_id);
            if (menu->items[entry_index].enabled == 2) {
                text_id = func_001FE540_1C1B0(0x4F54);
            }
            x = half_width - (func_001F65B0_1C1B0(text_id, -1, glyphs) >> 1);
            if (menu->flags & 0x40) {
                x = 4;
            } else if (menu->flags & 0x4000) {
                x = half_width - (maximum_text_width >> 1);
            }
            func_001F65A8();
            func_001F6668_1C1B0(x + (*(s32 *)&D_001602B8), y + (*(s32 *)&D_001602BC), 0x80000000L, text_id, -1,
                       func_001F4868(font_texture_index), glyphs);
            func_001F6598();
            if (menu->flags & 0x80) {
                func_001F65A8();
            }
            func_001F6668_1C1B0(x, y, color, text_id, -1, func_001F4868(font_texture_index), glyphs);
            y += row_height;
            if (menu->items[entry_index].secondary_text_id != 0) {
                func_001F65A8();
                shadow_x = x + (*(s32 *)&D_001602B8);
                shadow_y = y + (*(s32 *)&D_001602BC);
                secondary_text = func_001FE540_1C1B0(menu->items[entry_index].secondary_text_id);
                func_001F6668_1C1B0(shadow_x, shadow_y, 0x80000000L, secondary_text, -1,
                           func_001F4868(font_texture_index), glyphs);
                if (!(menu->flags & 0x80)) {
                    func_001F6598();
                }
                func_001F6668_1C1B0(x, y, color,
                           func_001FE540_1C1B0(menu->items[entry_index].secondary_text_id), -1,
                           func_001F4868(font_texture_index), glyphs);
                y += row_height;
            }
            if (menu->flags & 0x80) {
                func_001F6598();
            }
            entry_index++;
            selection_index++;
        } while (menu->items[entry_index].text_id != 0);
    }
    func_001F4748();
    return 2;
}

/* SDA, gp -0x6A4C: declared 2 bytes so the assembler reaches it through
   $gp, and read as the int it is. */
extern short D_001602B4;
extern int func_001FA8A8(int, int, float);

/* Fade colour for the pause menus: -1 picks the default colours
   0x80FFA888 / 0x8020FFFF and a negative start becomes 0. While the
   D_001602B4 timer (func_001F98C0) is still below start the blend factor
   is 1; after that it is 1 - (elapsed - start) / total, and the two
   colours are blended by func_001FA8A8. The parameters are updated in
   place; the timer is read again for each value. */
int func_0021C6C0(int start, int col1, int col2) {
    float t;

    if (start < 0) {
        start = 0;
    }
    if (col1 == -1) {
        col1 = 0x80FFA888;
    }
    if (col2 == -1) {
        col2 = 0x8020FFFF;
    }
    if (func_001F98C0(*(int *)&D_001602B4) >= start) {
        int a = func_001F98C0(*(int *)&D_001602B4);
        t = 1.0f - (float)(a - start) / (float)func_001F98C0(*(int *)&D_001602B4);
    } else {
        t = 1.0f;
    }
    return FastTweenColor(col1, col2, t);
}

/* Pause page link walk: with arg1 clear, follow arg0's +0x4C chain for
   as long as the current page is skippable (bit 8 of its +0x30 flags
   while D_001D5F70+0x134 is set, or bit 4 while +0x138 is set), and if
   it moved at all, make the page it stopped on the current one's +0x80.
   Each block reads D_001D5F70 through its own `char *` local: the loop's
   own copy is why GCSE leaves its +0x138 load in the loop (only +0x134,
   read first in the body, is hoisted by loop.c), and the last block's
   copy is retail's kept %hi. */
int func_0021C790(char *arg0, int arg1) {
    int ok = 0;
    int found = 0;

    if (arg1 != 0) {
        return 0;
    }
    {
        char *g = D_001D5F70;
        if (*(int *)(g + 0x134) != 0 && (*(int *)(arg0 + 0x30) & 8)) {
            ok = 1;
        }
        if (*(int *)(g + 0x138) != 0 && (*(int *)(arg0 + 0x30) & 4)) {
            ok = 1;
        }
    }
    while (ok) {
        char *g = D_001D5F70;
        found = 1;
        arg0 = *(char **)(arg0 + 0x4C);
        ok = 0;
        if (*(int *)(g + 0x134) != 0 && (*(int *)(arg0 + 0x30) & 8)) {
            ok = 1;
        }
        if (*(int *)(g + 0x138) != 0 && (*(int *)(arg0 + 0x30) & 4)) {
            ok = 1;
        }
    }
    if (found) {
        char *g2 = D_001D5F70;
        *(char **)(*(char **)(g2 + 4) + 0x80) = arg0;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021C840);

int func_0021CD98(void) {
    return 0;
}


/* D_001D5F70 + 0xCB. Retail addresses this one byte BOTH ways -- through
   its own %hi/%lo here and as 0xCB($16) off the D_001D5F70 base a few
   instructions later -- so it really is two names on one address in the
   original source, and splat has the second as its own linker symbol. */
extern char D_001D603B[];
extern short D_001517D0[];

/*
 * Pause/menu teardown. When the D_001517D0+8 state word is clear, drop
 * the active slot: clear the 0xCB flag byte, toggle 0x1000 in that
 * slot's flag word and mark no slot active. When it is SET instead and
 * the 0xCB byte is still set, run func_00217588 first, then clear the
 * byte and invalidate the slot outright (flags = -1, not a toggle).
 *
 * The two tests read D_001517D0[4] twice rather than being one if/else,
 * and that IS the source: the indirect store into the slot table between
 * them kills gcc 2.95's memory, so retail reloads the halfword and
 * re-materialises the base. Written as if/else it collapses to a single
 * test.
 *
 * D_001A01F0 is the struct from func_00205790 above; `sel` at +0x2A0
 * sits between the flag array and the size array, which is why flags is
 * 5 entries and not 6.
 *
 * `char *g = D_001D5F70;` is load-bearing: writing the two accesses as
 * D_001D5F70[0xCB] lets gcc fold 0xCB into the symbol addend, giving
 * %hi/%lo(D_001D5F70+203) and a base register already at +0xCB. Retail
 * keeps the UNOFFSET base in $16 and puts 0xCB in both displacements.
 * Same instruction count, but it also changes what the branch delay
 * slot gets filled with, so it is a byte difference, not just cosmetic.
 */
int func_0021CDA0(void) {
    int cur;

    if (D_001517D0[4] == 0) {
        cur = D_001A01F0_slots.sel;
        if (cur != -1) {
            D_001D603B[0] = 0;
            D_001A01F0_slots.flags[cur] ^= 0x1000;
            D_001A01F0_slots.sel = -1;
        }
    }
    if (D_001517D0[4] != 0) {
        char *g = D_001D5F70;
        if (*(unsigned char *)(g + 0xCB) != 0) {
            request_audio_stream_break();
            g[0xCB] = 0;
            D_001A01F0_slots.flags[D_001A01F0_slots.sel] = -1;
            D_001A01F0_slots.sel = -1;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021CE60); /* DrawMapScreen */

extern int D_001A0414;
extern int D_001CFBF4;
extern int D_001CFAD8;
extern void func_0020C7A0(void *);
extern int func_0020CA50(void *, void *, void *, int);

int func_0021D420(void *arg0) {
    int *p;

    func_0020C7A0(arg0);
    *(int *)((char *)arg0 + 0x7C) =
        func_0020CA50((void *)0x70000000, (void *)0, (void *)0x70000100, 1);
    p = (int *)((char *)arg0 + 0x30);
    p += D_001A0414;
    if (*p != -1) {
        D_001CFBF4 = ((int *)0x70000000)[*p];
        D_001CFAD8 = ((int *)0x70000100)[*p];
    }
    return 0;
}

struct Menu;
struct Owner {
    char pad0[0x38];
    int unk38;
    char pad3C[4];
    struct Menu *owner;
};
struct Menu {
    char pad0[0x14];
    int unk14;
    char pad18[0x18];
    int choice[19];
    int count;
};
typedef struct {
    char pad0[4];
    struct Owner *cur;
    int unk8;
    char padC[0x118];
    int unk124;
} PauseG;
typedef struct {
    char pad0[0x1C4];
    unsigned int pressed;
} PadG;
typedef struct {
    char pad0[0x224];
    int level;
} LevelG;
extern PauseG D_001D5F70_g __asm__("D_001D5F70") NOT_SDA;
extern PadG D_0013CA40_g __asm__("D_0013CA40");
extern LevelG D_001A01F0_lv __asm__("D_001A01F0");
extern unsigned char D_0013DE48[];

/* Pause slot-select tick. Adapted from Lombyte (MIT) for PAL: src/ui/menus/fun_0021c4c0.c, FUN_0021c4c0. */
int func_0021D4C0(struct Menu *m) {
    int old;
    int i;
    int prev;
    unsigned int pad;
    int *p;
    int *ch;

    {
        struct Menu *owner = D_001D5F70_g.cur->owner;
        old = D_001A01F0_lv.level;
        if (owner != m) {
            if (old < 20) {
                m->choice[old] = -1;
            }
            return 0;
        }
    }
    if ((D_0013CA40_g.pressed & 0xD00) && D_001D5F70_g.unk124 == 0) {
        return 1;
    }
    if (D_0013CA40_g.pressed & 8) {
        for (i = D_001A01F0_lv.level + 1; i < 20; i++) {
            if (D_0013DE48[i] != 0 || D_0015EE84 == i) {
                D_001A01F0_lv.level = i;
                break;
            }
        }
    }
    if (D_0013CA40_g.pressed & 4) {
        for (i = D_001A01F0_lv.level - 1; i >= 0; i--) {
            if (D_0013DE48[i] != 0 || D_0015EE84 == i) {
                D_001A01F0_lv.level = i;
                break;
            }
        }
    }
    if (D_001A01F0_lv.level != old) {
        func_0022ED80(1, 0x11, m->unk14);
        func_0020C7A0_i();
    }
    if (m->count != 0) {
        pad = D_0013CA40_g.pressed;
        ch = m->choice;
        prev = m->choice[D_001A01F0_lv.level];
        if (pad & 0x1000) {
            m->choice[D_001A01F0_lv.level] = (prev + m->count - 1) % m->count;
        }
        if (pad & 0x4000) {
            m->choice[D_001A01F0_lv.level] = (m->choice[D_001A01F0_lv.level] + 1) % m->count;
        }
        if (m->choice[D_001A01F0_lv.level] != prev) {
            func_0022ED80(1, 0x11, m->unk14);
        }
        if ((pad & 0x5000) || D_001A01F0_lv.level != old) {
            m->count = func_0020CA50((void *)0x70000000, (void *)0, (void *)0x70000100, 1);
            p = &ch[D_001A01F0_lv.level];
            D_001CFBF4 = ((unsigned int *)0x70000000)[*p];
            D_001CFAD8 = ((unsigned int *)0x70000100)[*p];
        }
    }
    if (D_0013CA40_g.pressed & 0x10) {
        if (D_001D5F70_g.cur->unk38 != 0) {
            D_001D5F70_g.unk8 = D_001D5F70_g.cur->unk38;
        } else if (D_001D5F70_g.unk124 == 0) {
            return -1;
        }
    }
    return 0;
}

extern char *D_001D5F74 NOT_SDA;
extern int D_0015EFA4 MACRO_ADDR;
extern void func_0022ED80(int, int, int);
typedef struct {
    char pad[0xA8];
    unsigned short count;  /* 0xA8 */
    unsigned short best;   /* 0xAA */
    unsigned int mask;     /* 0xAC */
} Stats_0021D7A0;
extern Stats_0021D7A0 D_00141948;
typedef struct {
    char pad[0x14];
    int id;      /* 0x14 */
    char pad18[0x18];
    int list[8]; /* 0x30 */
    int idx;     /* 0x50 */
} SelList_0021D7A0;

/* Quick-select slot handler: 8 and 4 on the pad step the slot index at
   +0x50 forward and back (mod 8), with a func_0022ED80 notify when it
   moved. With 0x40, if the highlighted item (the current page's entry
   id) is owned, it is recorded: the use count at D_00141948+0xA8 (to
   0xFFFF), the best time +0xAA and the level mask +0xAC; then the item
   leaves any slot it held and is put in the current slot, and the index
   advances. The three conditions are one `&&` (one shared exit) and the
   time is compared call-first, as retail evaluates them. */
int func_0021D7A0(SelList_0021D7A0 *arg0) {
    char *q = *(char **)(D_001D5F74 + 0x40);
    int item = *(short *)(*(int *)(q + 0x3C) * 10 + *(char **)(q + 0x48) + 6);
    int old = arg0->idx;
    char *pad = D_0013CA40;
    int i;

    if (*(int *)(pad + 0x1C4) & 8) {
        arg0->idx = (old + 1) % 8;
    }
    if (*(int *)(pad + 0x1C4) & 4) {
        arg0->idx = (arg0->idx + 7) % 8;
    }
    if (arg0->idx != old) {
        func_0022ED80(1, 0x11, arg0->id);
    }
    if (item != 0 && ((unsigned char *)&D_0013D5C8)[item] != 0
        && (*(int *)(pad + 0x1C4) & 0x40)) {
        Stats_0021D7A0 *s = &D_00141948;

        if (s->count <= 0xFFFE) {
            s->count++;
        }
        if (func_001F98C0(D_0015EFA4) / 600 > s->best) {
            s->best = func_001F98C0(D_0015EFA4) / 600;
        }
        s->mask = s->mask | (1 << D_0015EE84) | 0x80000000;
        for (i = 0; i < 8; i++) {
            if (arg0->list[i] == item) {
                break;
            }
        }
        if (i < 8) {
            arg0->list[i] = 0;
        }
        arg0->list[arg0->idx] = item;
        arg0->idx = (arg0->idx + 1) % 8;
    }
    return 0;
}

extern int D_00141FA0[];

typedef struct {
    char pad[0x30];
    int list[8]; /* 0x30 */
    int idx;     /* 0x50 */
} SelList_0021D9C8;

/* Reload the 8-entry list at +0x30 from D_00141FA0 (the reverse of
   func_0021DA60 below), then leave idx at +0x50 on the first empty entry,
   wrapped to 0..7. Indexing arg0->list directly in both loops (no local
   list pointer) is what gives retail's hoisted base; the scan is a do-while
   guarded by list[0] (a plain while/for rotated differently). */
int func_0021D9C8(SelList_0021D9C8 *arg0) {
    int i;
    for (i = 0; i < 8; i++) {
        arg0->list[i] = D_00141FA0[i];
    }
    arg0->idx = 0;
    if (arg0->list[0] != 0) {
        do {
            arg0->idx++;
        } while (arg0->list[arg0->idx] != 0 && arg0->idx < 8);
    }
    arg0->idx %= 8;
    return 0;
}

extern int D_00141FA0[];

int func_0021DA60(void *arg0) {
    int *src = (int *)((char *)arg0 + 0x30);
    int *dst = D_00141FA0;
    int i = 7;
    do {
        *dst++ = *src++;
    } while (--i >= 0);
    return 0;
}

extern char D_001D0A50[];
extern char D_001D0A88[];

int func_0021DA98(void *arg0) {
    *(char **)((char *)arg0 + 0x34) =
        (gHaveHeliPack != 0) ? D_001D0A50 : D_001D0A88;
    return 0;
}

extern int D_001A0418 NOT_SDA;

int func_0021DAC8(void) {
    D_001A0418 = -1;
    return 0;
}

extern void func_00226D50(int);

int func_0021DAE0(void) {
    func_00226D50(1);
    return 0;
}

extern int D_0015EEF0 MACRO_ADDR;
extern int D_0013E6A0;

int func_0021DB00(void) {
    D_0013E6A0 = (D_0015EEF0 * 8) / 10;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021DB30);

INCLUDE_ASM("asm/nonmatchings/text", func_0021DE08); /* DrawSoundMenu */

typedef struct {
    unsigned short v;
    short pad;
} Half4;
typedef struct {
    unsigned short a;
    short b;
    char pad[8];
} Rec0C;
extern int D_0015EF30 MACRO_ADDR;
extern Rec0C D_001CFFC0[];
extern unsigned char D_00141F08[];
extern Half4 D_00199812[];

int func_0021E170(void) {
    int i;
    for (i = 0; i < D_0015EF30;) {
        int j = i + 1;
        unsigned char k;
        D_001CFFC0[i].b = 1;
        k = D_00141F08[D_0015EF30 - j];
        D_001CFFC0[i].a = D_00199812[k].v;
        i = j;
    }
    D_001CFFC0[D_0015EF30].a = 0;
    return 0;
}

int func_0021E1F8(void) {
    return 0;
}

typedef struct {
    int key;
    int flags;
} PadBind;

/* Aliased rather than renamed: func_00227018 further down still walks
   the same table as a flat int array. */
extern PadBind D_001D6448_t[] __asm__("D_001D6448");

extern int D_001D6078;
extern int D_00137C80[];
extern int func_00217628_3(int, int, int) __asm__("func_00217628");

int func_0021E200(char *arg0) {
    int i;
    char *g;
    func_00226D50(1);
    *(int *)(arg0 + 0x54) = 0;
    *(int *)(arg0 + 0x38) = 0;
    g = D_001D5F70;
    for (i = 0; i < 5; i++) {
        if (D_001D6448_t[i].key != 0
            && (unsigned int)D_001D6448_t[i].key
                   < *(unsigned int *)(g + 0x10C)) {
            D_001D6448_t[i].flags |= 2;
        }
    }
    *(int *)(arg0 + 0x50) = 0;
    if (D_001517D0[4] == 0) {
        if (func_00217628_3(D_001D6078, D_00137C80[0x1528 / 4],
                            D_00137C80[0x152C / 4]) != 0) {
            *(int *)(arg0 + 0x50) = 1;
        } else {
            *(int *)(arg0 + 0x50) = 3;
        }
    }
    *(int *)(arg0 + 0x10) |= 4;
    return 0;
}

/* D_0015F780 is SDA elsewhere; pause.cpp stores it through $at. */
extern int D_0015F780_m __asm__("D_0015F780") MACRO_ADDR;
extern int D_001997FC;

int func_0021E2D0(int arg0) {
    if (D_001517D0[4] != 0 && *(int *)(arg0 + 0x50) == 1) {
        request_audio_stream_break();
    }
    func_00226D50(1);
    {
        int v = *(int *)(arg0 + 0x54);
        if (v != 0) {
            int w = *(int *)(arg0 + 0x38);
            D_0015F780_m = v;
            D_001997FC = w;
        }
    }
    return 0;
}

extern int D_0015EE88 MACRO_ADDR;
extern char D_001997D0[];
extern void func_001F9A00(void *, void *, int);

/* The slot count at D_001997D0+0x2C, read through its own base pointer:
   retail's loop keeps a separate copy of the D_001997D0 address. */
static inline int pauseSlotCount(void) {
    char *b = D_001997D0;
    return *(int *)(b + 0x2C);
}

/* Pause sub-state machine at arg0+0x50 (func_00221688's family). State 0
   runs func_00217628 and moves to 1 or 3; state 1, once D_001517D0[4]
   clears, pulls the controller's record out of the table at
   D_001D5F70+0x108 (func_001F9A00 moves it to the table start), swaps it
   in as D_0015F780 and the D_001997D0+0x2C count (old values kept in
   arg0+0x54/+0x38), rebases the entries' first words and moves to 2.
   Cases 2 and 3 are empty; they make gcc's case tree test 1 first, as
   retail does. The reloaded table gets its own variable so that the
   first one stays block-local, and the rebase delta is computed in the
   loop so that loop.c hoists it. */
int func_0021E340(char *arg0) {
    switch (*(int *)(arg0 + 0x50)) {
    case 0:
        if (D_001517D0[4] == 0) {
            if (func_00217628_3(D_001D6078, D_00137C80[0x1528 / 4],
                                D_00137C80[0x152C / 4]) != 0) {
                *(int *)(arg0 + 0x50) = 1;
            } else {
                *(int *)(arg0 + 0x50) = 3;
            }
        }
        break;
    case 1:
        if (D_001517D0[4] == 0) {
            char *g = D_001D5F70;
            int *tbl = *(int **)(g + 0x108);
            int *p = (int *)((char *)tbl + tbl[D_0015EE88]);
            int n = *p++;
            int sz = *p++;
            char *b;
            int *t;
            int i;

            func_001F9A00(tbl, p, ((sz + 3) & ~3) - 8);
            b = D_001997D0;
            *(int *)(arg0 + 0x54) = D_0015F780_m;
            *(int *)(arg0 + 0x38) = *(int *)(b + 0x2C);
            *(int *)(b + 0x2C) = n;
            t = *(int **)(g + 0x108);
            D_0015F780_m = (int)t;
            for (i = 0; i < pauseSlotCount(); i++) {
                int d = (int)t - 8;
                *(int *)((char *)t + i * 0x10) += d;
            }
            *(int *)(arg0 + 0x10) &= ~4;
            *(int *)(arg0 + 0x50) = 2;
        }
        break;
    case 2:
    case 3:
        break;
    }
    return 0;
}

extern void func_001F7070_a(void *, long, char *, int, int, unsigned char *) __asm__("func_001F7070");
extern void func_00234C98_l(int, long) __asm__("func_00234C98");
extern void *func_001FE540_id(int) __asm__("func_001FE540");
extern void func_001153FC(void *, int, int); /* memset */
extern void func_001F65A8(void);
extern void func_001F6598(void);
extern void func_002208F8(int x, int y, int flag);

typedef struct {
    short text;
    short enabled;
    int id;
    short subtext;
    short pad0A;
} MenuItem;

typedef struct {
    char pad00[0x20];
    int width;
    int height;
    char pad28[8];
    int flags;
    MenuItem *items;
    char pad38[8];
    int sel;
    int scroll;
} Menu;

typedef struct {
    short s[12];
} MenuBox;

/* Draws a pause-menu list (rows of text with optional subtext, cursor highlight, scrolling). Adapted from Lombyte (MIT) for PAL: ui/menus/fun_0021d4a8.c, FUN_0021d4a8. */
int func_0021E4B0(Menu *menu) {
    int size;
    int kind;
    int focused;
    unsigned char *font;
    int n;
    MenuItem *p;
    int rowh;
    int glyphs;
    int i;
    int sel;
    int en;
    int color;
    char *text;
    int end;
    int y;

    size = 12;
    kind = 1;
    font = D_001DF3D0;
    focused = *(Menu **)(D_001D5F74 + 0x40) == menu;
    if (menu->flags & 4) {
        size = 14;
        kind = 3;
        font = D_001DFB10;
    }
    if (menu->flags & 8) {
        size = 10;
        kind = 2;
        font = D_001DF770;
    }
    func_00234C98_l(0x42, 0x44);
    func_00234C98_l(0x47, 0x2004B);
    func_001F4630(0);

    n = 0;
    p = menu->items;
    while (p->text != 0) {
        p++;
        n++;
    }
    if (menu->flags & 0x10) {
        rowh = size + 3;
    } else {
        rowh = menu->height / (n + 1);
    }
    y = rowh - size / 2;
    {
        MenuBox box = { { 4, menu->height - 4, 0, menu->width - 2, 0,
                          y - menu->scroll, 0, 0, size + 2 } };

        glyphs = func_001F4868(kind);
        for (i = 0; menu->items[i].text != 0; i++) {
            sel = 0;
            if (focused && menu->sel == i) {
                sel = 1;
            }
            en = menu->items[i].enabled != 0;
            if (menu->flags & 2) {
                color = 0x80FFA888;
            } else if (sel) {
                color = en ? 0x8020FFFF : 0x80006060;
            } else {
                color = en ? 0x80FFA888 : 0x80303030;
            }
            if (!(menu->flags & 0x10000) && sel && box.s[5] < 4) {
                menu->scroll -= 4;
            }
            text = func_001FE540_id(menu->items[i].text);
            box.s[4] = (menu->flags & 0xA00) ? 0x20 : 4;
            if (menu->flags & 0x400) {
                box.s[9] = 1;
                box.s[4] = menu->width >> 1;
            }
            if (sel) {
                func_001F65A8();
            }
            func_001F7070_a(&box, color, text, -1, glyphs, font);
            if (sel) {
                func_001F6598();
            }
            if (menu->flags & 0x200) {
                func_002208F8(0xF, box.s[5] + 9, D_0013D510[i] != 0);
            }
            if (menu->flags & 0x800) {
                func_002208F8(0xF, box.s[5] + 9, D_0015EE88 == menu->items[i].id);
            }
            box.s[5] += box.s[7];
            if (menu->items[i].subtext != 0) {
                text = func_001FE540_id(menu->items[i].subtext);
                box.s[4] = 0x14;
                func_001F7070_a(&box, color, text, -1, glyphs, font);
                box.s[5] += rowh;
            }
            box.s[5] += 8;
            if (!(menu->flags & 0x10000) && sel) {
                end = box.s[5] + box.s[7];
                if (box.s[1] < end) {
                    if (menu->flags & 0x8000) {
                        menu->scroll += end - box.s[1];
                    } else {
                        menu->scroll += 4;
                    }
                }
            }
        }
    }
    func_001F4748();
    if (menu->flags & 0x8000) {
        menu->flags ^= 0x8000;
        return 1;
    }
    return 2;
}

typedef struct {
    u8 pad_0[0x40];
    f32 w;
    f32 h;
} Font;
typedef struct {
    u8 pad_0[0x78];
    Font *font;
} FontHolder;
typedef struct {
    u8 pad_0[0x40];
    void *owner;
} MenuFocus;
typedef struct {
    u8 pad_0[0x4];
    MenuFocus *focus;
    u8 pad_8[0x28];
    s32 slot[65];
    s32 unk134;
    s32 unk138;
} MenuState;
typedef struct {
    s32 slot;
    u8 pad_4[0x48];
} ItemInfo;
typedef struct {
    u16 icon;
    s16 frame;
    s16 kind;
    s16 id;
    u16 pad_8;
} MenuGridCell;
typedef struct {
    u8 pad_0[0x14];
    FontHolder *holder;
    u8 pad_18[0x8];
    s32 w;
    s32 h;
    u8 pad_28[0x8];
    s32 flags;
    f32 margin_x;
    f32 margin_y;
    s32 selected_cell;
    s32 rows;
    s32 cols;
    MenuGridCell *cells;
} MenuItemGrid;
extern MenuState D_001D5F70_1E950 __asm__("D_001D5F70");
extern ItemInfo D_001864D8[];
extern u8 D_0013D490[];
extern u8 D_0013D5C8_1E950[] __asm__("D_0013D5C8");
extern u8 D_0013E620[];
extern s32 D_0015F538 MACRO_ADDR;
extern short D_001602B0;
extern short D_00160390;
extern short D_00160394;
extern s32 func_001F9B70(s32);
extern void func_001F4630(s32);
extern s32 func_00200198(s32, s32);
extern void func_002008B8(s32, s32, s32, s32, s32, s32);
extern void func_00201640_1E950(s32, s32, s32, s32, u64, s32) __asm__("func_00201640");
extern s32 func_00234C98_1E950(s32, s64) __asm__("func_00234C98");
s32 func_0021E950(MenuItemGrid *grid);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/ui/menus/fun_0021d948.c, draw_menu_item_grid. */
s32 func_0021E950(MenuItemGrid *grid) {
    Font *font;
    MenuGridCell *cell;
    s32 focused;
    f32 start_x, column_step, start_y, y, row_step, x;
    f32 scale;
    s32 largest_dimension;
    s32 icon_width, icon_height;
    s32 i, j;
    s32 left, top, right, bottom;
    s32 id, frame_offset;
    u32 color;

    font = grid->holder->font;
    focused = D_001D5F70_1E950.focus->owner == grid;
    cell = grid->cells;
    func_00234C98_1E950(0x42, 0x8000000044L);
    func_00234C98_1E950(0x47, 0xB);
    func_001F4630(0);

    if (grid->cols >= 2) {
        start_x = grid->margin_x;
        column_step = (*(f32 *)&D_00160390) + (font->w - (start_x + start_x) - (*(f32 *)&D_00160390) * grid->cols) / (grid->cols - 1);
    } else {
        column_step = 0.0f;
        start_x = (font->w - (*(f32 *)&D_00160390)) * 0.5f;
    }

    if (grid->flags & 2) {
        row_step = (*(f32 *)&D_00160394) + 0.15f;
        start_y = grid->margin_y;
    } else if (grid->rows >= 2) {
        start_y = grid->margin_y;
        row_step = (*(f32 *)&D_00160394) + (font->h - (start_y + start_y) - (*(f32 *)&D_00160394) * grid->rows) / (grid->rows - 1);
    } else {
        row_step = 0.0f;
        start_y = (font->h - (*(f32 *)&D_00160394)) * 0.5f;
    }

    largest_dimension = grid->h;
    if (largest_dimension < grid->w) {
        largest_dimension = grid->w;
    }
    scale = (f32)(largest_dimension << 4) / (font->h < font->w ? font->w : font->h);
    icon_width = scale * (*(f32 *)&D_00160390);
    icon_height = scale * (*(f32 *)&D_00160394);

    y = start_y;
    for (i = 0; i < grid->rows; i++) {
        x = start_x;
        for (j = 0; j < grid->cols; j++) {
            top = scale * y;
            bottom = top + icon_height;
            left = scale * x;
            right = left + icon_width;
            if (focused && grid->selected_cell == cell - grid->cells) {
                color = ((func_001F9B70((D_0015F538 & 0x3F) - 0x20) + 0x40) * 0x10202) | 0x80000000;
                func_00201640_1E950(left - 0x30, top - 0x30, right + 0x30, bottom + 0x30, color, 1);
                func_00201640_1E950(left - 0x10, top - 0x10, right + 0x10, bottom + 0x10, (*(s32 *)&D_001602B0), 1);
            }
            if (cell->kind == 0 ? D_0013D5C8_1E950[cell->id] : D_0013D490[cell->id]) {
                frame_offset = 0;
                if ((u16)cell->kind == 0) {
                    id = cell->id;
                    if (D_001D5F70_1E950.slot[D_001864D8[id].slot] == id && !(grid->flags & 0x20)) {
                        frame_offset = 1;
                    }
                    if (frame_offset == 0) {
                        frame_offset = D_0013E620[id] ? 4 : 0;
                    }
                    if (D_001D5F70_1E950.unk134 != 0 && (grid->flags & 8)) {
                        frame_offset = 2;
                    }
                    if (D_001D5F70_1E950.unk138 != 0 && (grid->flags & 4)) {
                        frame_offset = 2;
                    }
                }
                func_002008B8(func_00200198(cell->icon, cell->frame + frame_offset), left, top, icon_width, icon_height, 0x80);
            }
            cell++;
            x += column_step;
        }
        y += row_step;
    }
    func_001F4748();
    return 2;
}

extern unsigned char D_001414F4 NOT_SDA;

/* A short-returning inline keeps retail's branch: the select happens in
   HImode, which has no movcc pattern, so jump.c cannot turn it into the
   xori/movz a promoted `short` local gets. */
static inline short pauseFlagState(void) {
    if (D_001414F4 != 1) {
        return 3;
    }
    return 0;
}

/* The select sits in a `static inline short` helper so that it stays a
   branch: jump.c turns an if into movz/movn only when its arm sets a
   full register, and the inline's short return value is a subreg. */
int func_0021EDD8(char *arg0) {
    char *p = *(char **)(arg0 + 0x34);
    *(short *)(p + 2) = pauseFlagState();
    return 0;
}

extern int D_001D53A0[];
extern void func_0020E180(int, int);

/* Tears down the 24 handles at arg0+0x44, skipping the ones in use.
   The switch's table is jtbl_001E8AD0; `off` before `slots` and a second
   slots2/off2 pair give retail's copies. */
int func_0021EE00(char *arg0) {
    int i;

    for (i = 0; i < 24; i++) {
        int off = i * 4;
        char *slots = arg0 + 0x44;
        if (*(int *)(slots + off) == 0) {
            continue;
        }
        if (*(unsigned char *)(arg0 + i + 0xA4) != 0) {
            continue;
        }
        if (D_001D53A0[i] == 0) {
            continue;
        }
        if (i == 7 && *(short *)(*(char **)(arg0 + 0x60) + 0xA6) == 0x4A) {
            unsigned char *o = *(unsigned char **)(arg0 + 0x44);
            if (o[0x52] != o[0x53]) {
                continue;
            }
        }
        switch (i) {
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 10:
        case 11:
        case 12: {
            unsigned char *o = *(unsigned char **)(arg0 + 0x44);
            if ((*(long *)(o + 0x50) & 0xFFFF0000L) == 0x99990000L
                && o[0x50] >= 0x4D && o[0x50] < 0x92) {
                continue;
            }
            break;
        }
        }
        {
            char *slots2 = arg0 + 0x44;
            int off2 = i * 4;
            DrawMobyList(*(int *)(slots2 + off2), 1);
        }
    }
    return 4;
}

int func_0021EF30(void) {
    return 0;
}

int func_0021EF38(void *arg0) {
    char *p = (char *)arg0;
    *(float *)(p + 0x38) = 3.14159274f;
    *(int *)(p + 0x34) = 0;
    *(int *)(p + 0x44) = 0;
    *(int *)(p + 0x48) = 0;
    return 0;
}

int func_0021EF60(char *arg0) {
    *(int *)(arg0 + 0x44) = func_002267C0(*(int *)(arg0 + 0x44));
    *(int *)(arg0 + 0x48) = func_002267C0(*(int *)(arg0 + 0x48));
    return 0;
}

extern char D_001864D0[];
extern char D_00187040[];
extern void *func_00226720_a(int) __asm__("func_00226720");
extern void func_0021F200(char *);

/* Keeps the item preview moby in step with the highlighted entry: drop it
   when the entry's class (D_001864D0 record +0x3A) is -1, spawn it in
   front of the camera focus when there is none yet, or respawn it with
   the old one's position and orientation when the class changed. */
int func_0021EFA0(char *arg0) {
    char *q = *(char **)(D_001D5F74 + 0x40);
    int item = *(short *)(*(int *)(q + 0x3C) * 10 + *(char **)(q + 0x48) + 6);
    short cur;
    char *rec;
    short want;

    cur = *(char **)(arg0 + 0x44) != 0 ? *(short *)(*(char **)(arg0 + 0x44) + 0xA6) : -1;
    rec = D_001864D0 + item * 0x4C;
    want = *(short *)(rec + 0x3A);
    if (want != -1 && cur == -1) {
        char *o = func_00226720_a(want);

        if (o != 0) {
            char *t = D_00187040;

            *(char **)(arg0 + 0x44) = o;
            *(short *)(o + 0x34) = 0;
            *(float *)(o + 0x10) = *(float *)(t + 0x140) + 6.0f;
            *(float *)(o + 0x14) = *(float *)(t + 0x144);
            *(float *)(o + 0x18) = *(float *)(t + 0x148) - 0.3f;
            *(float *)(o + 0x48) = 3.1415927f;
            *(void **)(o + 0x74) = (void *)func_0021F200;
            **(void ***)(o + 0x78) = arg0;
        }
    } else if (want == -1) {
        *(int *)(arg0 + 0x44) = func_002267C0(*(int *)(arg0 + 0x44));
    } else if (cur != want) {
        char *n = func_00226720_a(want);

        if (n != 0) {
            char *old;

            *(short *)(n + 0x34) = 0;
            old = *(char **)(arg0 + 0x44);
            qcopy(n + 0x10, old + 0x10);
            qcopy(n + 0x40, old + 0x40);
            *(int *)(n + 0x74) = *(int *)(old + 0x74);
            **(void ***)(n + 0x78) = arg0;
        }
        func_002267C0(*(int *)(arg0 + 0x44));
        *(char **)(arg0 + 0x44) = n;
    }
    return 0;
}

extern char *D_001D5F74 NOT_SDA;
extern void func_0020E180(int, int);
extern void func_001F4630(int);
extern void func_001F4748(void);
extern void *func_001FE540_id(int) __asm__("func_001FE540");
extern void func_00227A30(void *, char *);
extern void func_001F7560(void *, long, void *, int);

/* Draw/teardown callback (func_0021F610's family). The entry address is
   written inline in the index; a `char *e` local leaves an addu swap. */
int func_0021F118(char *arg0) {
    short buf[10];
    char *p = *(char **)(D_001D5F74 + 0x40);

    if (((unsigned char *)&D_0013D5C8)[*(short *)(*(int *)(p + 0x3C) * 10 + *(char **)(p + 0x48) + 6)] == 0) {
        return 0;
    }
    if (*(int *)(arg0 + 0x44) != 0) {
        DrawMobyList(*(int *)(arg0 + 0x44), 1);
        if (*(int *)(arg0 + 0x48) != 0) {
            DrawMobyList(*(int *)(arg0 + 0x48), 1);
        }
        return 8;
    }
    SetupGifPaging(0);
    func_00227A30(buf, arg0);
    buf[8] = 0x10;
    buf[9] = 3;
    func_001F7560(buf, 0x80FFA888L, func_001FE540_id(0x4F4D), -1);
    DoGifPaging();
    return 2;
}

extern float func_001FA748(float, float);

void func_0021F200(char *arg0) {
    *(float *)(arg0 + 0x48) = FastAddRots(*(float *)(arg0 + 0x48), 0.01f);
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021F238);

extern char *D_001D5F74 NOT_SDA;
extern void func_0020E180(int, int);

/* The entry address is written inline in the index, as in
   func_0021F118; a `char *e` local swapped the addu. */
int func_0021F610(char *arg0) {
    char *p = *(char **)(D_001D5F74 + 0x40);
    int v;

    if (((unsigned char *)&D_0013D5C8)[*(short *)(*(int *)(p + 0x3C) * 10 + *(char **)(p + 0x48) + 6)] == 0) {
        return 0;
    }
    v = *(int *)(arg0 + 0x44);
    if (v != 0) {
        DrawMobyList(v, 1);
    }
    v = *(int *)(arg0 + 0x48);
    if (v != 0) {
        DrawMobyList(v, 1);
    }
    return 8;
}

struct ItemPreviewPlacement {
    f32 alternate_x;
    f32 normal_x;
    f32 y;
    f32 z;
    u8 pad10[8]; /* Per-item record stride is 0x20. */
    f32 side_offset;
    f32 forward_offset;
};
struct ItemPreviewBinding {
    u8 pad0[0x30];
    s32 flags;
    u8 pad34[4];
    f32 rotation_angle;
};
struct ItemPreviewVars {
    struct ItemPreviewBinding *owner;
    u8 pad4[8];
    s32 item_index;
};
struct ItemPreviewMoby {
    u8 pad0[0x10];
    volatile f32 x;
    f32 y;
    f32 z;
    u8 pad1C[0x2C];
    f32 rotation_z;
    u8 pad4C[0x2C];
    struct ItemPreviewVars *preview_vars;
};
struct PreviewCamera {
    u8 pad0[0x140];
    f32 x;
    f32 y;
    f32 z;
};
extern struct PreviewCamera D_00187040_1F6A0 __asm__("D_00187040");
extern struct ItemPreviewPlacement D_001E0708[];
extern f32 func_001F9F90(f32);
extern f32 func_001F9FA8(f32);

/* Position the selected item relative to the preview camera. Owner flag bit 0 selects alternate_x; otherwise use normal_x. Rotate the side and forward offsets in the X/Y plane after adding the unrotated placement position.
   Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/textbin/update_item_preview_transform.c, update_item_preview_transform. */
void func_0021F6A0(struct ItemPreviewMoby *moby) {
    struct ItemPreviewBinding *preview = moby->preview_vars->owner;
    s32 item_index = moby->preview_vars->item_index;
    f32 side_offset;
    f32 forward_offset;
    f32 cosine;
    f32 sine;
    f32 negated_forward_offset;
    s32 flags = preview->flags;
    f32 y_offset;
    struct PreviewCamera *camera;

    moby->rotation_z = preview->rotation_angle;
    moby->x = D_00187040_1F6A0.x + ((flags & 1) ? D_001E0708[item_index].alternate_x
                                              : D_001E0708[item_index].normal_x);
    camera = &D_00187040_1F6A0;
    y_offset = D_001E0708[item_index].y;
    moby->y = camera->y + y_offset;
    moby->z = camera->z + D_001E0708[item_index].z;
    forward_offset = D_001E0708[item_index].forward_offset;
    side_offset = D_001E0708[item_index].side_offset;
    cosine = func_001F9F90(moby->rotation_z);
    negated_forward_offset = -forward_offset;
    sine = func_001F9FA8(moby->rotation_z);
    moby->x += negated_forward_offset * sine + side_offset * cosine;
    moby->y += forward_offset * cosine + side_offset * sine;
}

extern int D_0013D48C;
extern int D_0015F6E4 MACRO_ADDR;
extern int D_0015F6FC_m __asm__("D_0015F6FC") MACRO_ADDR;
extern int D_0015F690 MACRO_ADDR;

/* Pad handler (func_0021ACD8's family). A separate `char *` local for
   each block that reads the pad word gives retail's pattern: the %hi
   kept in a register and the %lo rebuilt before each use. */
int func_0021F7D0(char *arg0) {
    char *pad = D_0013CA40;
    int v = *(int *)(pad + 0x1C4);

    if (v & 0xD00) {
        if (*(int *)(arg0 + 0x30) & 0x20) {
            D_001A0414 = D_0015EE84;
        }
        return -1;
    }
    if (v & 0x10) {
        char *g;
        int t;
        if (*(int *)(arg0 + 0x30) & 0x20) {
            D_001A0414 = D_0015EE84;
        }
        g = D_001D5F70;
        t = *(int *)(*(char **)(g + 4) + 0x38);
        if (t != 0) {
            *(int *)(g + 8) = t;
            return 0;
        }
        if (*(int *)(g + 0x124) == 0) {
            return -1;
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x20) {
            D_0013D48C = 0;
            D_0015F6E4 = -1;
            D_0015F690 = 1;
            D_0015F6FC_m = 1;
        }
    }
    return 0;
}

extern void func_001153FC(void *, int, int); /* memset */
extern void func_00234C98_l(int, long) __asm__("func_00234C98");
extern void func_001F6EA8(int, int, long, void *, int);
extern void func_001F68E8_c(int, int, long, void *, int) __asm__("func_001F68E8");

/* DrawQuitGameMenu: a text box (zeroed, then sized from arg0+0x20/0x24)
   with text 0x4F6D, text 0x4F3F centred 0x40 above the bottom, and the
   two answers 0x5250/0x5254 at 0x28 and 0x14 above it, left-aligned so
   that the wider of the two (func_001F6600) is centred. The three y
   positions are computed before the calls (retail keeps them in saved
   registers); the box's y halfword is stored last, after the other
   fields in order, which gives retail's store schedule. */
int func_0021F898(char *arg0) {
    short box[12];
    int w = *(int *)(arg0 + 0x20);
    int h = *(int *)(arg0 + 0x24);
    int cx;
    int y1, y2, y3;
    int a, b, x;

    func_001153FC(box, 0, 0x18);
    cx = w / 2;
    y1 = h - 0x40;
    y2 = h - 0x28;
    y3 = h - 0x14;
    box[2] = 4;
    box[3] = w - 4;
    box[4] = cx;
    box[5] = 6;
    box[6] = w;
    box[7] = h;
    box[8] = 0x10;
    box[9] = 1;
    box[1] = h;
    SetupGifPaging(0);
    func_00234C98_l(0x47, 0x2004B);
    func_001F7560(box, 0x80FFA888L, func_001FE540_id(0x4F6D), -1);
    FontPrintCenter(cx, y1, 0x80FFA888L, func_001FE540_id(0x4F3F), -1);
    a = func_001F6600(func_001FE540_id(0x5250), -1);
    b = func_001F6600(func_001FE540_id(0x5254), -1);
    if (b >= a) {
        a = b;
    }
    x = (*(int *)(arg0 + 0x20) - a) >> 1;
    func_001F68E8_c(x, y2, 0x80FFA888L, func_001FE540_id(0x5250), -1);
    func_001F68E8_c(x, y3, 0x80FFA888L, func_001FE540_id(0x5254), -1);
    DoGifPaging();
    return 2;
}

extern char D_00187040[];
extern void func_00220128(void *);
extern void *func_00226720_a(int) __asm__("func_00226720");

/* Attach marker object 0x46E to arg0, parked just above the camera
   focus in D_00187040's 0x140 block, and point it back at its owner
   through the node at +0x78. Always returns 0. */
int func_0021FA50(void *arg0) {
    char *o = (char *)func_00226720_a(0x46E);
    char *t;

    if (o != 0) {
        t = D_00187040;
        *(char **)((char *)arg0 + 0x44) = o;
        *(short *)(o + 0x34) = 0;
        *(float *)(o + 0x10) = *(float *)(t + 0x140) + 8.0f;
        *(float *)(o + 0x14) = *(float *)(t + 0x144) + 0.5f;
        *(float *)(o + 0x18) = *(float *)(t + 0x148) - 0.1f;
        *(float *)(o + 0x44) = -1.9f;
        *(void **)(o + 0x74) = (void *)func_00220128;
        **(void ***)(o + 0x78) = arg0;
    }
    return 0;
}

extern int func_002267C0(int);

int func_0021FAF8(char *arg0) {
    *(int *)(arg0 + 0x44) = func_002267C0(*(int *)(arg0 + 0x44));
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0021FB28); /* DrawItemsMenu */

extern char D_00187040[];
/* 1.3f, in small data (gp -0x695C). The const float view is what the
   code reads (read-only, so its load is scheduled above the stores); the
   2-byte view, referenced last, makes the assembler use $gp. */
extern const float D_001603A4_f __asm__("D_001603A4") MACRO_ADDR;
extern short D_001603A4_s __asm__("D_001603A4");
extern char D_001603A8[];
extern int D_001E0B88[];
extern int D_0013E600[];
extern int func_00116248(char *, const char *, ...);
extern void func_001F6CF8_c(int, int, long, char *, int) __asm__("func_001F6CF8");

/* DrawGBsShipMenu (maybe): parks the marker object at arg0+0x44 by the
   camera focus in D_00187040, draws it, then prints "%s %d %s %d" (texts
   0x4F4F and 0x4F53, the number of D_0014BFC0 flags set for the current
   map, and the map's D_001E0B88 total) twice, as shadow and text. The
   flag count reads the map index inside its loop: PRE then computes
   D_001A01F0's %hi once, straight into the register that is kept for the
   second read, as retail has it (a pointer set before the loop leaves a
   copy). */
int func_0021FF80(char *arg0) {
    char buf[0x100];
    char *t = D_00187040;
    char *o = *(char **)(arg0 + 0x44);
    int count;

    *(float *)(o + 0x10) = *(float *)(t + 0x140) + 8.0f;
    *(float *)(o + 0x14) = *(float *)(t + 0x144) + D_001603A4_f;
    *(float *)(o + 0x18) = *(float *)(t + 0x148) - 0.1f;
    if (*(int *)(arg0 + 0x44) != 0) {
        DrawMobyList(*(int *)(arg0 + 0x44), 1);
    }
    SetupGifPaging(0);
    count = 0;
    {
        int k;
        for (k = 0; k < 4; k++) {
            if (D_0014BFC0[D_001A01F0[0x89] * 4 + k] != 0) {
                count = count + 1;
            }
        }
    }
    func_00116248(buf, D_001603A8, func_001FE540_id(0x4F4F), count,
                  func_001FE540_id(0x4F53), D_001E0B88[D_001A01F0[0x89]]);
    func_001F6CF8_c(*(int *)(arg0 + 0x20) - 0x10, (D_0013E600[1] >> 1) - 8,
                    0x80000000L, buf, -1);
    func_001F6CF8_c(*(int *)(arg0 + 0x20) - 0x11, (D_0013E600[1] >> 1) - 9,
                    0x80FFA888L, buf, -1);
    DoGifPaging();
    if (0) {
        /* no code: registers the 2-byte view last (see above) */
        (void)D_001603A4_s;
    }
    return 8;
}

extern float func_001FA748(float, float);

void func_00220128(void *arg0) {
    *(float *)((char *)arg0 + 0x40) =
        FastAddRots(*(float *)((char *)arg0 + 0x40), 0.02f);
}

/* func_00234C98's second parameter is 64-bit (as in draw.c and
   mobyfunc.c); the (int, int) declaration below this point is kept for
   the functions matched against it. */
extern void func_00234C98_l(int, long) __asm__("func_00234C98");
extern int func_00116248(char *, const char *, ...); /* sprintf */
extern int D_0013D530[];
extern char D_001E02B0[];
extern char D_001603A0[]; /* "%d" */
extern char D_001603B8[]; /* "%d/" */
extern char D_001603C0[]; /* "%d,%03d/" */
extern char D_001603D0[]; /* "%d,%03d" */

/* Draws the current item's count banner, gated like func_0021F118: the
   item's text id 0x4F52 when its D_001E02B0 record has no counter,
   otherwise "have/total" with thousands separators. The second sprintf
   appends at text + the first one's length; forming that pointer in each
   arm (`q = text + sprintf(...)`) lets gcc merge the two into one addu at
   the join, as retail has it. */
int func_00220160(char *arg0) {
    char text[0x50];
    short box[10];
    char *p = *(char **)(D_001D5F74 + 0x40);
    int id = *(short *)(*(int *)(p + 0x3C) * 10 + *(char **)(p + 0x48) + 6);
    int have, total;
    char *rec;

    if (((unsigned char *)&D_0013D5C8)[id] == 0) {
        return 0;
    }
    have = D_0013D530[id];
    rec = D_001E02B0 + id * 0x18;
    total = *(unsigned short *)(rec + 0xE);
    func_00234C98_l(0x42, 0x44);
    func_00234C98_l(0x47, 0xB);
    if (*(unsigned short *)(rec + 8) == 0) {
        func_00116248(text, func_001FE540_id(0x4F52));
    } else {
        char *q;
        if (have < 1000) {
            q = text + func_00116248(text, D_001603B8, have);
        } else {
            q = text + func_00116248(text, D_001603C0, have / 1000, have % 1000);
        }
        if (total < 1000) {
            func_00116248(q, D_001603A0, total);
        } else {
            func_00116248(q, D_001603D0, total / 1000, total % 1000);
        }
    }
    SetupGifPaging(0);
    func_00227A30(box, arg0);
    box[8] = 0x10;
    box[9] = 3;
    func_001F7560(box, 0x80FFA888L, text, -1);
    DoGifPaging();
    return 2;
}

extern void func_00234C98(int, int);
extern void func_00205E70(void);

int func_00220338(void) {
    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0xB);
    func_00205E70();
    return 8;
}

extern void func_001F65A8(void);
extern void func_001F6598(void);
extern void func_001F6968_c(int, int, long, void *, int) __asm__("func_001F6968");

/* DrawMissionsMenu: the menu's text lines (0x4EEE, 0x4EEF only when
   D_0015EE84 is set, then 0x4EFA, 0x4EFB, 0x4EFC, 0x4EE0) in one column,
   left-aligned so the widest (func_001F6620) is centred in arg0+0x20 but
   at least 2 in, and spaced arg0+0x24 / (6 or 7 lines) apart starting a
   line minus 6 down. */
int func_00220370(char *arg0) {
    int maxw, w;
    int x, y, step;

    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0xB);
    SetupGifPaging(0);
    maxw = func_001F6620(func_001FE540_id(0x4EEE), -1);
    w = func_001F6620(func_001FE540_id(0x4EFA), -1);
    if (w >= maxw) {
        maxw = w;
    }
    if (D_0015EE84 != 0) {
        w = func_001F6620(func_001FE540_id(0x4EEF), -1);
        if (w >= maxw) {
            maxw = w;
        }
    }
    w = func_001F6620(func_001FE540_id(0x4EFB), -1);
    if (w >= maxw) {
        maxw = w;
    }
    w = func_001F6620(func_001FE540_id(0x4EFC), -1);
    if (w >= maxw) {
        maxw = w;
    }
    w = func_001F6620(func_001FE540_id(0x4EE0), -1);
    if (w >= maxw) {
        maxw = w;
    }
    x = (*(int *)(arg0 + 0x20) - maxw) >> 1;
    if (x < 2) {
        x = 2;
    }
    step = *(int *)(arg0 + 0x24) / (D_0015EE84 != 0 ? 7 : 6);
    func_001F65A8();
    y = step - 6;
    func_001F6968_c(x, y, 0x80FFA888L, func_001FE540_id(0x4EEE), -1);
    if (D_0015EE84 != 0) {
        y += step;
        func_001F6968_c(x, y, 0x80FFA888L, func_001FE540_id(0x4EEF), -1);
    }
    y += step;
    func_001F6968_c(x, y, 0x80FFA888L, func_001FE540_id(0x4EFA), -1);
    y += step;
    func_001F6968_c(x, y, 0x80FFA888L, func_001FE540_id(0x4EFB), -1);
    y += step;
    func_001F6968_c(x, y, 0x80FFA888L, func_001FE540_id(0x4EFC), -1);
    y += step;
    func_001F6968_c(x, y, 0x80FFA888L, func_001FE540_id(0x4EE0), -1);
    func_001F6598();
    DoGifPaging();
    return 2;
}
__asm__(".section .text\n\tnop\n");

extern void func_001F4630(int);
extern void func_001F4748(void);
/* func_001F68E8 is defined above with pointer parameters; this site
   passes a packed 64-bit colour in $a2, and func_001FE540 above is
   declared (void) while retail's caller here passes an id in $a0 --
   reach both through aliases rather than redeclaring them. */
extern void func_001F68E8_c(int, int, long, void *, int)
    __asm__("func_001F68E8");
extern void *func_001FE540_id(int) __asm__("func_001FE540");

/* Sibling of func_00220338 above: the same two func_00234C98 setup
   calls, then two banner draws. 0x80FFA888 is spelled `long` (64-bit)
   so it builds via ori/dsll/ori rather than a sign-extending lui. */
int func_00220600(void) {
    VU1_addGSregister(0x42, 0x44);
    VU1_addGSregister(0x47, 0xB);
    SetupGifPaging(0);
    func_001F68E8_c(4, 7, 0x80FFA888L, func_001FE540_id(0x4EE0), -1);
    func_001F68E8_c(4, 0x17, 0x80FFA888L, func_001FE540_id(0x4F05), -1);
    DoGifPaging();
    return 2;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00220690);

extern short D_001602B0;              /* SDA, gp -0x6A50 */
extern void func_00201640(int, int, int, int, long, long);
extern int func_00200198(int, int);
extern void func_00200468(int, int, int, int, int, int);

void func_002208F8(int x, int y, int flag) {
    func_00201640(x - 5, y - 5, x + 5, y + 5, 0x80FFA888L, 0);
    func_00201640(x - 4, y - 4, x + 4, y + 4, *(int *)&D_001602B0, 0);
    if (flag != 0) {
        int c = GetIconFrame(0xE99E, 1);
        HudSprite(c, x - 0xD, y - 0x12, 0x1E, 0x1E, 0x80);
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_002209A0);

extern void func_001F5800(int, int, int, int, int, int, int, int, long,
                          long);
extern short D_00151880[];
extern long D_001A0448;

/* Eight register arguments ($a0-$a3, $t0-$t3) then two 64-bit stack
   slots -- both written with `sd`, so they are `long`, not `long long`
   (which would be 128-bit here). */
int func_00220C90(void *arg0) {
    if (*(int *)((char *)arg0 + 0x44) < 2) {
        return 0;
    }
    DrawTexturedQuad(0, 0, D_00151880[0xB0], D_00151880[0xB1], 0, 0,
                  *(int *)((char *)arg0 + 0x38),
                  *(int *)((char *)arg0 + 0x3C), 0x80808080L, D_001A0448);
    return 0x10;
}

extern int func_00226EA8(int);

/* Sibling of func_00220DA0 below: the same slot set (0x44/0x48/0x4C/
   0x50/0x54/0x5C) on the same object, seeded here instead of torn down.
   The tail stores go 5C, 50, 54. */
int func_00220D08(void *arg0) {
    char *s = (char *)arg0;

    *(int *)(s + 0x44) = 0;
    *(int *)(s + 0x48) = func_00226EA8(*(int *)(s + 0x34) & 0x200);
    *(int *)(s + 0x4C) = func_00226EA8(*(int *)(s + 0x34) & 0x200);
    if ((*(int *)(s + 0x34) & 0x200) == 0) {
        if (*(int *)(s + 0x48) == 0) {
            *(int *)(s + 0x48) = func_00226EA8(1);
        }
        if (*(int *)(s + 0x4C) == 0) {
            *(int *)(s + 0x4C) = func_00226EA8(1);
        }
    }
    *(int *)(s + 0x5C) = 0;
    *(int *)(s + 0x50) = -1;
    *(int *)(s + 0x54) = -1;
    return 0;
}

extern int func_00226F68(int);

int func_00220DA0(void *arg0) {
    char *s = (char *)arg0;
    *(int *)(s + 0x48) = func_00226F68(*(int *)(s + 0x48));
    *(int *)(s + 0x4C) = func_00226F68(*(int *)(s + 0x4C));
    *(int *)(s + 0x50) = -1;
    *(int *)(s + 0x54) = -1;
    *(int *)(s + 0x44) = -1;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00220DF0);

INCLUDE_ASM("asm/nonmatchings/text", func_00221380);

/* The per-controller request slots in D_00137C80 (declared above as a
   flat int array). */
typedef struct { int a, b; } Pair8;
typedef struct {
    char pad[0x2C8];
    Pair8 req0[6]; /* 0x2C8 */
    Pair8 req2[6]; /* 0x2F8 */
} Tbl137C80;
extern Tbl137C80 D_00137C80_t __asm__("D_00137C80");
extern int D_0015EE88 MACRO_ADDR;

/* Pause sub-state machine at arg0+0x44. States 0 and 2 wait for their
   request (arg0+0x48 / +0x4C) and for D_001517D0[4] to clear, then run
   func_00217628 with the controller's slot and advance (or go to -1 on
   failure); states 1 and 3 advance once D_001517D0[4] clears. Always
   returns 0. A switch with its cases in source order 0-3 gives retail's
   case tree and block order; reading the slot's two words as fields of a
   struct gives two address adds (one becomes retail's copy). */
int func_00221688(char *arg0) {
    switch (*(int *)(arg0 + 0x44)) {
    case 0:
        if (*(int *)(arg0 + 0x48) != 0 && D_001517D0[4] == 0) {
            if (func_00217628_3(*(int *)(arg0 + 0x48),
                                D_00137C80_t.req0[D_0015EE88].a,
                                D_00137C80_t.req0[D_0015EE88].b) != 0) {
                *(int *)(arg0 + 0x44) += 1;
            } else {
                *(int *)(arg0 + 0x44) = -1;
            }
        }
        break;
    case 1:
        if (D_001517D0[4] == 0) {
            *(int *)(arg0 + 0x44) = 2;
        }
        break;
    case 2:
        if (*(int *)(arg0 + 0x4C) != 0 && D_001517D0[4] == 0) {
            if (func_00217628_3(*(int *)(arg0 + 0x4C),
                                D_00137C80_t.req2[D_0015EE88].a,
                                D_00137C80_t.req2[D_0015EE88].b) != 0) {
                *(int *)(arg0 + 0x44) += 1;
            } else {
                *(int *)(arg0 + 0x44) = -1;
            }
        }
        break;
    case 3:
        if (D_001517D0[4] == 0) {
            *(int *)(arg0 + 0x44) = 4;
        }
        break;
    }
    return 0;
}

/* Returns the packed texture handle the draw call takes as its last
   64-bit argument, so it is `long`: retail stores $v0 straight to the
   stack slot with `sd`, without sign-extending it. */
extern long func_00205520(int);

/* Sibling of func_00220C90 above: same 0x44 guard and the same
   func_001F5800 draw, twice, with the colour held in one local because
   retail keeps it in a callee-saved register across both calls. */
int func_002217C8(void *arg0) {
    long c;

    if (*(int *)((char *)arg0 + 0x44) < 4) {
        return 0;
    }
    c = 0x80808080L;
    SetupGifPaging(0);
    DrawTexturedQuad(0, 0, 0x100, 0x100, 0, 0, 0x100, 0x100, c,
                  func_00205520(*(int *)((char *)arg0 + 0x48)));
    DrawTexturedQuad(0x100, 0, 0x100, 0x100, 0, 0, 0x100, 0x100, c,
                  func_00205520(*(int *)((char *)arg0 + 0x4C)));
    DoGifPaging();
    return 8;
}

extern char D_001A01F0_c[] __asm__("D_001A01F0");
extern char D_00151880_c[] __asm__("D_00151880");
extern int D_0015F538 MACRO_ADDR;
extern int D_001DE2E8[];
extern void func_00200CA0(long, int, int, int, int, int, int, int, int, int);

/* Draws the three overlay layers (textures at D_001A01F0 +0x258/0x260/
   0x268) over the whole screen (D_00151880's +0x160/+0x162 extent).
   States 6, 13 and 17 blink the upper two layers on D_0015F538; the
   others inset them by D_001DE2E8[state] and scroll the bottom one.
   Each branch reaches the two globals through its own block-scoped
   pointers, which gives retail's registers. */
int func_00221888(void) {
    char *g = D_001A01F0_c;
    int s = *(int *)(g + 0x228);
    int k, d, u, t;

    if (s < 0) {
        return 0;
    }
    if (s == 6 || s == 13 || s == 17) {
        char *g2;
        char *fb;

        func_00234C98_l(0x47, 0);
        func_00234C98_l(8, 5);
        g2 = D_001A01F0_c;
        fb = D_00151880_c;
        func_00200CA0(*(long *)(g2 + 0x258), 0, 0, 7, 7,
                      *(short *)(fb + 0x160) << 4, *(short *)(fb + 0x162) << 4, 0, 0, 0x80);
        func_00234C98_l(0x47, 0x360B);
        if (D_0015F538 % 60 < 40) {
            func_00200CA0(*(long *)(g2 + 0x260), 0, 0, 7, 7,
                          *(short *)(fb + 0x160) << 4, *(short *)(fb + 0x162) << 4, 0, 0, 0x80);
        }
        if (D_0015F538 % 150 < 90) {
            func_00200CA0(*(long *)(g2 + 0x268), 0, 0, 7, 7,
                          *(short *)(fb + 0x160) << 4, *(short *)(fb + 0x162) << 4, 0, 0, 0x80);
        }
    } else {
        char *fb;

        t = D_0015F538 + s * 0x2AB;
        k = D_001DE2E8[s];
        d = k * 2;
        u = t % 2048;
        func_00234C98_l(0x47, 0);
        func_00234C98_l(8, 0);
        fb = D_00151880_c;
        func_00200CA0(*(long *)(g + 0x258), k, k, 7, 7,
                      (*(short *)(fb + 0x160) << 4) - d, (*(short *)(fb + 0x162) << 4) - d, u, 0, 0x80);
        func_00234C98_l(0x47, 0x360B);
        func_00234C98_l(8, 5);
        func_00200CA0(*(long *)(g + 0x260), k, k, 7, 7,
                      (*(short *)(fb + 0x160) << 4) - d, (*(short *)(fb + 0x162) << 4) - d, 0, 0, 0x80);
        func_00200CA0(*(long *)(g + 0x268), k, k, 7, 7,
                      (*(short *)(fb + 0x160) << 4) - d, (*(short *)(fb + 0x162) << 4) - d, 0, 0, 0x80);
    }
    return 4;
}

extern float func_001FA7D8(float, int);
extern int func_001F9B70(int); /* abs */

extern char D_001864D0[];
extern int D_0015F538 MACRO_ADDR;
extern char D_001603D8[];
extern char D_001603E0[];
extern short D_001602B0; /* SDA, gp -0x6A50 */

/* The icon slots are a real member: written as pointer arithmetic, gcc
   strength-reduces the index into a 7th saved register (retail uses 6). */
typedef struct {
    char pad[0x30];
    int slots[8];
} PauseIcons;

/* Draws the pause menu's ring of 8 icons and the quick-select overlay.
   arg0 is the pause-state object: unk20/unk24 give the viewport
   width/height (used for the ring's center and radius), unk50 the
   selected slot (drawn with a pulsing highlight box), and slots[i]
   indexes an icon-info table (D_001864D0, 0x4C bytes/entry) and a byte
   flags table (D_0013E620) when nonzero. */
int func_00221B58(char *arg0) {
    int v0, v1;
    int flag;
    float cx, cy, radius;
    char *iconTab;
    int i;

    SetupGifPaging(0);
    v1 = *(int *)(arg0 + 0x20);
    iconTab = D_001864D0;
    v0 = *(int *)(arg0 + 0x24);
    flag = v1 < v0;
    /* v1 doubles as `selected`: retail's movz reuses the same register
       that already holds v1 (loaded straight into it), so the C must
       overwrite v1 itself rather than assign a fresh local -- that
       fresh local costs an extra `move` before the conditional one. */
    cx = (float)v1 * 0.5f;
    cy = (float)v0 * 0.5f;
    if (flag == 0) {
        v1 = v0;
    }
    radius = (float)v1 * 0.5f - 40.0f;
    i = 0;
    do {
        float ang, t, x, y;
        int idx;

        ang = (float)i * 0.7853982f + -1.5707964f;
        t = FastNormalizeAngle(ang, flag);
        x = cx + FastCos(t) * radius;
        y = cy + FastSin(t) * radius;
        if (i == *(int *)(arg0 + 0x50)) {
            unsigned int color = ((func_001F9B70((D_0015F538 & 0x3F) - 0x20) + 0x40) * 0x10202) | 0x80000000;
            func_00201640((int)x - 0x13, (int)y - 0x13, (int)x + 0x13, (int)y + 0x13, color, 0);
            func_00201640((int)x - 0x12, (int)y - 0x12, (int)x + 0x12, (int)y + 0x12, *(int *)&D_001602B0, 0);
        }
        idx = ((PauseIcons *)arg0)->slots[i];
        if (idx == 0) {
            func_00201640((int)x - 0xF, (int)y - 0xF, (int)x + 0xF, (int)y + 0xF, 0x40404040L, 0);
        } else {
            int id = GetIconFrame(*(unsigned short *)(iconTab + idx * 0x4C + 0x38), D_0013E620[idx] == 0 ? 0 : 4);
            HudSprite(id, (int)x - 0x11, (int)y - 0x11, 0x20, 0x20, 0x80);
        }
        i += 1;
        flag = 0xE99E;
    } while (i < 8);
    HudSprite(GetIconFrame(0xE99E, 0), 8, 0x27, 0x20, -0x20, 0x80);
    HudSprite(GetIconFrame(0xE99E, 0), *(int *)(arg0 + 0x20) - 0xA, 0x27, -0x20, -0x20, 0x80);
    func_001F68E8_c(0x28, 0xF, 0x80FFA888L, D_001603D8, -1);
    func_001F68E8_c(*(int *)(arg0 + 0x20) - 0x3C, 0xF, 0x80FFA888L, D_001603E0, -1);
    DoGifPaging();
    return 2;
}

extern void func_0022ED80(int, int, int);
extern void func_001FBC80(int, void *, int);
extern int func_001F98C0(int);
extern void func_001F4E08(int);
extern float D_0015F53C MACRO_ADDR;
extern int D_0016044C_i __asm__("D_0016044C") MACRO_ADDR;
typedef struct {
    int w0;              /* 0x00 */
    unsigned char *buf;  /* 0x04 */
    int e[2];            /* 0x08 */
    int flags;           /* 0x10 */
} Row14;

/* Pad handler for a row list of toggles (func_002222F8's sibling, one
   `char *` pad local per block). While the timer at arg0+0x3C runs it
   counts down and sets D_0015F53C to min(t, 4) / 4. Otherwise, for the
   active item: 0xD00/0x10 leave, 0x1000/0x4000 move the cursor with a
   notify on change, and 0x40 flips the current row's byte, or for a row
   with flag 1 starts the timer and flips D_0016044C (func_001FBC80 once
   it is set). The row is indexed directly at each use (index-first addu)
   and the cursor compare is written new != old, as retail's beq has it. */
int func_00221E60(char *arg0) {
    char *g = D_001D5F70;
    int sel = *(char **)(*(char **)(g + 4) + 0x40) == arg0;

    if (*(int *)(arg0 + 0x3C) != 0) {
        int t = *(int *)(arg0 + 0x3C) - 1;
        *(int *)(arg0 + 0x3C) = t;
        D_0015F53C = (float)(t < 5 ? t : 4) * 0.25f;
        return 0;
    }
    if (!sel) {
        return 0;
    }
    {
        char *pad = D_0013CA40;
        if (*(int *)(pad + 0x1C4) & 0xD00) {
            if (*(int *)(g + 0x124) == 0) {
                return 1;
            }
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x10) {
            char *g2 = D_001D5F70;
            int t = *(int *)(*(char **)(g2 + 4) + 0x38);
            if (t != 0) {
                *(int *)(g2 + 8) = t;
                return 0;
            }
            if (*(int *)(g2 + 0x124) == 0) {
                return -1;
            }
        }
    }
    {
        char *pad3 = D_0013CA40;
        int old = *(int *)(arg0 + 0x38);
        if ((*(int *)(pad3 + 0x1C4) & 0x1000) && old != 0) {
            *(int *)(arg0 + 0x38) = old - 1;
        }
        {
            char *pad4 = D_0013CA40;
            if (*(int *)(pad4 + 0x1C4) & 0x4000) {
                int c = *(int *)(arg0 + 0x38);
                if ((*(Row14 **)(arg0 + 0x34))[c + 1].w0 != 0) {
                    *(int *)(arg0 + 0x38) = c + 1;
                }
            }
        }
        {
            char *pad5 = D_0013CA40;
            if (*(int *)(pad5 + 0x1C4) & 0x40) {
                func_0022ED80(0, 0x11, *(int *)(arg0 + 0x14));
                if ((*(Row14 **)(arg0 + 0x34))[*(int *)(arg0 + 0x38)].flags & 1) {
                    if (*(unsigned char *)&D_0016044C_i) {
                        char *g3 = D_001D5F70;
                        func_001FBC80(6, *(void **)(g3 + 4), 0);
                    } else {
                        FadeToBlack(4);
                        *(int *)(arg0 + 0x3C) = func_001F98C0(0x10);
                        *(unsigned char *)&D_0016044C_i = !*(unsigned char *)&D_0016044C_i;
                    }
                } else {
                    unsigned char *buf = (*(Row14 **)(arg0 + 0x34))[*(int *)(arg0 + 0x38)].buf;
                    if (buf != 0) {
                        buf[0] = !buf[0];
                    }
                }
            }
        }
        if (*(int *)(arg0 + 0x38) != old) {
            func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
        }
    }
    return 0;
}
__asm__(".section .text\n\tnop\n");

INCLUDE_ASM("asm/nonmatchings/text", func_00222070); /* DrawCheatsMenu */

extern void func_0022ED80(int, int, int);
typedef struct {
    int w0;              /* 0x00 */
    unsigned char *buf;  /* 0x04 */
    int e[4];            /* 0x08 */
} Row18;

/* Pad handler for a row list (func_0021ACD8's gate, one `char *` pad
   local per block): the cursor at arg0+0x38 moves down on 0x1000 and up
   on 0x4000 while the next row is used, with a func_0022ED80 notify on
   change; the current row's used entries (up to 4) are counted, and 0x40
   cycles the row's byte selector through them. The row is indexed with a
   fresh read of the cursor after the notify (reorg skips that load on the
   no-notify path, where the compare already left it in $v0), and the
   count walks an `int *e = rec->e` set before its first test, which puts
   `rec + 8` in the test's delay slot as retail does. */
int func_002222F8(char *arg0) {
    char *g = D_001D5F70;

    if (*(char **)(*(char **)(g + 4) + 0x40) != arg0) {
        return 0;
    }
    {
        char *pad = D_0013CA40;
        if (*(int *)(pad + 0x1C4) & 0xD00) {
            if (*(int *)(g + 0x124) == 0) {
                return 1;
            }
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x10) {
            char *g2 = D_001D5F70;
            int t = *(int *)(*(char **)(g2 + 4) + 0x38);
            if (t != 0) {
                *(int *)(g2 + 8) = t;
                return 0;
            }
            if (*(int *)(g2 + 0x124) == 0) {
                return -1;
            }
        }
    }
    {
        char *pad3 = D_0013CA40;
        int old = *(int *)(arg0 + 0x38);
        if ((*(int *)(pad3 + 0x1C4) & 0x1000) && old != 0) {
            *(int *)(arg0 + 0x38) = old - 1;
        }
        {
            char *pad4 = D_0013CA40;
            if (*(int *)(pad4 + 0x1C4) & 0x4000) {
                int c = *(int *)(arg0 + 0x38);
                if ((*(Row18 **)(arg0 + 0x34))[c + 1].w0 != 0) {
                    *(int *)(arg0 + 0x38) = c + 1;
                }
            }
        }
        if (old != *(int *)(arg0 + 0x38)) {
            func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
        }
    }
    {
        Row18 *rec = &(*(Row18 **)(arg0 + 0x34))[*(int *)(arg0 + 0x38)];
        int n = 0;
        char *pad5;
        int *e;

        e = rec->e;
        if (e[0] != 0) {
            do {
                n++;
            } while (e[n] != 0 && n < 4);
        }
        pad5 = D_0013CA40;
        if (*(int *)(pad5 + 0x1C4) & 0x40) {
            unsigned char *buf = rec->buf;
            if (buf != 0) {
                buf[0] = (buf[0] + 1) % n;
                func_0022ED80(0, 0x11, *(int *)(arg0 + 0x14));
            }
        }
    }
    return 0;
}

struct OptItem { int text; unsigned char *value; int names[4]; };
struct OptMenu { unsigned char pad0[0x20]; int x; int height; unsigned char pad28[0xC]; struct OptItem *items; int selected; };

/* Draws an options menu: each item's label at left, its current value's name at right, selected one highlighted. Adapted from Lombyte (MIT) for PAL: src/ui/menus/fun_00221460.c, FUN_00221460. */
int func_002224A8(struct OptMenu *m) {
    struct OptItem *p;
    struct OptItem *it;
    int n;
    int i;
    int step;
    int y;
    int color;

    func_00234C98_l(0x47, 0x2004B);
    func_001F4630(0);
    n = 0;
    for (p = m->items; p->text != 0; p++) {
        n++;
    }
    step = m->height / (n + 1);
    p = m->items;
    y = step - 8;
    for (i = 0; m->items[i].text != 0; i++) {
        it = &m->items[i];
        if (i == m->selected) {
            color = 0x8020FFFF;
        } else {
            color = 0x80FFA888;
        }
        func_001F68E8_c(0xC, y, color, func_001FE540_id(it->text), -1);
        func_001F6CF8_c(m->x - 0xC, y, 0x80FFA888, func_001FE540_id(it->names[*it->value]), -1);
        y += step;
    }
    func_001F4748();
    return 2;
}

typedef struct {
    unsigned short a;   /* +0 */
    short b;            /* +2 */
    short c;            /* +4 */
    short id;           /* +6 */
    short idx;          /* +8 */
} Item0A;
extern Item0A D_001CF4A0[];
extern Item0A D_001D6470[];
extern int D_001D6508[];
extern char D_001864D0[];
extern char D_001D1080[];

/* Builds the pause-menu item list from the 15 entries of D_001CF4A0
   that D_0013D5C8 enables. The flag needs its own `f = b != 0`:
   `(b != 0) << 2` folds into a branch. */
int func_00222640(void) {
    int i;
    int n = 0;
    char *m;

    for (i = 0; i < 15; i++) {
        int id = D_001CF4A0[i].id;
        if (((unsigned char *)&D_0013D5C8)[id] != 0) {
            Item0A *out = &D_001D6470[n];
            char *rec = D_001864D0 + id * 0x4C;
            unsigned char b = D_0013E620[id];
            int f = b != 0;
            out->a = *(unsigned short *)(rec + 0x38);
            out->b = f << 2;
            out->c = 0;
            out->id = id;
            out->idx = i;
            D_001D6508[n] = b ? *(short *)(rec + 0x42) : *(short *)(rec + 0x40);
            n++;
        }
    }
    m = D_001D1080;
    *(int *)(m + 0x40) = n;
    return 0;
}

extern Item0A D_001CEFA0[];
extern Item0A D_001CEFE0[];
extern Item0A D_001CF000[];
extern Item0A D_001CF020[];
extern Item0A D_001D6548[];
extern int D_001D65D8[];
extern int D_001D6610[];
extern char D_001864D0[];
extern char D_001D1408[];

/* func_00222640's sibling over four source tables. */
int func_00222708(void) {
    int i;
    int id;
    int n = 0;
    char *m;

    for (i = 0; i < 14; i++) {
        if (i < 6) {
            id = D_001CEFA0[i].id;
        } else if (i < 9) {
            id = D_001CEFE0[i - 6].id;
        } else if (i < 12) {
            id = D_001CF000[i - 9].id;
        } else {
            id = D_001CF020[i - 12].id;
        }
        if (((unsigned char *)&D_0013D5C8)[id] != 0) {
            char *rec = D_001864D0 + id * 0x4C;
            D_001D6548[n].a = *(unsigned short *)(rec + 0x38);
            D_001D6548[n].b = 0;
            D_001D6548[n].c = 0;
            D_001D6548[n].id = id;
            D_001D6548[n].idx = i;
            D_001D65D8[n] = *(short *)(rec + 0x40);
            D_001D6610[n] = *(short *)(rec + 0x44);
            n++;
        }
    }
    m = D_001D1408;
    *(int *)(m + 0x40) = n;
    return 0;
}

int func_00222840(void) {
    return 0;
}

typedef struct {
    unsigned short a;   /* +0 */
    short b;            /* +2 */
    int c;              /* +4 */
    unsigned short d;   /* +8 */
    short e;            /* +A */
} Out0C;
typedef struct {
    unsigned short a;   /* +0 */
    short pad2;
    unsigned short b;   /* +4 */
    short pad6;
    int pad8;
} Src0C;
extern int D_0013D618[];
extern Src0C D_001DE0C0[];
extern Out0C D_001D6648[];
extern short D_001602E0_s __asm__("D_001602E0");
extern int *D_001602E0_m __asm__("D_001602E0") MACRO_ADDR;

typedef struct {
    char pad00[0x30];
    int flags;          /* +0x30 */
    char pad34[0xC];
    int sel;            /* +0x40 */
} MenuObj40;

/* Menu builder: the item list from D_0013D618, then the selection. The
   scalar MACRO_ADDR view of D_001602E0 lets sched2 hoist its load, and
   the dead reference to the 2-byte view keeps `.extern D_001602E0, 2`
   last, so the assembler still uses $gp. A later 4-byte reference in
   this file would undo that. */
int func_00222848(MenuObj40 *arg0) {
    int i;

    for (i = 0; i < 20 && gGalaxyList[i] != 0; i++) {
        D_001D6648[i].a = *(unsigned short *)((char *)D_001DE0C0 + gGalaxyList[i] * 12);
        D_001D6648[i].b = 1;
        D_001D6648[i].c = 0;
        D_001D6648[i].d = *(unsigned short *)((char *)D_001DE0C0 + gGalaxyList[i] * 12 + 4);
    }
    D_001D6648[i].a = 0;
    arg0->sel = 0;
    arg0->flags |= 0x8000;
    for (i = 0; D_001602E0_m[i] != 0; i++) {
        char *slots = (char *)D_001A01F0;
        if (*(int *)(slots + 0x224) == D_001602E0_m[i]) {
            arg0->sel = i;
            break;
        }
    }
    if (0) {
        /* no code: registers the 2-byte view last, so the file ends with
           `.extern D_001602E0, 2` and the assembler uses $gp */
        (void)D_001602E0_s;
    }
    return 0;
}

extern int D_0013CC04 NOT_SDA;
extern char D_001D2678[];
extern char *D_001D5F78 NOT_SDA;

int func_00222950(void) {
    if (D_0013CC04 & 0x40) {
        D_001D5F78 = D_001D2678;
    }
    return 0;
}

extern void func_001FDF78(int, int, int, int);

int func_00222978(void *arg0) {
    int *p = (int *)arg0;
    func_001FDF78(p[6], p[6] + p[8], p[7], p[7] + p[9]);
    return 2;
}

extern void func_0022ED80(int, int, int);

/* Pause sub-menu input on the pad word D_0013CC04. One if/else-if chain
   falling to a single `return 0`; early returns in the 0x10 arm give a
   movn instead. */
int func_002229B0(char *arg0) {
    int v = D_0013CC04;

    if (v & 0x10) {
        char *g = D_001D5F70;
        int t = *(int *)(*(char **)(g + 4) + 0x38);
        if (t != 0) {
            *(int *)(g + 8) = t;
        } else if (*(int *)(g + 0x124) == 0) {
            return -1;
        }
    } else if (v & 0x800) {
        D_001A0414 = D_0015EE84;
        return 1;
    } else if (v & 0x40) {
        func_0022ED80(0, 0x11, *(int *)(arg0 + 0x14));
        return 1;
    } else if (v & 0x20) {
        char *g = D_001D5F70;
        *(int *)(g + 0xE4) = D_001A0414;
        *(int *)(g + 0xF0) = *(int *)(g + 4);
        *(int *)(g + 0xC) = 3;
        *(int *)(g + 0xF4) = 0xF;
        func_0022ED80(0, 0x11, *(int *)(arg0 + 0x14));
    }
    return 0;
}

extern int func_00226EA8(int);
extern char *D_001D5F74 NOT_SDA;

int func_00222A90(void *arg0) {
    *(int *)(D_001D5F74 + 0x84) = 0;
    *(int *)((char *)arg0 + 0x54) = func_00226EA8(0);
    return 0;
}

extern int func_00226F68(int);

int func_00222AD0(void *arg0) {
    *(int *)((char *)arg0 + 0x54) = func_00226F68(*(int *)((char *)arg0 + 0x54));
    return 0;
}

/*
 * Near-miss, same size (4 words differ, allocator only): in the default
 * arm retail keeps the D_001D5F70 base in $v1 (the register that held
 * D_0015EFB0) with the loaded pointer in $a0 and the value in $a1; we
 * get base $a1, pointer $a0, value $v1. Spellings tried (asm-differ
 * score, lower is better; all 0x94 bytes):
 *   this one (if/!=, base local per arm)                       25
 *   base, pointer and value as separate locals in the arm      45
 *   la-macro alias (MACRO_ADDR) for the base in the arm        235
 *   switch with default first and break                        2430
 * D_0013CBE4 is NOT a macro access: retail splits its lui into the
 * second beq's delay slot and branches past it, which only the split
 * form can do (MACRO_ADDR there trips check_macro_slots).
 */
/* D_0015EFB0 is SDA elsewhere; pause.cpp reaches it through the
   assembler macro. */
extern int D_0015EFB0_m __asm__("D_0015EFB0") MACRO_ADDR;
extern int D_0013CBE4;

/* variation: separate "g" local per arm (shadowed), not shared across
   the whole function, to see if that lets the allocator pick the
   per-block register retail uses instead of one merged pseudo. */
extern int D_0015EFB0_m __asm__("D_0015EFB0") MACRO_ADDR;
extern int D_0013CBE4;
extern char D_001D5F70[] NOT_SDA;

int func_00222B00(void) {
    if (D_0015EFB0_m != 0x10 && D_0015EFB0_m != 1) {
        char *g = D_001D5F70;
        int val = *(int *)(*(char **)(g + 4) + 0x38);
        *(int *)(g + 8) = val;
        return 0;
    }
    {
        int v = D_0013CBE4;
        if (v & 0x20) {
            char *g = D_001D5F70;
            *(int *)(g + 0xD4) = 0;
            *(int *)(g + 8) = *(int *)(*(char **)(g + 4) + 0x38);
            *(int *)(*(char **)(g + 4) + 0x84) = 1;
        } else if (v & 0x10) {
            char *g = D_001D5F70;
            *(int *)(g + 8) = *(int *)(*(char **)(g + 4) + 0x38);
            *(int *)(*(char **)(g + 4) + 0x84) = 0;
        }
    }
    return 0;
}

typedef struct {
    short s[12];
} TextBox;

extern char D_001603E8[];
extern int D_001D6044;
extern void func_00234C98_l(int, long) __asm__("func_00234C98");
extern void func_001F4630(int);
extern void func_001F4748(void);
extern void *func_001FE540_id(int) __asm__("func_001FE540");
extern void func_001F75D0(TextBox *, long, char *, int);

/* Draws the two-line prompt box (text 0x4FB3 for D_001D6044 in 0..2,
   0x4FB5 for 3, else D_001603E8) sized from arg0's +0x20/+0x24. The box
   is an aggregate initializer: this compiler clears it with a memset
   libcall, fills a temporary and copies that into the local with
   ldl/ldr/sdl/sdr pairs, exactly retail's sequence. */
int func_00222B98(char *arg0) {
    char *text;
    int v;

    func_00234C98_l(0x42, 0x44);
    func_00234C98_l(0x47, 0x2004B);
    SetupGifPaging(0);
    text = D_001603E8;
    v = D_001D6044;
    switch (v) {
    case 0:
    case 1:
    case 2:
        text = func_001FE540_id(0x4FB3);
        break;
    case 3:
        text = func_001FE540_id(0x4FB5);
        break;
    }
    {
        TextBox c = { { 1, *(int *)(arg0 + 0x24) + 1, 1, *(int *)(arg0 + 0x20) + 1,
                        *(int *)(arg0 + 0x20) >> 1, 5, 0, 0, 0x10, 5 } };

        func_001F75D0(&c, 0x80000000L, text, -1);
        c.s[5] = (*(int *)(arg0 + 0x24) - c.s[7]) >> 1;
        c.s[9] ^= 4;
        func_001F75D0(&c, 0x80000000L, text, -1);
        c.s[0]--;
        c.s[1]--;
        c.s[2]--;
        c.s[3]--;
        c.s[4]--;
        c.s[5]--;
        func_001F75D0(&c, 0x80FFA888L, text, -1);
    }
    DoGifPaging();
    return 2;
}

extern int D_001D48A8[];

/* `D_001D48A8[(unsigned)D_0015EE84 % 19]`; the older near-miss
   predated MACRO_ADDR. */
int func_00222D70(char *arg0) {
    *(int *)(arg0 + 0x34) = D_001D48A8[(unsigned int)D_0015EE84 % 19];
    return 0;
}

extern void func_0022ED80(int, int, int);

/* Pause sub-menu with a 30-entry wrapping cursor at arg0+0x40, in
   func_0021F7D0's shape. */
int func_00222DB0(char *arg0) {
    char *pad = D_0013CA40;
    int w, old;

    if (*(int *)(pad + 0x1C4) & 0xD00) {
        if (*(int *)(D_001D5F70 + 0x124) == 0) {
            return 1;
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x10) {
            char *g = D_001D5F70;
            int t = *(int *)(*(char **)(g + 4) + 0x38);
            if (t != 0) {
                *(int *)(g + 8) = t;
                return 0;
            }
            if (*(int *)(g + 0x124) == 0) {
                return -1;
            }
        }
    }
    {
        char *pad3 = D_0013CA40;
        w = *(int *)(pad3 + 0x1A4);
    }
    old = *(int *)(arg0 + 0x40);
    if (w & 0x40) {
        *(int *)(arg0 + 0x40) = (old + 1) % 30;
    } else if (w & 0x20) {
        *(int *)(arg0 + 0x40) = (old + 29) % 30;
    }
    if (*(int *)(arg0 + 0x40) != old) {
        func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
    }
    return 0;
}

extern void func_0022ED80(int, int, int);

/* func_00222DB0's twin with a 12-entry cursor at arg0+0x54. */
int func_00222E98(char *arg0) {
    char *pad = D_0013CA40;
    int w;

    if (*(int *)(pad + 0x1C4) & 0xD00) {
        if (*(int *)(D_001D5F70 + 0x124) == 0) {
            return 1;
        }
    }
    {
        char *pad2 = D_0013CA40;
        if (*(int *)(pad2 + 0x1C4) & 0x10) {
            char *g = D_001D5F70;
            int t = *(int *)(*(char **)(g + 4) + 0x38);
            if (t != 0) {
                *(int *)(g + 8) = t;
                return 0;
            }
            if (*(int *)(g + 0x124) == 0) {
                return -1;
            }
        }
    }
    {
        char *pad3 = D_0013CA40;
        w = *(int *)(pad3 + 0x1A4);
    }
    if (w & 0x2040) {
        *(int *)(arg0 + 0x54) = (*(int *)(arg0 + 0x54) + 1) % 12;
        func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
    } else if (w & 0x8020) {
        *(int *)(arg0 + 0x54) = (*(int *)(arg0 + 0x54) + 11) % 12;
        func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
    }
    return 0;
}

typedef struct { char c[2]; } Glyph2;
extern char D_001603F0[];
extern char D_001603F8[];
extern int func_00200248(int);
/* x, y first: arguments are evaluated in order, so the y conversion runs
   before the nested texture calls and is kept in $f20 across them, as in
   retail (and in func_00205E70, the other caller). */
extern void func_00200E38_f(float, float, int, int, int, float, float, float)
    __asm__("func_00200E38");

/* Draws the glyph "\x10" (arg0->0x38 set) or "\x11" and a gauge sprite
   below it, rotated by pi in the second case. The glyph string is a
   2-byte char struct copied onto the stack (retail's lb/lb/sb/sb). */
int func_00222FA8(char *arg0) {
    Glyph2 buf;

    SetupGifPaging(0);
    if (*(int *)(arg0 + 0x38) != 0) {
        buf = *(Glyph2 *)D_001603F0;
        func_001F68E8_c(4, *(int *)(arg0 + 0x24) / 2 - 8, 0x80FFA888L, &buf, -1);
        func_00200E38_f(640.0f, (float)(*(int *)(arg0 + 0x24) << 3), 0x20, 0x10,
                        GetFrameTex(GetIconFrame(0xE99E, 6)), 128.0f,
                        256.0f, 0.0f);
    } else {
        buf = *(Glyph2 *)D_001603F8;
        func_001F68E8_c(*(int *)(arg0 + 0x20) - 0x18,
                        *(int *)(arg0 + 0x24) / 2 - 8, 0x80FFA888L, &buf, -1);
        func_00200E38_f(192.0f, (float)(*(int *)(arg0 + 0x24) << 3), 0x20, 0x10,
                        GetFrameTex(GetIconFrame(0xE99E, 6)), 128.0f,
                        256.0f, 3.14159274f);
    }
    DoGifPaging();
    return 2;
}

/* D_001DE0C0 is declared above as Src0C (halfword view); these rows are
   read here as two word-sized icon ids. */
typedef struct { int a; int b; int c; } IconRow;
extern IconRow D_001DE0C0_i[] __asm__("D_001DE0C0");
extern void func_001F6EA8(int, int, long, void *, int);

/* Draws the current page's icons: the page record (stride 0xC) gives a
   row of D_001DE0C0, or -1 for the single icon 0x5019 centred in the
   box; otherwise the row's two icons at 1/3 and 2/3 of the height. The
   row is read before func_001F4630, and each call is written with its
   arguments inline: gcc evaluates them in order, so x and y are computed
   before the nested func_001FE540 call. */
int func_00223140(char *arg0) {
    char *p = *(char **)(D_001D5F74 + 0x40);
    int row = *(int *)(*(char **)(p + 0x34) + *(int *)(p + 0x40) * 12 + 4);

    SetupGifPaging(0);
    if (row == -1) {
        FontPrintCenter(*(int *)(arg0 + 0x20) / 2, *(int *)(arg0 + 0x24) / 2 - 8,
                      0x80FFA888L, func_001FE540_id(0x5019), -1);
    } else {
        FontPrintCenter(*(int *)(arg0 + 0x20) / 2, *(int *)(arg0 + 0x24) / 3 - 8,
                      0x80FFA888L, func_001FE540_id(D_001DE0C0_i[row].a), -1);
        FontPrintCenter(*(int *)(arg0 + 0x20) / 2, *(int *)(arg0 + 0x24) * 2 / 3 - 8,
                      0x80FFA888L, func_001FE540_id(D_001DE0C0_i[row].b), -1);
    }
    DoGifPaging();
    return 2;
}

extern int D_0015EFB4_m __asm__("D_0015EFB4") MACRO_ADDR;
extern int D_0015EFA0 MACRO_ADDR;
extern int D_0015F6CC MACRO_ADDR;
extern char D_001D5240[];
extern void func_001FBC80(int, void *, int);
extern unsigned char D_0014C008[];
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern unsigned char D_0015EEC0[] MACRO_ADDR;
extern void func_001F9A00(void *, void *, int);
extern void func_00209CE8(int);
extern void func_0020BFC8(int, int);
extern int func_001F98C0(int);
extern void func_001F4E08(int);
extern unsigned char D_0013F450[];
extern void func_00228268(void);

/* Pad handler for the restart option (pad word D_0013CBE4): 0x20 opens
   the confirm popup; 0x40 restarts: the level reload (func_00209CE8)
   runs with the D_0013D510+0x1D byte, the 4 bytes at D_0014C008 and the
   D_0015EEB0/D_0015EEC0 tables saved and restored around it, then the
   pause state is reset. The flags are MACRO_ADDR (retail's one-register
   loads, $at stores and $gp-relative stores in delay slots); globals
   used as bases get a `char *` local per block, so retail keeps only the
   D_0013D510 %hi across the calls. */
int func_002232E0(void) {
    int v = D_0013CBE4;

    if (v & 0x20) {
        D_0015EFB4_m |= 2;
        func_001FBC80(3, D_001D5240, 0);
        return 0;
    }
    if (v & 0x40) {
        unsigned char save[4];
        char tbl0[0xC];
        char tbl1[0xC];
        unsigned char tag;
        int i;

        {
            unsigned char *m = gSkillPoints;
            tag = m[0x1D];
        }
        for (i = 0; i < 4; i++) {
            save[i] = D_0014C008[i];
        }
        func_001F9A00(tbl0, gCheats, 0xC);
        func_001F9A00(tbl1, D_0015EEC0, 0xC);
        {
            char *g = D_001D5F70;
            memcard_RestoreGame(*(int *)(g + 0xE0));
        }
        {
            unsigned char *m2 = gSkillPoints;
            m2[0x1D] = tag;
        }
        for (i = 0; i < 4; i++) {
            D_0014C008[i] = save[i];
        }
        func_001F9A00(gCheats, tbl0, 0xC);
        func_001F9A00(D_0015EEC0, tbl1, 0xC);
        D_0015EFA0 = 1;
        D_0015F6CC = 0;
        memcard_Save(0, -1);
        func_00228268();
        FadeToBlack(func_001F98C0(0x10));
        D_0013F450[0x20B1] = 1;
        return -1;
    }
    return 0;
}

int func_00223478(void *arg0) {
    *(int *)((char *)arg0 + 0x40) = 0;
    *(int *)((char *)arg0 + 0x50) = 0;
    *(int *)((char *)arg0 + 0x3C) = 0;
    return 0;
}

extern int D_001D2E74;
extern char D_001D5418[];
extern char D_001D54C8[];
extern char D_001D5588[];
extern char D_001D5618[];
extern char D_001D5630[];

/* A timed text sequence (credits-style): arg0+0x50 is the step, +0x40
   a countdown (func_001F98C0-scaled), +0x3C a scroll that advances 10 a
   frame (20 while all four shoulder buttons are held, which also halves
   the countdown), +0x34 the current text (a D_001D5xxx page or a text
   id) with D_001D2E74 its title id. Case bodies are in retail's layout
   order (4 before 3), and the shared "wait, then next step" tail is
   written out in each case: cross-jumping merges it, leaving each
   case's own argument load in its branch's delay slot. */
int func_00223490(char *arg0) {
    char *pad = D_0013CA40;
    char *g = D_001D5F70;
    int fast = (*(long *)(pad + 0x1A0) & 0xF) == 0xF;
    int x;

    if (*(int *)(g + 0xDC) == 0 || (fast && (*(int *)(pad + 0x1A0) & 0x10))) {
        if (*(int *)(pad + 0x1C4) & 0xD00) {
            if (*(int *)(g + 0x124) == 0) {
                return 1;
            }
        }
        {
            char *pad2 = D_0013CA40;
            if (*(int *)(pad2 + 0x1C4) & 0x10) {
                char *g2 = D_001D5F70;
                int t = *(int *)(*(char **)(g2 + 4) + 0x38);
                if (t != 0) {
                    *(int *)(g2 + 8) = t;
                    return 0;
                }
                if (*(int *)(g2 + 0x124) == 0) {
                    return -1;
                }
            }
        }
    }
    x = *(int *)(arg0 + 0x40);
    *(int *)(arg0 + 0x40) = x - 1;
    if (fast) {
        *(int *)(arg0 + 0x40) = x - 2;
    }
    if (*(int *)(arg0 + 0x40) < 0) {
        *(int *)(arg0 + 0x40) = 0;
    }
    switch (*(int *)(arg0 + 0x50)) {
    case 0:
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        *(int *)(arg0 + 0x40) = func_001F98C0(0xB4);
        *(int *)(arg0 + 0x3C) = 0;
        D_001D2E74 = 0x50A9;
        *(char **)(arg0 + 0x34) = D_001D5418;
        break;
    case 1:
    case 5:
    case 8:
    case 12:
    case 16:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        *(int *)(arg0 + 0x54) = 0;
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        break;
    case 2:
    case 6:
    case 9:
    case 13:
    case 17:
        x = *(int *)(arg0 + 0x3C);
        *(int *)(arg0 + 0x3C) = x + 10;
        if (fast) {
            *(int *)(arg0 + 0x3C) = x + 20;
        }
        if (*(int *)(arg0 + 0x54) == 0) {
            return 0;
        }
        *(int *)(arg0 + 0x40) = func_001F98C0(0xB4);
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        break;
    case 4:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        *(int *)(arg0 + 0x50) = 5;
        *(int *)(arg0 + 0x40) = func_001F98C0(0xB4);
        *(int *)(arg0 + 0x3C) = 0;
        D_001D2E74 = 0x50D6;
        *(char **)(arg0 + 0x34) = D_001D54C8;
        break;
    case 3:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        D_001D2E74 = 0x50D4;
        *(int *)(arg0 + 0x34) = 0x50D5;
        *(int *)(arg0 + 0x40) = func_001F98C0(0xF0);
        *(int *)(arg0 + 0x3C) = 0;
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        break;
    case 7:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        D_001D2E74 = 0x510B;
        *(char **)(arg0 + 0x34) = D_001D5588;
        *(int *)(arg0 + 0x40) = func_001F98C0(0xF0);
        *(int *)(arg0 + 0x3C) = 0;
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        break;
    case 10:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        D_001D2E74 = 0x513B;
        *(int *)(arg0 + 0x34) = 0x513C;
        *(int *)(arg0 + 0x40) = func_001F98C0(0xF0);
        *(int *)(arg0 + 0x3C) = 0;
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        break;
    case 11:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        *(int *)(arg0 + 0x40) = func_001F98C0(0xB4);
        *(int *)(arg0 + 0x3C) = 0;
        D_001D2E74 = 0x513D;
        *(char **)(arg0 + 0x34) = D_001D5618;
        *(int *)(arg0 + 0x50) = 12;
        break;
    case 14:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        D_001D2E74 = 0;
        *(int *)(arg0 + 0x34) = 0x5148;
        *(int *)(arg0 + 0x40) = func_001F98C0(0xF0);
        *(int *)(arg0 + 0x3C) = 0;
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        break;
    case 15:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        *(int *)(arg0 + 0x40) = func_001F98C0(0xB4);
        *(int *)(arg0 + 0x3C) = 0;
        D_001D2E74 = 0x5149;
        *(char **)(arg0 + 0x34) = D_001D5630;
        *(int *)(arg0 + 0x50) = 16;
        break;
    case 18:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        D_001D2E74 = 0;
        *(int *)(arg0 + 0x34) = 0x517A;
        *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
        *(int *)(arg0 + 0x40) = func_001F98C0(300);
        break;
    case 19:
        if (*(int *)(arg0 + 0x40) != 0) {
            return 0;
        }
        {
            char *g3 = D_001D5F70;
            if (*(int *)(g3 + 0xDC) != 0) {
                return 1;
            }
        }
        break;
    }
    return 0;
}

/* A text box on the menu's own geometry (func_00227A30's box, then
   arg0+0x18..0x24: top y + 4, bottom y + h - 4, x, x + w, centre).
   Per state (arg0+0x50): the list states draw each text id of the -1
   terminated list at arg0+0x34 down from a start that the line count
   (arg0+0x3C >> 4) centres, taking each height from box[7] as the text
   call leaves it, and flag arg0+0x54 when the list ends 0x18 above the
   bottom; states 4, 11, 15 and 19 draw the single text id at +0x34. The
   line count's low nibble goes through `& 0xF` (retail's lbu + dsrl) and
   the start subtracts its own variable, which keeps retail's order. */
int func_00223810(char *arg0) {
    short box[12];
    int ypos;
    int n;
    int i;

    func_00234C98_l(0x47, 0x30000);
    func_00234C98_l(0x42, 0x8000000044L);
    SetupGifPaging(0);
    func_00227A30(box, arg0);
    box[9] = 9;
    box[3] = *(int *)(arg0 + 0x18) + *(int *)(arg0 + 0x20);
    box[0] = *(int *)(arg0 + 0x1C) + 4;
    box[1] = *(int *)(arg0 + 0x1C) + *(int *)(arg0 + 0x24) - 4;
    box[2] = *(int *)(arg0 + 0x18);
    box[4] = *(int *)(arg0 + 0x18) + (*(int *)(arg0 + 0x20) >> 1);
    box[10] = 0;
    box[11] = 0;
    switch (*(int *)(arg0 + 0x50)) {
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 12:
    case 13:
    case 14:
    case 16:
    case 17:
    case 18:
        n = (*(int *)(arg0 + 0x3C) >> 4) - 4;
        ypos = *(int *)(arg0 + 0x1C) - n;
        for (i = 0; (*(int **)(arg0 + 0x34))[i] != -1; i++) {
            box[5] = ypos;
            box[11] = (*(int *)(arg0 + 0x3C) >> 4) & 0xF;
            func_001F7560(box, 0x80FFA888L,
                          func_001FE540_id((*(int **)(arg0 + 0x34))[i]), -1);
            ypos += box[7];
            ypos += 10;
        }
        if (ypos + 0x18 < *(int *)(arg0 + 0x1C) + *(int *)(arg0 + 0x24)) {
            *(int *)(arg0 + 0x54) = 1;
        }
        break;
    case 4:
    case 11:
    case 15:
    case 19:
        box[5] = *(int *)(arg0 + 0x1C) + 0x20;
        func_001F7560(box, 0x80FFA888L, func_001FE540_id(*(int *)(arg0 + 0x34)), -1);
        break;
    case 0:
        break;
    }
    DoGifPaging();
    return 2;
}

extern void func_001153FC(void *, int, int); /* memset */
typedef struct {
    short f0, f2, f4, f6;
    short x, y;
    short fC;
    unsigned short fE;
    short f10, f12, f14, f16;
} Box18;

/* ObtainAllGoldWeaponsMenu: two icon + text rows (texts 0x5187, 0x5188)
   in a box sized from arg0+0x20/0x24. The box is built zeroed in a
   temporary and copied (a struct assignment: retail's ldl/ldr copy). y is
   one variable, 4 and then the bottom of the first text (buf.fE, written
   by func_001F7560) plus 0x10, with each icon at y + 8; it and x stay in
   callee-saved registers across the calls, as in retail. */
int func_002239F0(char *arg0) {
    Box18 buf;
    Box18 tmp;
    int x, y;

    func_001153FC(&tmp, 0, 0x18);
    tmp.f2 = *(unsigned short *)(arg0 + 0x24);
    tmp.f6 = *(unsigned short *)(arg0 + 0x20);
    tmp.f10 = 0x10;
    buf = tmp;
    x = 0x18;
    y = 4;
    SetupGifPaging(0);
    HudSprite(GetIconFrame(0xE99A, 6), 4, y + 8, 0x10, 0x10, 0x80);
    buf.x = x;
    buf.y = y;
    func_001F7560(&buf, 0x80FFA888L, func_001FE540_id(0x5187), -1);
    y = (short)buf.fE + 0x10;
    buf.y = y;
    HudSprite(GetIconFrame(0xE99A, 6), 4, y + 8, 0x10, 0x10, 0x80);
    func_001F7560(&buf, 0x80FFA888L, func_001FE540_id(0x5188), -1);
    DoGifPaging();
    return 2;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00223B40); /* DrawEndScreenMenuMaybe */

/* Page records in D_0013D390: stride 0x1C from +0x20. */
typedef struct { int row; char pad[0x18]; } PageRec;
typedef struct { char pad[0x20]; PageRec rec[1]; } PageTbl;

/* func_00223140's sibling for the D_0013D390 pages: if the page passes
   four gates, draws icon 0x521C centred (row -1) or the row's two
   D_001DE0C0 icons at y 4 and 0x14. The row is read before
   func_001F4630; the record is reached as a struct member off the page
   base (retail's base-first add). */
int func_00223E40(char *arg0) {
    char *g = D_001D5F70;
    char *b = D_0013D390;
    int row = ((PageTbl *)b)->rec[*(int *)(*(char **)(*(char **)(g + 4) + 0x40) + 0x40)].row;

    SetupGifPaging(0);
    if (*(int *)(b + 0xDC) < 3 && *(int *)(b + 0xE4) < 0
        && *(int *)(g + 0x154) >= 0xB && *(int *)(b + 8) == 2) {
        if (row == -1) {
            FontPrintCenter(*(int *)(arg0 + 0x20) / 2, *(int *)(arg0 + 0x24) / 2 - 8,
                          0x80FFA888L, func_001FE540_id(0x521C), -1);
        } else {
            FontPrintCenter(*(int *)(arg0 + 0x20) / 2, 4, 0x80FFA888L,
                          func_001FE540_id(D_001DE0C0_i[row].a), -1);
            FontPrintCenter(*(int *)(arg0 + 0x20) / 2, 0x14, 0x80FFA888L,
                          func_001FE540_id(D_001DE0C0_i[row].b), -1);
        }
    }
    DoGifPaging();
    return 2;
}

extern void func_00226D50(int);

int func_00223FD0(void *arg0) {
    func_00226D50(1);
    *(int *)((char *)arg0 + 0x48) = func_00226EA8(0);
    *(int *)((char *)arg0 + 0x4C) = 0;
    return 0;
}

int func_00224010(void *arg0) {
    *(int *)((char *)arg0 + 0x48) = func_00226F68(*(int *)((char *)arg0 + 0x48));
    return 0;
}

extern int D_0015EFB4_mm __asm__("D_0015EFB4") MACRO_ADDR;
extern int D_0015EF34 MACRO_ADDR;
extern int D_0015FF4C MACRO_ADDR;
extern int D_0015EE98 MACRO_ADDR;
extern int D_0015EF20 MACRO_ADDR;
extern int D_0015EF24 MACRO_ADDR;
extern int D_0015EE84_mm __asm__("D_0015EE84") MACRO_ADDR;
/* The 8-byte D_0015EF98 pair saved into a slot. As a char block in small
   data, its copy is la + ldl/ldr, as in retail. */
typedef struct {
    char b[8];
} SaveWord2;
extern SaveWord2 D_0015EF98_s __asm__("D_0015EF98") MACRO_ADDR;
extern char D_001D28F8[];
extern int D_001D29C0;
extern void func_00227C78(int, int);
extern int D_0015EFB0_mm __asm__("D_0015EFB0") MACRO_ADDR;

/* Save-slot menu handler (func_002243E8's sibling): on entry it may arm
   a save (0x4FB5 prompt), then while the prompt is up it either aborts
   (flag 0x80 in D_0015EFB4) or writes the current game into slot
   d+0x14 of D_0013D390's 0x1C-byte records; otherwise the usual pad
   handling, with slot selection on 0x1000/0x4000 and 0x40 either
   loading the slot (via D_001D28F8) or re-arming. Each branch reaches
   the globals through its own block-scoped pointers. */
int func_00224040(char *arg0) {
    char *g = D_001D5F70;
    int s;
    int old;

    if (*(char **)(*(char **)(g + 4) + 0x40) != arg0) {
        return 0;
    }
    s = *(int *)(arg0 + 0x4C);
    if (s == 0) {
        char *m = *(char **)(g + 0xD0);
        if (m == D_001D28F8) {
            if (*(int *)(m + 0x84) != 0) {
                *(int *)(arg0 + 0x4C) = 1;
            }
        }
        s = *(int *)(arg0 + 0x4C);
    }
    if (s == 1) {
        char *g2;

        func_00227C78(*(int *)(arg0 + 0x48), *(int *)(arg0 + 0x40));
        g2 = D_001D5F70;
        *(int *)(g2 + 0x12C) = 0x4FB5;
        *(int *)(g2 + 0x128) = 1;
        D_0015FF4C = 0;
    }
    *(int *)(arg0 + 0x4C) = 2;
    old = *(int *)(arg0 + 0x40);
    {
        char *g3 = D_001D5F70;

        if (*(int *)(g3 + 0x128) != 0) {
            char *d = D_0013D390;
            char *e;

            if (*(int *)(d + 0xDC) >= 3) {
                return 0;
            }
            if (*(int *)(d + 0xE4) >= 0) {
                return 0;
            }
            if (*(int *)(g3 + 0x154) < 11) {
                return 0;
            }
            *(int *)(g3 + 0x128) = 0;
            if (*(int *)(d + 0xEC) != 0 || *(int *)(d + 0x1C) != 0 || D_0015FF4C != 0) {
                char *d2 = D_0013D390;
                char *g4 = D_001D5F70;

                D_0015EFB4_mm |= 0x80;
                *(int *)(d2 + 0xFC) = 0;
                func_001FBC80(3, *(void **)(g4 + 4), 0);
                return 0;
            }
            e = d + 0x30;
            *(int *)(d + 0xFC) = 1;
            *(int *)(d + 0x24 + *(int *)(d + 0x14) * 0x1C) = gBolts;
            *(int *)(d + 0x20 + *(int *)(d + 0x14) * 0x1C) = D_0015EE84_mm;
            *(int *)(d + 0x2C + *(int *)(d + 0x14) * 0x1C) = D_0015EF24;
            *(SaveWord2 *)(e + *(int *)(d + 0x14) * 0x1C) = D_0015EF98_s;
            *(int *)(d + 0x28 + *(int *)(d + 0x14) * 0x1C) = D_0015EF20;
        }
    }
    {
        char *pad = D_0013CA40;
        if (*(int *)(pad + 0x1C4) & 0xD00) {
            char *g2 = D_001D5F70;
            if (*(int *)(g2 + 0x124) == 0) {
                return 1;
            }
        }
    }
    {
        char *pad = D_0013CA40;
        if (*(int *)(pad + 0x1C4) & 0x10) {
            char *g2 = D_001D5F70;
            int t = *(int *)(*(char **)(g2 + 4) + 0x38);
            if (t != 0) {
                *(int *)(g2 + 8) = t;
                return 0;
            }
            if (*(int *)(g2 + 0x124) == 0) {
                return -1;
            }
        }
    }
    if (D_0015EFB0_mm != 0x10 && D_0015EFB0_mm != 1) {
        char *g2 = D_001D5F70;
        *(int *)(g2 + 8) = *(int *)(*(char **)(g2 + 4) + 0x38);
        return 0;
    }
    {
        char *d = D_0013D390;
        int p;
        int x;

        if (*(int *)(d + 0xDC) >= 3) {
            return 0;
        }
        if (*(int *)(d + 0xE4) >= 0) {
            return 0;
        }
        {
            char *g2 = D_001D5F70;
            if (*(int *)(g2 + 0x154) < 11) {
                return 0;
            }
        }
        if (*(int *)(d + 8) != 2) {
            return 0;
        }
        if (*(int *)(arg0 + 0x30) & 1) {
            char *pad = D_0013CA40;
            p = *(int *)(pad + 0x1B4);
        } else {
            char *pad = D_0013CA40;
            p = *(int *)(pad + 0x1A4);
        }
        x = D_0015EF34;
        *(int *)(arg0 + 0x40) = x;
        if ((p & 0x1000) && x != 0) {
            *(int *)(arg0 + 0x40) = x - 1;
        }
        if ((p & 0x4000) && *(int *)(arg0 + 0x40) < 4) {
            *(int *)(arg0 + 0x40) = *(int *)(arg0 + 0x40) + 1;
        }
        D_0015EF34 = *(int *)(arg0 + 0x40);
        if (p & 0x40) {
            char *d2 = D_0013D390;

            if (*(int *)(d2 + 8) == 2) {
                if (*(int *)(d2 + 0x20 + D_0015EF34 * 0x1C) != -1) {
                    char *g2 = D_001D5F70;

                    *(int *)(g2 + 0xD4) = 0;
                    *(char **)(g2 + 8) = D_001D28F8;
                    D_001D29C0 = *(int *)(arg0 + 0x40);
                } else {
                    *(int *)(arg0 + 0x4C) = 1;
                }
            }
        }
    }
    if (*(int *)(arg0 + 0x40) != old) {
        func_0022ED80(1, 0x11, *(int *)(arg0 + 0x14));
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_002243E8);

INCLUDE_ASM("asm/nonmatchings/text", func_00224728);

INCLUDE_ASM("asm/nonmatchings/text", func_00224C30);

struct MenuFlashingPanel {
    u8 pad0[0x48];
    s32 active;
    s32 time;
    s32 x;
    s32 y;
    s32 width;
    s32 height;
};
struct MenuPanelOwner {
    u8 pad0[0x78];
    struct MenuFlashingPanel *flash;
};
extern short D_0015EE80_250B8 __asm__("D_0015EE80");
extern void func_00234C98_250B8(s32, u64) __asm__("func_00234C98");
extern s32 func_002140B0(s32);
extern s64 func_001F4868_250B8(s32) __asm__("func_001F4868");
extern void func_001F5800_250B8(s32, s32, s32, s32, s32, s32, s32, s32, s64, s64) __asm__("func_001F5800");
void func_002250B8(struct MenuPanelOwner *owner);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/ui/menus/fun_00223e28.c, draw_menu_flashing_panel. */
void func_002250B8(struct MenuPanelOwner *owner) {
    struct MenuFlashingPanel *panel;
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    s32 u;
    s32 v;
    s32 alpha;
    s32 width_adjustment;
    s32 height_adjustment;

    if (owner == 0) {
        return;
    }
    panel = owner->flash;
    if (panel == 0) {
        return;
    }
    x = panel->x;
    y = panel->y;
    width = panel->width;
    height = panel->height;
    if (x >= 0x200 || x + width < 0) {
        return;
    }
    if (!((*(s32 *)&D_0015EE80_250B8) != 0 ? y < 0x1C1 : y < 0x1A1) || y + height < 0) {
        return;
    }
    func_00234C98_250B8(8, 0);
    func_00234C98_250B8(0x42, 0x8000000044ULL);
    func_002008B8(func_00200198(0xE99E, 7), x << 4, y << 4, width << 4, height << 4, 0x80);
    if (panel->active) {
        panel->time += 2;
        u = func_002140B0(200);
        v = func_002140B0(200);
        alpha = 0x80 - func_001F9B70(panel->time - 0x80);
        func_00234C98_250B8(8, 0);
        alpha = alpha * 2;
        func_00234C98_250B8(0x42, ((u64)(alpha > 0x80 ? 0x80 : alpha) << 32) | 0x68);
        func_001F5800_250B8(x, y, width, height, u, v, width, height, 0x808080, func_001F4868_250B8(0x1A));
        if (panel->time >= 0x100) {
            panel->active = 0;
        }
    } else if (func_002140B0(2000) == 0) {
        panel->time = 0;
        panel->active = 1;
    }
    func_00234C98_250B8(8, 0);
    func_00234C98_250B8(0x42, 0x8000000044ULL);
    width_adjustment = -2;
    height_adjustment = -2;
    func_001F5800_250B8(x, y, width, height, 0, 0, width, (height * 3) >> 1, 0x50606060, func_001F4868_250B8(0x1C));
    if (width >= 0x4C) {
        width_adjustment = -1;
    }
    if (height >= 0x4C) {
        height_adjustment = -1;
    }
    if (width >= 0x97) {
        width_adjustment = 0;
    }
    if (height >= 0x97) {
        height_adjustment = 0;
    }
    func_001F5800_250B8(x + 1, y + 1, width + width_adjustment, height + height_adjustment, 1, 1, 0x3E, 0x3E, 0x80808080, func_001F4868_250B8(0x19));
}

extern int func_002279D0(void);
extern int D_0016004C MACRO_ADDR;
extern char D_001D6160[];
extern char D_001D61A0[];
extern char D_001D61E0[];
extern char D_00186410[];
extern void func_001E9790(void *);
extern void func_00225DF0(void);
extern void func_00226250(void *);

/* Menu setup: resets the pad bindings, marks D_001D5F70's selections
   unset, fetches three entries with func_00226EA8, clears the 24 flag
   bytes at arg0+0xA4, then spawns moby 0 in front of the camera (update
   func_00225DF0, owner arg0) and moby 0x259 (update func_00226250). One
   `g` local per block reading D_001D5F70 gives retail's %hi kept in a
   saved register with %lo rebuilt; the -1 stores are written in the
   order that gives retail's schedule. */
int func_00225358(char *arg0) {
    char *m;
    int *p;
    int *v;
    char *q;
    int i, j;

    func_00226D50(1);
    func_002279D0();
    {
        unsigned char *g = D_001D5F70;

        D_0016004C = -1;
        *(int *)(g + 0x11C) = -1;
        *(int *)(g + 0x120) = -1;
        *(int *)(g + 0xA0) = func_00226EA8(1);
        *(int *)(g + 0xA4) = func_00226EA8(1);
        g[0xC8] = 0xFF;
        g[0xC9] = 0xFF;
        g[0xCA] = 0;
        p = (int *)(g + 0xB0);
        for (i = 2; i >= 0; i--) {
            *p = func_00226EA8(0);
            p++;
        }
    }
    {
        char *g = D_001D5F70;

        D_001D6160[1] = 0;
        *(int *)(g + 0x1C) = -1;
        D_001D61A0[1] = 0;
        D_001D61E0[1] = 0;
    }
    m = func_00226720_a(0);
    q = arg0 + 0xBB;
    for (j = 23; j >= 0; j--) {
        *q = 0;
        q--;
    }
    if (m != 0) {
        char *cam = D_00187040;
        char *g;

        *(char **)(arg0 + 0x44) = m;
        *(short *)(m + 0x34) = 0;
        *(float *)(m + 0x10) = *(float *)(cam + 0x140) + 4.0f;
        *(float *)(m + 0x14) = *(float *)(cam + 0x144);
        *(float *)(m + 0x18) = *(float *)(cam + 0x148) - 0.6f;
        *(float *)(m + 0x48) = 3.1415927f;
        *(void **)(m + 0x74) = func_00225DF0;
        v = *(int **)(m + 0x78);
        v[0] = (int)arg0;
        v[1] = 0;
        v[2] = 0;
        g = D_001D5F70;
        *(int *)(g + 0xC0) = -1;
        FastMemSet(D_00186410, 0, 0x40);
        func_001E9790(m);
    }
    m = func_00226720_a(0x259);
    if (m != 0) {
        **(int **)(m + 0x78) = (int)arg0;
        *(void **)(m + 0x74) = func_00226250;
        *(short *)(m + 0x34) = 4;
    }
    *(char **)(arg0 + 0x48) = m;
    return 0;
}
__asm__(".section .text\n\tnop\n");

/*
 * Exact once the short-loop padding retail's assembler adds is reproduced
 * (tools/fix_short_loops.py): both loops are five instructions and get one
 * nop each. g is unsigned char so 0xFF is built as 0xFF, not -1. Two
 * separate loop counters are load-bearing: with one variable reused, both
 * loops take $s2 and q takes $s1.
 */
extern int func_002267C0(int);
extern int func_00226F68(int);
extern void func_00227A70(void);

int func_00225548(void *arg0) {
    unsigned char *g; int *p; int *q; int i; int j;

    p = (int *)((char *)arg0 + 0x44);
    for (i = 23; i >= 0; i--) { *p = func_002267C0(*p); p++; }
    g = D_001D5F70;
    *(int *)(g + 0xA0) = func_00226F68(*(int *)(g + 0xA0));
    *(int *)(g + 0xA4) = func_00226F68(*(int *)(g + 0xA4));
    g[0xC8] = 0xFF;      // emitted 0xC9 then 0xC8: two stores of the
    g[0xC9] = 0xFF;      // same value come out in the opposite order
    g[0xCA] = 0;
    func_00227A70();
    q = (int *)(g + 0xB0);
    for (j = 2; j >= 0; j--) { *q = func_00226F68(*q); q++; }
    return 0;
}

typedef struct Moby {
    u8 pad00[0x20];
    u8 state;
    u8 pad21[0x13];
    s16 update_kind;
    u8 pad36[0x1D];
    u8 anim;
    u8 pad54[0x20];
    void *update;
    void **vars;
    u8 pad7C[0x2A];
    s16 oclass;
    u8 padA8[0x14];
    u8 ammo_moby_slot;
} Moby;
typedef struct HandGadgetState {
    u8 pad00[0x44];
    s32 source_moby_address;
    u8 pad48[4];
    Moby *pose_moby;
    Moby *animation_moby;
    Moby *class_pose_moby;
    Moby *first_attachment_moby;
    Moby *second_attachment_moby;
    s32 x60;
    s32 x64;
    s32 x68;
    Moby *class_0197_moby;
    Moby *class_0266_moby;
    Moby *class_026a_moby;
    Moby *ammo_mobys[8];
    u8 pad98[0xC];
    u8 timers[0x18];
} HandGadgetState;
typedef struct HandGadgetDefinition {
    u8 pad00[0x10];
    s32 oclass;
    u8 pad14[0x38];
} HandGadgetDefinition;
typedef struct HandGadgetSelection {
    u8 pad00[0x1C];
    s32 current_gadget;
    u8 pad20[0x10];
    s32 selected_gadget;
    s32 attachment_gadget;
    s32 animation_gadget;
    s32 pose_gadget;
    u8 pad40[0x8C];
    s32 animation_base;
    u8 padD0[0x48];
    s32 D_00160050;
    s32 active_resource_class;
    s32 requested_resource_class;
    u8 pad124[0x1C];
    s32 last_requested_resource_class;
    s32 last_resource_request_state;
} HandGadgetSelection;
typedef struct HandGadgetManipulator {
    u8 pad0;
    u8 active;
    u8 pad2[0x1E];
    float rotation_x;
    float rotation_y;
    float rotation_z;
} HandGadgetManipulator;
typedef struct HandGadgetAnimation {
    s32 resource_first;
    s32 resource_count;
    s32 primary_animation;
    s32 delay_frames;
    s32 item_animation;
    s32 secondary_animation;
    s32 attachment0_class;
    s32 attachment0_animation;
    s32 attachment1_class;
    s32 attachment1_animation;
    s32 attachment2_class;
    s32 attachment2_animation;
} HandGadgetAnimation;
extern u8 D_0013D5C8_255F8[] __asm__("D_0013D5C8");
typedef struct HandGadgetPlayerState {
    u8 pad0[0x10B8];
    s32 equipped_gadget;
    u8 pad10BC[0xF3A];
    u8 ammo_used;
    u8 ammo_capacity;
} HandGadgetPlayerState;
extern HandGadgetPlayerState D_0013F450_255F8 __asm__("D_0013F450");
extern s32 D_00160050 MACRO_ADDR;
extern HandGadgetDefinition D_001864D0_255F8[] __asm__("D_001864D0");
extern u8 * D_001B3580_255F8[] __asm__("D_001B3580");
extern u8 D_001B3E40[];
extern HandGadgetAnimation D_001D5668[];
extern HandGadgetSelection D_001D5F70_255F8 __asm__("D_001D5F70");
extern HandGadgetManipulator D_001D6160_255F8 __asm__("D_001D6160");
extern HandGadgetManipulator D_001D61A0_255F8 __asm__("D_001D61A0");
extern HandGadgetManipulator D_001D61E0_255F8 __asm__("D_001D61E0");
extern float D_001D6220[];
extern s32 D_001D6238_255F8[] __asm__("D_001D6238");
extern void func_001E97F0(s32, s32);
extern void func_001E97F8(Moby *, s32);
extern void func_00205270(s32, s32);
extern void func_0020D960_255F8(s32, s32, HandGadgetManipulator *) __asm__("func_0020D960");
extern void func_0020D9D8_255F8(s32, HandGadgetManipulator *) __asm__("func_0020D9D8");
extern void func_00213DE0(void *, int, int, int);
extern Moby *func_00226720_255F8(s32) __asm__("func_00226720");
extern Moby *func_002267C0_255F8(Moby *) __asm__("func_002267C0");
extern void func_00227100(s32, Moby *, Moby *, s32 *, s32 *, s32 *);
extern s32 func_00227890(s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32, s32);
extern s32 func_002279D0(void);
extern void func_00225E00();
extern void func_00225FB8();
extern void func_002260A8();
extern void func_00226250();
extern void func_00226380();
extern void func_00226410();

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/textbin/gameplay/gadgets/load_hand_gadget.c, load_hand_gadget. */
s32 func_002255F8(HandGadgetState *hand) {
    Moby *moby;
    s32 previous_selected_class;
    s32 previous_animation_class;
    s32 previous_attachment_class;
    s32 previous_pose_class;
    s32 previous_class_0197;
    s32 previous_class_0266;
    s32 previous_class;
    s32 requested_class;
    s32 selected_class;
    s32 selected_gadget;
    s32 loaded_gadget;
    s32 slot_index;
    s32 velocity_offset;
    HandGadgetAnimation *animation;
    float *ammo_offset;
    s32 selected_class_ready;
    Moby **ammo_moby_slot;
    loaded_gadget = 0;
    moby = hand->class_pose_moby;
    previous_selected_class = (moby != 0) ? (moby->oclass) : (-1);
    selected_gadget = D_001D5F70_255F8.selected_gadget;
    selected_class = D_001864D0_255F8[selected_gadget].oclass;
    selected_class_ready = selected_class == D_001D5F70_255F8.requested_resource_class;
    if ((previous_selected_class != selected_class) && selected_class_ready) {
        func_002267C0_255F8(moby);
        if (D_001D6160_255F8.active) {
            func_0020D9D8_255F8(hand->source_moby_address, &D_001D6160_255F8);
        }
        if ((D_0013F450_255F8.equipped_gadget != 0) &&
            (selected_gadget != D_0013F450_255F8.equipped_gadget)) {
            func_001E97F0(0, 0);
        }
        D_00160050 = D_001D5F70_255F8.D_00160050 == 0;
        func_00205270(selected_class, -1);
        D_001D5F70_255F8.active_resource_class = selected_class;
        D_001D5F70_255F8.D_00160050 = D_00160050;
        D_001D5F70_255F8.last_requested_resource_class = selected_class;
        D_001D5F70_255F8.last_resource_request_state = D_00160050;
        D_001B3580_255F8[D_001B3E40[selected_class]][0xD] = 0;
        moby = func_00226720_255F8(selected_class);
        if (moby != 0) {
            loaded_gadget = selected_gadget;
            if (D_0013E620[loaded_gadget]) {
                func_001E97F8(moby, hand->source_moby_address);
            }
            *moby->vars = hand;
            moby->update = func_00225E00;
            moby->update_kind = 4;
            if (loaded_gadget == 0x12) {
                func_0020D960_255F8(hand->source_moby_address, 0, &D_001D6160_255F8);
                D_001D6160_255F8.rotation_x = 0;
                D_001D6160_255F8.rotation_y = 0;
                D_001D6160_255F8.rotation_z = 0;
            }
        }
        hand->class_pose_moby = moby;
    }
    moby = hand->animation_moby;
    previous_animation_class = (moby != 0) ? (moby->oclass) : (-1);
    requested_class = D_001864D0_255F8[D_001D5F70_255F8.animation_gadget].oclass;
    if (previous_animation_class != requested_class) {
        moby = func_002267C0_255F8(moby);
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                loaded_gadget = D_001D5F70_255F8.animation_gadget;
                *moby->vars = hand;
                moby->update = func_00225FB8;
                moby->update_kind = 4;
            }
        }
        hand->animation_moby = moby;
    }
    moby = hand->first_attachment_moby;
    previous_attachment_class = (moby != 0) ? (moby->oclass) : (-1);
    requested_class = D_001864D0_255F8[D_001D5F70_255F8.attachment_gadget].oclass;
    if (previous_attachment_class != requested_class) {
        moby = func_002267C0_255F8(moby);
        if (D_001D61A0_255F8.active) {
            func_0020D9D8_255F8(hand->source_moby_address, &D_001D61A0_255F8);
        }
        if (D_001D61E0_255F8.active) {
            func_0020D9D8_255F8(hand->source_moby_address, &D_001D61E0_255F8);
        }
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                loaded_gadget = D_001D5F70_255F8.attachment_gadget;
                *moby->vars = hand;
                moby->update = func_002260A8;
                moby->update_kind = 4;
                func_0020D960_255F8(hand->source_moby_address, 0x16, &D_001D61A0_255F8);
                func_0020D960_255F8(hand->source_moby_address, 0x17, &D_001D61E0_255F8);
                D_001D61E0_255F8.rotation_z =
                    (D_001D61A0_255F8.rotation_z =
                         (D_001D61E0_255F8.rotation_y =
                              (D_001D61E0_255F8.rotation_x =
                                   (D_001D61A0_255F8.rotation_y =
                                        (D_001D61A0_255F8.rotation_x = 0.01f)))));
            }
        }
        hand->first_attachment_moby = moby;
        moby = func_002267C0_255F8(hand->second_attachment_moby);
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                *moby->vars = hand;
                moby->update = func_002260A8;
                moby->update_kind = 4;
            }
        }
        hand->second_attachment_moby = moby;
    }
    moby = hand->pose_moby;
    previous_pose_class = (moby != 0) ? (moby->oclass) : (-1);
    requested_class = D_001864D0_255F8[D_001D5F70_255F8.pose_gadget].oclass;
    if (previous_pose_class != requested_class) {
        moby = func_002267C0_255F8(moby);
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                *moby->vars = hand;
                moby->update = func_00226250;
                moby->update_kind = 4;
                if (moby->oclass == 0x25F) {
                    if (moby->anim != 6) {
                        func_00213DE0(moby, 6, 0, 10);
                    }
                    moby->state = 8;
                }
            }
        }
        hand->pose_moby = moby;
    }
    moby = hand->class_0197_moby;
    previous_class_0197 = (moby != 0) ? (moby->oclass) : (-1);
    requested_class = (D_0013D5C8_255F8[0x23]) ? (0x197) : (-1);
    if (previous_class_0197 != requested_class) {
        moby = func_002267C0_255F8(moby);
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                *moby->vars = hand;
                moby->update = func_00226380;
                moby->update_kind = 4;
            }
        }
        hand->class_0197_moby = moby;
    }
    moby = hand->class_0266_moby;
    previous_class_0266 = (moby != 0) ? (moby->oclass) : (-1);
    requested_class = (D_0013D5C8_255F8[0x21]) ? (0x266) : (-1);
    if (previous_class_0266 != requested_class) {
        moby = func_002267C0_255F8(moby);
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                *moby->vars = hand;
                moby->update = func_00226380;
                moby->update_kind = 4;
            }
        }
        hand->class_0266_moby = moby;
    }
    moby = hand->class_026a_moby;
    previous_class = (moby != 0) ? (moby->oclass) : (-1);
    requested_class = (D_0013D5C8_255F8[0x22]) ? (0x26A) : (-1);
    if (previous_class != requested_class) {
        moby = func_002267C0_255F8(moby);
        if (requested_class != (-1)) {
            moby = func_00226720_255F8(requested_class);
            if (moby != 0) {
                *moby->vars = hand;
                moby->update = func_00226380;
                moby->update_kind = 4;
            }
        }
        hand->class_026a_moby = moby;
    }
    slot_index = 0;
    ammo_offset = D_001D6220;
    ammo_moby_slot = hand->ammo_mobys;
    do {
        moby = *ammo_moby_slot;
        previous_class = (moby != 0) ? (moby->oclass) : (-1);
        requested_class = (slot_index < D_0013F450_255F8.ammo_capacity) ? (0x1DF) : (-1);
        if (previous_class != requested_class) {
            moby = func_002267C0_255F8(moby);
            if (requested_class != (-1)) {
                moby = func_00226720_255F8(requested_class);
                velocity_offset = slot_index * 4;
                *ammo_offset = (slot_index < D_0013F450_255F8.ammo_used) ? (0.0f) : (3.0f);
                *(s32 *)((u8 *)D_001D6238_255F8 + velocity_offset) = 0;
                if (moby != 0) {
                    *moby->vars = hand;
                    moby->update = func_00226410;
                    moby->update_kind = 4;
                    moby->ammo_moby_slot = slot_index;
                }
            }
            *ammo_moby_slot = moby;
        }
        slot_index++;
        ammo_moby_slot++;
        ammo_offset++;
    } while (slot_index < 8);
    if (((loaded_gadget != D_001D5F70_255F8.current_gadget) && (loaded_gadget > 0)) &&
        (loaded_gadget < 0x24)) {
        D_001D5F70_255F8.current_gadget = loaded_gadget;
        func_002279D0();
        func_00227890(D_001D5668[loaded_gadget].primary_animation +
                                    D_001D5F70_255F8.animation_base,
                                0, D_001D5668[loaded_gadget].delay_frames, loaded_gadget,
                                D_001D5668[loaded_gadget].item_animation,
                                D_001D5668[loaded_gadget].attachment0_class,
                                D_001D5668[loaded_gadget].attachment0_animation,
                                D_001D5668[loaded_gadget].attachment1_class,
                                D_001D5668[loaded_gadget].attachment1_animation,
                                D_001D5668[loaded_gadget].attachment2_class,
                                D_001D5668[loaded_gadget].attachment2_animation,
                                D_001D5668[loaded_gadget].resource_first,
                                D_001D5668[loaded_gadget].resource_count);
        if (D_001D5668[loaded_gadget].delay_frames != 0) {
            func_00227890(D_001D5668[loaded_gadget].secondary_animation +
                                        D_001D5F70_255F8.animation_base,
                                    2, 0, loaded_gadget, 1, -1, 0, -1, 0, -1, 0, 0, 0);
        }
    }
    for (slot_index = 0; slot_index < 0x18; slot_index++) {
        if (hand->timers[slot_index] != 0) {
            hand->timers[slot_index]--;
        }
    }

    func_00227100(hand->source_moby_address, hand->class_pose_moby,
                                             hand->animation_moby, &hand->x60, &hand->x64,
                                             &hand->x68);
    return 0;
}

void func_00225DF0(void) {
}

void func_00225DF8(void) {
}

extern char D_001864D0_a[] __asm__("D_001864D0");
extern int func_0020E3D0(void *);
extern void func_0020ED48(void *);
extern void func_0020DAF8(int, int, void *);
extern void func_00214F78(void *);
extern void func_0020EEE8(void *);
extern void func_001E9800(void *, void *, int, int, int);

/* The pause-screen mobys these updates drive. */
typedef struct {
    char pad00[0x10];
    float pos[4];       /* 0x10 */
    char pad20[4];
    int sound;          /* 0x24 */
    char pad28[0x28];
    int x50;            /* 0x50 */
    int x54;            /* 0x54 */
    char pad58[0x10];
    char *x68;          /* 0x68 */
    char *x6C;          /* 0x6C */
    char pad70[8];
    char **cls;         /* 0x78 */
    char pad7C[0x2A];
    short oclass;       /* 0xA6 */
    char padA8[0x18];
    float mtx[16];      /* 0xC0 */
} PauseMoby;

/* D_001864D0's 0x4C-byte per-class records. */
typedef struct {
    char pad00[0xC];
    int bone;           /* 0x0C */
    int cls;            /* 0x10 */
    char pad14[4];
    int still;          /* 0x18 */
    char pad1C[0x30];
} PauseClassRec;

extern PauseClassRec D_001864D0_r[] __asm__("D_001864D0");
extern void func_00213DE0(void *, int, int, int);
extern void func_0020D9D8(int, void *);
extern void func_0020D960(int, int, void *);

/* Moby update with a per-class record: find the class in D_001864D0 (0x4C
   bytes each, 0x25 of them), refresh the matrix from the record's bone,
   and either register the moby (+0x18 set) or restart its idle sound;
   also re-sync the D_001D6160 animation group while its +1 flag is up.
   The two store groups are in the order that schedules as retail's. */
void func_00225E00(void *arg0) {
    PauseMoby *m = arg0;
    unsigned char *b = arg0;
    float mtx[16];
    int id = *(int *)(*m->cls + 0x44);
    int i;
    int still;
    int sync;
    unsigned char *s;

    if ((b[0x70] & 2) && b[0x53] != 1) {
        func_00213DE0(m, 1, 0, 0);
    }
    for (i = 0; i < 0x25; i++) {
        if (D_001864D0_r[i].cls == m->oclass) {
            break;
        }
    }
    func_0020DAF8(id, D_001864D0_r[i].bone, mtx);
    qcopy(m->pos, &mtx[12]);
    func_0020ED48(m);
    still = D_001864D0_r[i].still == 0;
    func_001FA480(m->mtx, mtx);
    if (!still) {
        normalize_vector_triplet(m->mtx);
    }
    func_0020EEE8(m);
    sync = 0;
    s = (unsigned char *)D_001D6160;
    if (s[1] != 0) {
        sync = 1;
        DetachManipulator(id, s);
    }
    if (still) {
        func_001E9800(D_001864D0_a, D_001864D0, m->sound, 0, id);
        m->x68 = D_001864D0;
        m->x54 = 0;
        m->x6C = D_001864D0;
        *(int *)(b + 0x58) = 0;
        m->x50 = 0;
    }
    if (sync) {
        AttachManipulator(id, 0, s);
        *(int *)(s + 0x20) = 0;
        *(int *)(s + 0x24) = 0;
        *(int *)(s + 0x28) = 0;
    }
}

/* Moby update: refresh its matrix from bone 4 of its class (+0x44),
   copying the translation row to the position, re-register it, start
   its idle sound (6 for class 0x1B1, else 0) on the D_001864D0 table
   and reset the sound fields. */
void func_00225FB8(PauseMoby *m) {
    float mtx[16];
    int id = *(int *)(*m->cls + 0x44);

    func_0020E3D0(m);
    func_0020ED48(m);
    func_0020DAF8(id, 4, mtx);
    qcopy(m->pos, &mtx[12]);
    func_001FA480(m->mtx, mtx);
    normalize_vector_triplet(m->mtx);
    func_0020EEE8(m);
    if (m->oclass == 0x1B1) {
        func_001E9800(D_001864D0_a, D_001864D0, m->sound, 6, id);
    } else {
        func_001E9800(D_001864D0_a, D_001864D0, m->sound, 0, id);
    }
    m->x50 = 0;
    m->x68 = D_001864D0;
    m->x54 = 0;
    m->x6C = D_001864D0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_002260A8);

extern void func_00213DE0(void *, int, int, int);

/* Moby 0x259 update: while flag 0x02 is set, keep its animation on 1 (or,
   for class 0x25F in state 6, run the 6 animation until its timer +0x20
   runs out and then blend back to 1), then refresh its matrix from bone 5
   as func_00225FB8 does. */
void func_00226250(void *arg0) {
    PauseMoby *m = arg0;
    unsigned char *b = (unsigned char *)m;
    float mtx[16];
    int id = *(int *)(*m->cls + 0x44);

    if (b[0x70] & 2) {
        if (m->oclass == 0x25F) {
            if (b[0x52] == 6) {
                if (--b[0x20] == 0) {
                    if (b[0x53] != 1) {
                        func_00213DE0(m, 1, 0, 10);
                    }
                } else if (b[0x53] != 6) {
                    func_00213DE0(m, 6, 0, 0);
                }
            } else if (b[0x53] != 1) {
                func_00213DE0(m, 1, 0, 0);
            }
        } else if (b[0x53] != 1) {
            func_00213DE0(m, 1, 0, 0);
        }
    }
    func_0020DAF8(id, 5, mtx);
    qcopy(m->pos, &mtx[12]);
    func_001FA480(m->mtx, mtx);
    normalize_vector_triplet(m->mtx);
    func_0020EEE8(m);
}

/* func_00225FB8's sibling: refresh the matrix from bone 0x1E (class 0x197) or 0x1D, copy its translation row to the position and re-register. func_0020E3D0 takes the moby, and the bone test is written == 0x197 so the movn picks 0x1D as retail does. */
void func_00226380(PauseMoby *m) {
    float mtx[16];
    int id = *(int *)(*m->cls + 0x44);

    func_0020E3D0(m);
    func_0020ED48(m);
    func_0020DAF8(id, m->oclass == 0x197 ? 0x1E : 0x1D, mtx);
    qcopy(m->pos, &mtx[12]);
    func_001FA480(m->mtx, mtx);
    normalize_vector_triplet(m->mtx);
    func_0020EEE8(m);
}

struct AmmoPreviewMoby;
struct AmmoPreviewOwner {
    u8 pad00[0x44];
    struct AmmoPreviewMoby *source_moby;
};
struct AmmoPreviewVars {
    struct AmmoPreviewOwner *owner;
};
struct AmmoPreviewResource {
    u8 pad00[0x24];
    f32 scale;
};
struct AmmoPreviewMoby {
    u8 pad00[0x10];
    f32 position[4];
    u8 pad20[4];
    struct AmmoPreviewResource *resource;
    u8 pad28[4];
    f32 scale;
    u8 pad30[0x48];
    struct AmmoPreviewVars *preview_vars;
    u8 pad7C[0x40];
    u8 slot;
    u8 padBD[3];
    f32 basis[12];
};
extern f32 D_0015EE6C MACRO_ADDR;
extern short D_0015EE70;
extern f32 D_001D6220[];
extern f32 D_001D6238[];
extern void func_001F9BC0(void *);
extern f32 func_001F9F90(f32);
extern f32 func_001F9FA8(f32);
extern void func_001FA1F8(void *, void *);
extern void func_001FA4F0(void *, void *, void *);
extern f32 func_001FA748(f32, f32);
extern f32 func_001FA7D8_26410(f32) __asm__("func_001FA7D8");
extern f32 func_001FA888(s32);
extern void func_0020E3D0_26410(void *) __asm__("func_0020E3D0");
extern f32 func_00214D88(f32 *, f32 *, f32, f32, f32, f32);
void func_00226410(struct AmmoPreviewMoby *moby);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/ui/menus/item_preview/update_ammo_preview_transform.c, update_ammo_preview_transform. */
void func_00226410(struct AmmoPreviewMoby *moby) {
    f32 offset[4];
    f32 rotation_basis[12];
    f32 angles[4];
    struct AmmoPreviewMoby *source_moby;
    f32 *basis;
    s32 phase_index;
    f32 phase;
    f32 orbit_angle;
    f32 phase_angle;
    f32 bob_angle;
    f32 double_phase;
    f32 zero;

    source_moby = moby->preview_vars->owner->source_moby;
    func_0020E3D0_26410(moby);
    func_0020ED48(moby);
    qcopy(moby->position, source_moby->position);
    basis = moby->basis;
    func_001FA480(basis, source_moby->basis);
    func_00214F78(basis);
    moby->scale = moby->resource->scale;
    if (moby->slot < 3U) {
        phase_index = (moby->slot * 2) % 6;
    } else {
        phase_index = (moby->slot * 2 + 1) % 6;
    }
    orbit_angle = (((f32)(D_0015F538 % 200) / func_001FA888(200)) * 6.28318f) - 3.14159f;
    phase = (f32)phase_index;
    phase_angle = ((phase * 6.28318f) / func_001FA888(6)) - 3.14159f;
    bob_angle = (((f32)(D_0015F538 % 170) / func_001FA888(170)) * 6.28318f) - 3.14159f;
    double_phase = (phase * 12.56636f) / func_001FA888(6);
    func_001FA7D8_26410(double_phase);
    zero = 0.0f;
    orbit_angle = func_001FA748(orbit_angle, phase_angle);
    bob_angle = func_001FA748(bob_angle, double_phase);
    if (D_001D6220[moby->slot] != zero) {
        func_00214D88(&D_001D6220[moby->slot],
                                  &D_001D6238[moby->slot],
                                  zero, 1.0f, (*(f32 *)&D_0015EE70) * 6.0f, D_0015EE6C * 6.0f);
    }
    offset[0] = func_001F9F90(orbit_angle);
    offset[1] = func_001F9FA8(orbit_angle);
    offset[2] = zero;
    offset[2] = func_001F9FA8(bob_angle) * 0.25f + 0.5f + D_001D6220[moby->slot];
    func_001F9EC0(offset, offset, source_moby->basis);
    func_001F9BD8(moby->position, moby->position, offset);
    func_001F9BC0(angles);
    angles[2] = func_001FA748(orbit_angle, 1.5707964f);
    func_001FA1F8(rotation_basis, angles);
    func_001FA4F0(moby->basis, rotation_basis, moby->basis);
    func_0020EEE8(moby);
}

/* Hoisted from the func_00227A70 block below so this earlier caller can
   see it -- a second NOT_SDA extern for the same symbol is a hard
   error. */
extern unsigned char D_001B3E40[] NOT_SDA;
extern void *func_0020D348(void);
extern void func_0020ED48(void *);
extern void func_0020E340(void *, int, int, int, int);

extern void *func_0020D348_c(int) __asm__("func_0020D348");

/* Spawns the pickup/marker object for slot arg0, unless the slot is
   disabled (0xFF in D_001B3E40). A fresh object gets 0xFF/0xFF/1 in the
   0x30 block, is registered, tinted mid-grey, and flagged 0x18 at +0x73
   when its descriptor says so. The +0x30 store is through `unsigned
   char` and comes before the halfword store, so the 0xFF stays in one
   saved register. CreateMoby takes oClass: passing arg0 on keeps $a0
   live, which puts the element address in $v0 as retail has it. */
void *func_00226720(int arg0) {
    char *o;

    if (D_001B3E40[arg0] == 0xFF) {
        return 0;
    }
    o = (char *)func_0020D348_c(arg0);
    if (o != 0) {
        *(unsigned char *)(o + 0x30) = 0xFF;
        *(short *)(o + 0x32) = 0xFF;
        *(char *)(o + 0x20) = 0;
        *(char *)(o + 0x31) = 1;
        func_0020ED48(o);
        func_0020E340(o, 0x202020, 0xE, 0xE, 0);
        if (*(unsigned char *)(*(int *)(o + 0x24) + 6) != 0) {
            *(char *)(o + 0x73) = 0x18;
        }
    }
    return o;
}

extern int D_0015F6F0 MACRO_ADDR;
extern void func_0020D678(void *); /* DeleteMoby */

int func_002267C0(int arg0) {
    if (arg0 == 0) {
        return 0;
    }
    DeleteMoby((void *)arg0);
    *(long *)(arg0 + 0x38) = D_0015F6F0;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00226808);

/*
 * Exact once the short-loop padding retail's assembler adds is reproduced
 * (tools/fix_short_loops.py); it was the whole residual. Taking the
 * D_001517D0 base into a local declared after the calls keeps its
 * %hi/%lo out of a callee-saved register, as retail has it.
 */
int func_002268F0(void *arg0) {
    int *p = (int *)((char *)arg0 + 0x44);
    unsigned short *q;
    int i;
    for (i = 0x17; i >= 0; i--) { *p = func_002267C0(*p); p++; }
    *(int *)((char *)arg0 + 0x3C) =
        func_00226F68(*(int *)((char *)arg0 + 0x3C));
    q = (unsigned short *)D_001517D0;
    if ((unsigned int)(q[0x2D] - 6) >= 2) { q[0x2D] = 5; }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00226978);

extern void func_0020E180(int, int);

int func_00226CF8(void *arg0) {
    int *p = (int *)((char *)arg0 + 0x44);
    int i = 0x17;
    do {
        if (*p != 0) {
            DrawMobyList(*p, 1);
        }
        i--;
        p++;
    } while (i >= 0);
    return 4;
}

void func_00226D48(void) {
}

INCLUDE_ASM("asm/nonmatchings/text", func_00226D50);


extern int func_00227018(int handle);

/* Find the first binding that is enabled (bit 0 clear, or set when
   `invert` is given), still has a key and is not already claimed, claim
   it, and scrub its buffer with 0xDEADBEEF. Returns the key, or 0 if
   there is nothing to claim. */
int func_00226EA8(int invert) {
    int i;
    int f;
    int n;

    for (i = 0; i < 5; i++) {
        if (invert != 0) {
            f = D_001D6448_t[i].flags ^ 1;
        } else {
            f = D_001D6448_t[i].flags;
        }
        if ((f & 1) == 0 && D_001D6448_t[i].key != 0 &&
            (D_001D6448_t[i].flags & 2) == 0) {
            D_001D6448_t[i].flags |= 2;
            n = get_stream_buffer_size(D_001D6448_t[i].key);
            FastMemSet(D_001D6448_t[i].key, 0xDEADBEEF, n);
            return D_001D6448_t[i].key;
        }
    }
    return 0;
}

/* Release the binding whose key matches. Bit 1 means "bound"; bit 2 on
   top of that means it also owns the shared 0xCB latch, which has to be
   handed back through func_00217588 first. Always returns 0 so callers
   can assign it straight over their handle. The flags are read from the
   table at every use, not kept in a local. */
int func_00226F68(int key) {
    int i;
    char *g;

    for (i = 0; i < 5; i++) {
        if (D_001D6448_t[i].key == key) {
            if ((D_001D6448_t[i].flags & 2) != 0) {
                if ((D_001D6448_t[i].flags & 4) != 0) {
                    g = D_001D5F70;
                    D_001D6448_t[i].flags ^= 4;
                    if (*(unsigned char *)(g + 0xCB) != 0) {
                        request_audio_stream_break();
                        g[0xCB] = 0;
                    }
                }
                D_001D6448_t[i].flags &= ~2;
                return 0;
            }
        }
    }
    return 0;
}

extern int D_001D6448[];

int func_00227018(int arg0) {
    int *e = D_001D6448;
    int i = 0;
    do {
        if (e[0] == arg0) {
            return (e[1] & 1) ? 0x4F000 : 0x11800;
        }
        i++;
        e += 2;
    } while (i < 5);
    return -1;
}

int func_00227068(int arg0) {
    int i = 0;
    int *base = D_001D6448;
    int *e = base + 1;
    do {
        i++;
        if (e[-1] != arg0) {
            e += 2;
            continue;
        }
        e[0] |= 4;
        return 0;
    } while (i < 5);
    return 1;
}

/* Twin of func_00227068 above, clearing bit 2 instead of setting it.
   Written in exactly that function's shape -- the separate `base` local
   is what stops %lo+4 folding into one addiu, which is what an earlier
   round's revert was missing. */
int func_002270B0(int arg0) {
    int i = 0;
    int *base = D_001D6448;
    int *e = base + 1;
    do {
        i++;
        if (e[-1] != arg0) {
            e += 2;
            continue;
        }
        e[0] &= ~4;
        return 0;
    } while (i < 5);
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00227100);

extern int D_00160450 MACRO_ADDR;
typedef struct {
    int unk00;
    int arg[13];
} Rec38;
extern Rec38 D_001D6250[];

int func_00227890(int a0, int a1, int a2, int a3, int a4, int a5, int a6,
                  int a7, int a8, int a9, int a10, int a11, int a12) {
    int n = D_00160450;
    Rec38 *r;
    if (n >= 8) {
        return -1;
    }
    D_00160450 = n + 1;
    r = &D_001D6250[n];
    r->arg[0] = a0;
    r->arg[1] = a1;
    r->arg[2] = a2;
    r->arg[3] = a3;
    r->arg[4] = a4;
    r->arg[5] = a5;
    r->arg[6] = a6;
    r->arg[7] = a7;
    r->arg[8] = a8;
    r->arg[9] = a9;
    r->arg[10] = a10;
    r->unk00 = 0;
    r->arg[11] = a11;
    r->arg[12] = a12;
    return 0;
}

/* Pops the head of the 8-entry queue at D_001D6250. Copying through
   `d = &D_001D6250[i - 1]; *d = d[1];` makes the store the loop's
   master giv; the plain form reverses the loop. */
int func_00227928(void) {
    int i;
    for (i = 1; i < 8; i++) {
        Rec38 *d = &D_001D6250[i - 1];
        *d = d[1];
    }
    D_00160450--;
    return 0;
}

extern int D_001D641C;

int func_002279D0(void) {
    if (D_001517D0[4] != 0) {
        char *g = D_001D5F70;
        if (*(unsigned char *)(g + 0xCB) != 0) {
            request_audio_stream_break();
            g[0xCB] = 0;
        }
    }
    D_001D641C = 0;
    D_00160450 = 0;
    return 0;
}

void func_00227A30(void *arg0, char *src) {
    char *d = (char *)arg0;
    unsigned short v0, v1;
    int w0, w1;

    *(short *)(d + 0) = 0;
    v0 = *(unsigned short *)(src + 0x24);
    *(short *)(d + 4) = 0;
    *(short *)(d + 2) = v0;
    v1 = *(unsigned short *)(src + 0x20);
    *(short *)(d + 6) = v1;
    w0 = *(int *)(src + 0x20);
    w0 >>= 1;
    *(short *)(d + 8) = w0;
    w1 = *(int *)(src + 0x24);
    *(short *)(d + 0x10) = 0x10;
    w1 >>= 1;
    *(short *)(d + 0x12) = 0;
    *(short *)(d + 0xA) = w1;
}

extern char D_001D5D58[] NOT_SDA;
extern char *D_001B3580[] NOT_SDA;

typedef struct {
    u8 pad0[0x30];
    s32 active_items[3];
    u8 pad3C[0x64];
    s32 buffer_address[2];
    s32 resource_first;
    s32 resource_count;
    s32 resource_buffer_address[3];
    s32 read_offset;
    u8 padC0[8];
    u8 loaded_animation[2];
    u8 read_buffer_index;
    u8 pending_buffer;
    u32 streamed_animation_base;
} PreviewAnimationStreamState;
typedef struct {
    s32 class_id;
    s32 animation_index;
} PreviewResourceBinding;
extern u8 D_001B3580_27A70[] __asm__("D_001B3580");
extern u8 D_001B3E40[];
extern u8 D_001D5D58_27A70[] __asm__("D_001D5D58");
extern PreviewAnimationStreamState D_001D5F70_27A70 __asm__("D_001D5F70");
void func_00227A70(void);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/ui/menus/item_preview/clear_preview_resource_bindings.c, clear_preview_resource_bindings. */
void func_00227A70(void) {
    register s32 class_slot;
    register s32 class_resource_address;
    s32 resource_index;
    register u8 *binding_base;
    PreviewResourceBinding *binding;
    register s32 animation_address;
    register s32 resource_first;
    register s32 resource_count;
    register s32 resource_end;

    resource_index = D_001D5F70_27A70.resource_first;
    if (resource_index < (resource_index + D_001D5F70_27A70.resource_count)) {
        u8 *class_slots = D_001B3E40;
        u8 *class_resources = D_001B3580_27A70;
        u8 *bindings = D_001D5D58_27A70;

        binding_base = bindings;
        binding = (PreviewResourceBinding *) ((resource_index * 8) + binding_base);
        do {
            resource_index += 1;
            class_slot = *((u8 *) (binding->class_id + (s32) class_slots));
            class_resource_address = *(s32 *) ((class_slot * 4) + class_resources);
            animation_address = class_resource_address + (binding->animation_index * 4);
            *(s32 *) (animation_address + 0x48) = 0;
            binding += 1;
            resource_first = D_001D5F70_27A70.resource_first;
            resource_count = D_001D5F70_27A70.resource_count;
            resource_end = resource_first + resource_count;
        } while (resource_index < resource_end);
    }
    D_001D5F70_27A70.resource_count = 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_00227B00);

extern char D_0015EF98[] MACRO_ADDR;
extern char D_00141FC0[];
extern void func_00121A80(void *);
extern void func_0012D818(void *);
extern void func_00208FA0(void);
extern void func_00208338(void *);

void func_00227C78(int arg0, int arg1) {
    char *b = D_0013D390;
    func_00121A80(D_0015EF98);
    func_0012D818(D_0015EF98);
    func_00208FA0();
    func_00208338(D_00141FC0 + (D_0015EE84 << 11));
    memcard_MakeWholeSave((char *)arg0);
    *(int *)(b + 0xF4) = arg0;
    *(int *)(b + 0x14) = arg1;
    *(int *)(b + 0xC8) = 0;
    if (*(int *)(b + 0xE4) < 0) {
        *(int *)(b + 0xE8) = 0;
        *(int *)(b + 0xE4) = 0x13;
    }
}

/* func_00209DC0 (memcard.c) takes nothing. */
extern void func_00209DC0(void);

/* func_00227C78's sibling: runs func_00209DC0, the D_0015EF98 pair and
   func_0020BA00(arg0), then records arg0/arg1 in D_0013D390 (+0xF4,
   +0x14), clears +0xC8 and page arg1's first word, and seeds the result
   (+0xE4 = 0x13) when none is pending. func_00209DC0 is called without
   arguments: passing arg0 gives it one more reference and so the first
   callee-saved register, the reverse of retail's. */
void func_00227D20(int arg0, int arg1) {
    func_00209DC0();
    func_00121A80(D_0015EF98);
    func_0012D818(D_0015EF98);
    memcard_MakeWholeSave((char *)arg0);
    {
        char *b = D_0013D390;
        *(int *)(b + 0xC8) = 0;
        *(int *)(b + 0x14) = arg1;
        *(int *)(b + arg1 * 0x1C + 0x20) = 0;
        *(int *)(b + 0xF4) = arg0;
        if (*(int *)(b + 0xE4) < 0) {
            *(int *)(b + 0xE8) = 0;
            *(int *)(b + 0xE4) = 0x13;
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_00227DB0);

extern int D_0015F6D0 MACRO_ADDR;
extern int D_0015EF24 MACRO_ADDR;
extern int D_0015EE80 MACRO_ADDR;

void func_00228110(void) {
    int t;
    if (D_0015F6D0 != 0) {
        return;
    }
    t = D_0015EF24;
    D_0015EF24 = t + 1;
    if ((t + 1) % 50 == 0 && D_0015EE80 != 0) {
        D_0015EF24 = t + 0xB;
    }
}

extern int func_00216198(void);
extern int func_00216150(void);
extern char D_001D2B80[];
extern char D_001D2BF8[];

/* Unlock states for the pause menu's reward rows: with at least 15 / 30
   of the D_0013D510 flags (func_00216198) and 10 of the D_0013E620 ones
   (func_00216150), rows 5-8 of the 12-byte item table D_001D2B80 get
   state 3 (10 for row 7) instead of 2, and the menu at D_001D2BF8 its
   text ids; row 8's first half is cleared while D_0015EF90 is set. The
   three tests are kept as 0/1 values. Each item store reads the table
   through its own `char *` (retail's kept %hi); the short selects stay
   branches (HImode has no conditional move), the int ones become
   movz/movn. */
void func_00228160(void) {
    int a = count_nonzero_entries_up_to_30() >= 15;
    int b = count_nonzero_entries_up_to_30() >= 30;
    int c = count_nonzero_entries_up_to_10() >= 10;

    {
        char *g = D_001D2B80;
        *(short *)(g + 0x3E) = a ? 3 : 2;
    }
    {
        char *g = D_001D2B80;
        *(short *)(g + 0x4A) = b ? 3 : 2;
    }
    {
        char *g = D_001D2B80;
        *(short *)(g + 0x56) = c ? 10 : 2;
    }
    {
        char *g = D_001D2B80;
        *(short *)(g + 0x62) = c ? 3 : 2;
    }
    {
        char *h = D_001D2BF8;
        *(int *)(h + 0x14) = a ? 0x4FD3 : 0x4FD9;
        *(int *)(h + 0x18) = b ? 0x4FD4 : 0x4FDA;
        *(int *)(h + 0x1C) = c ? 0x4FD7 : 0x4FDB;
        *(int *)(h + 0x20) = c ? 0x4FD8 : 0x4FDB;
    }
    if (D_0015EF90_m != 0) {
        char *g = D_001D2B80;
        *(short *)(g + 0x60) = 0;
    }
}
__asm__(".section .text\n\tnop\n\tnop\n");

extern int D_001D6860[];
extern int D_001D74C0[];
extern int D_001D6760[];

void func_00228268(void) {
    FastMemSet(D_001D6860, 0, 0xC60);
    FastMemSet(D_001D74C0, 0, 0xC60);
    FastMemSet(D_001D6760, 0, 0x100);
}

LINKER_REMNANT("asm/remnants/text", func_002282B8);

extern float D_00160470[] MACRO_ADDR;
extern float D_00160470_x __asm__("D_00160470");
extern float D_00160474;
extern float D_00160478;
extern float D_00160480[] MACRO_ADDR;
extern float D_00160480_x __asm__("D_00160480") MACRO_ADDR;
extern float D_00160484;
extern float D_00160488;
extern float D_00160490;
extern float D_00160494;
extern float D_00160498;
extern void func_001F9CA0(void *, void *, void *);

typedef struct {
    float v[4];
} __attribute__((aligned(16))) PauseVec;

/* Build a basis from dir: D_00160470 = dir normalised,
   D_00160490 = that scaled, and D_00160480 = the cross product with a
   vector built from dir's components reordered (the smallest moved), so
   the result is perpendicular, then normalised. Compiled with
   -mno-split-addresses (config/func_cflags.txt): every global goes
   through the assembler's lui $at macro, and only D_00160480, declared
   small, uses $gp when it lands in a delay slot. The aligned struct
/* func_002282D0: matched only with -mno-split-addresses, which the rest of pause.c does not build with (docs/BUILD_FIDELITY.md, "Removed"). */
INCLUDE_ASM("asm/nonmatchings/text", func_002282D0);

/*
 * Dispatch on a leading short: 0 and 1 each call a handler and advance
 * the pointer differently, anything else returns it unchanged.
 *
 * An earlier round reverted this at 4 bytes short and read the residual
 * as a delay-slot problem -- retail spends the first jal's delay slot on
 * `addiu $16,$16,0x20`, we emitted a nop. The real cause was one level
 * up: with a `return` inside each arm, gcc folds the advance into the
 * return value (`addu $2,$16,32`, one instruction), and there is then
 * nothing left for the delay slot. Retail updates the pointer and copies
 * it to $v0 separately, three times, which is what a SINGLE `return p`
 * at the join gives: the copy belongs to the join block and the
 * delay-slot filler duplicates it into both branches.
 *
 * So this is the exit-cross-jumping lever used the other way round.
 * The usual reach is to SPLIT exits that gcc merged; here retail really
 * does share one, and the fix was to stop returning early. When a
 * 4-byte shortfall looks like a missing delay-slot fill, check first
 * whether an expression got folded that retail kept in two steps.
 */
void *func_00228400(char *p) {
    short v = *(short *)p;
    if (v == 0) {
        func_00229098(p);
        p += 0x20;
    } else if (v == 1) {
        func_002291E8(p);
        p += 0x30;
    }
    return p;
}

extern int *D_00161000 MACRO_ADDR;
extern char D_001D8120[];
extern short D_00160460;              /* SDA, gp -0x68A0 */
extern void func_002298B0(int, int);

void func_00228458(int arg0, int idx, int n) {
    D_00161000[0] = 0x30000003;
    D_00161000[1] = (int)(D_001D8120 + n * 0x30);
    D_00161000[2] = 0x13000000;
    D_00161000[3] = 0x50000003;
    D_00161000 += 4;
    func_002298B0(arg0, ((int *)&D_00160460)[idx]);
}

INCLUDE_ASM("asm/nonmatchings/text", func_002284E8);

/*
 * REVERTED (size mismatch: 452 vs retail's 460 -- 8 bytes short, after
 * closing an initial 28-byte gap). Semantics recovered with confidence --
 * builds a texture-paging GIF/DMA packet: a tag header, a fixed 10-field
 * 0x50-byte block (the field at hdr+0x48 is easy to miss -- it isn't
 * adjacent to the others), then (if the tile count n=w/32 is positive) a
 * per-tile table of packed TRXPOS-style coordinates built from the
 * screen width/height at D_00151880[0xA8]/[0xA9]:
 *
 *   void func_00228690(unsigned long arg0) {
 *       int w, h, n, i;
 *       long ypack_a, ypack_b, xbase_a, xbase_b;
 *       char *hdr, *table, *newptr;
 *
 *       w = D_00151880[0xA8];
 *       h = D_00151880[0xA9];
 *       n = w / 32;
 *
 *       D_00161000[0] = (n + 5) | 0x10000000;
 *       D_00161000[1] = 0;
 *       D_00161000[2] = 0;
 *       D_00161000[3] = (n + 5) | 0x50000000;
 *       D_00161000 += 4;
 *
 *       hdr = (char *)D_00161000;
 *
 *       *(unsigned long *)(hdr + 0x00) = ((unsigned long)0x8000 << 45) | 1;
 *       *(unsigned long *)(hdr + 0x48) = 0x44;
 *       *(unsigned long *)(hdr + 0x08) = 0xE;
 *       *(unsigned long *)(hdr + 0x10) = 0x3D801;
 *       *(unsigned long *)(hdr + 0x18) = 0x47;
 *       *(unsigned long *)(hdr + 0x20) = ((unsigned long)0x9000 << 46) | 1;
 *       *(unsigned long *)(hdr + 0x28) = 0x10;
 *       *(unsigned long *)(hdr + 0x30) = 0x146;
 *       *(unsigned long *)(hdr + 0x38) = arg0;
 *       *(long *)(hdr + 0x40) = (long)(n | 0x8000) | ((long)0x9000 << 46);
 *
 *       if (n > 0) {
 *           long v0, v1;
 *
 *           ypack_a = (long)(0x8000 - h * 8) << 16;
 *           ypack_b = (long)(h * 8 + 0x7FF0) << 16;
 *           xbase_a = -(w * 8) + 0x8000;
 *           xbase_b = -(w * 8) + 0x8200;
 *
 *           table = hdr + 0x50;
 *           i = 0;
 *           do {
 *               v0 = xbase_a | ypack_a;
 *               v1 = xbase_b | ypack_b;
 *               *(long *)table = v0;
 *               i++;
 *               table += 8;
 *               xbase_b += 0x200;
 *               *(long *)table = v1;
 *               xbase_a += 0x200;
 *               table += 8;
 *           } while (i < n);
 *       }
 *
 *       newptr = hdr + 0x50 + n * 0x10;
 *       D_00161000 = (int *)newptr;
 *
 *       D_00161000[0] = 0x10000000;
 *       D_00161000[1] = 0;
 *       D_00161000[2] = 0x13000000;
 *       D_00161000[3] = 0;
 *       D_00161000 += 4;
 *   }
 *
 * (needs D_00161000 MACRO_ADDR). The per-tile loop is instruction-for-
 * instruction exact against retail (confirmed via diff -- this took
 * writing it as an incrementing-pointer do-while with both store values
 * precomputed up front, matching retail's exact interleaving of the
 * pointer bump between the two stores; a straightforward for-loop with
 * offset-indexed stores compiled to a different, larger schedule).
 * Residual: retail keeps BOTH w and h live in callee-saved registers
 * ($16/$17) across the whole function, needing a 0x20-byte frame; this
 * compiler only needs one saved register for the pair (keeping the other
 * in an ordinary temporary that happens to survive the header stores
 * unclobbered), needing a smaller frame -- 8 bytes under. Also builds a
 * few of the header's 64-bit constants via a different (same-length)
 * instruction encoding (`lui`+`dsll32` vs retail's `ori`+`dsll32`) for
 * the same value. Tried hoisting `i=0` earlier (made it worse: forced a
 * THIRD saved register instead of one); tried reordering the hdr+0x48
 * statement (no effect on size). Not reached further this pass.
 */
INCLUDE_ASM("asm/nonmatchings/text", func_00228690);

typedef u32 u128 __attribute__((mode(TI), aligned(16)));
struct GraphicsSetupRecord {
    s32 command_count;
    s32 command_flags;
    f32 second_depth;
    f32 first_depth;
    u128 direction;
};
extern u8 * D_00161000_28860 __asm__("D_00161000") MACRO_ADDR;
struct VideoModeState {
    s32 v;
};
extern struct VideoModeState D_0015EE80_28860 __asm__("D_0015EE80") MACRO_ADDR;
extern u8 D_001D8250[];
extern u8 D_001D81E0[];
extern s32 D_001604A0 MACRO_ADDR;
extern void func_002284E8(void);
extern void func_002282D0_28860(f32 *, f32) __asm__("func_002282D0");
extern u8 *func_00228400_28860(u8 *) __asm__("func_00228400");
extern void func_00229838(f32 *, f32 *);
extern void func_00228D20(u32, s32, s32);
extern void func_00228458(s32, s32, s32);
extern void func_00234C98_28860(s32, s64) __asm__("func_00234C98");
extern void func_00228690(s64);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/textbin/fun_00227548.c, submit_graphics_setup_command_stream. */
void func_00228860(u8 *command_stream) {
    f32 first_vector[4];
    f32 second_vector[4];
    f32 direction[4];
    struct GraphicsSetupRecord *record;
    s32 command_index;

    func_002284E8();
    record = (struct GraphicsSetupRecord *)command_stream;
    *(u32 *)(D_00161000_28860 + 0) = 0x30000007;
    *(u32 *)(D_00161000_28860 + 4) =
        (u32)(D_0015EE80_28860.v != 0 ? D_001D8250 : D_001D81E0);
    *(u32 *)(D_00161000_28860 + 8) = 0x13000000;
    *(u32 *)(D_00161000_28860 + 12) = 0x50000007;
    /* Remaining vector components are filled from each setup record. */
    second_vector[1] = first_vector[1] = second_vector[0] = first_vector[0] = 0.0f;
    D_00161000_28860 += 16;
    while (record->command_count != 0) {
        command_stream += 0x20;
        *(u128 *)direction = record->direction;
        func_002282D0_28860(direction, 1000.0f);
        first_vector[2] = record->first_depth;
        second_vector[2] = record->second_depth;
        for (command_index = 0; command_index < record->command_count; command_index++) {
            command_stream = func_00228400_28860(command_stream);
            func_00229838(first_vector, second_vector);
            func_00228D20(0x70000000, D_001604A0, record->command_flags);
            func_00228458(2, 1, 2);
            func_00228458(1, 0, 2);
        }
        record = (struct GraphicsSetupRecord *)command_stream;
    }
    if (D_0015EE80_28860.v) {
        func_00234C98_28860(0x4C, 0x80080);
    } else {
        func_00234C98_28860(0x4C, 0x80070);
    }
    func_00234C98_28860(0x42, 0x2000000064LL);
    func_00228690(0);
    func_00234C98_28860(0x47, 0x5360B);
    func_00234C98_28860(0x42, 0x8000000044LL);
}
