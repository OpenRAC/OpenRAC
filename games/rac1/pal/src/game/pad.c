#include "common.h"
#include "structs.h"

/*
 * pad.cpp in the original source; text 0x217F68-0x218928.
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

typedef struct {
    char unk_000[0x180];
    unsigned char mode[4]; /* 0x180 */
    unsigned char act[4];  /* 0x184 */
    char f188;             /* 0x188 */
    char f189;             /* 0x189 */
    char unk_18A[2];
    short f18C;            /* 0x18C */
    char unk_18E[6];
    int port;              /* 0x194 */
    int state;             /* 0x198 */
    int padState;          /* 0x19C */
    int f1A0;              /* 0x1A0 */
    char unk_1A4[8];
    int f1AC;              /* 0x1AC */
    int f1B0;              /* 0x1B0 */
    char unk_1B4[8];
    int f1BC;              /* 0x1BC */
    char unk_1C0[0x1C];
    int f1DC;              /* 0x1DC */
    char unk_1E0[0x168];
    int f348;              /* 0x348 */
} Pad;

extern int func_00124EE0(int);
extern int func_00124DF0(int, void *);
extern int func_00125218(int, void *);
extern int func_00124D18(int, void *);
extern void func_00218188(); /* ClearPadInput(PAD &), defined below */
extern void func_002181F0(void *, void *, int);

/* UpdatePad(PAD &): latches a few per-frame fields, then asks
   func_00124EE0 for the port's state. Not 1: the input is cleared and
   the pad state machine reset. Otherwise state 0 reads two 4-byte info
   blocks (func_00124DF0 -> +0x180, with +0x1DC = 0x79 when the first word
   is 0xFFFFFFFF, and func_00125218 -> +0x184; more than 4 bytes counts
   as none) and moves to state 1, or 2 when the first block was too
   long; state 1 reads the buttons into func_002181F0 and counts +0x18C
   down; state 2 clears the input and sets +0x18C to 2. The two copy
   loops per block are as retail has them (0..n, then n..3). */
void func_00217F68(void *arg0) {
    Pad *p = arg0;
    unsigned char buf[0x40];
    int n, i, s;

    p->f1A0 = 0;
    p->f1AC = p->f348;
    p->f1BC = p->f1B0;
    p->f1B0 = 0;
    p->padState = func_00124EE0(p->port);
    if (p->padState == 1) {
        switch (s = p->state) {
        case 0:
            n = func_00124DF0(p->port, buf);
            if (*(unsigned int *)buf == 0xFFFFFFFF) {
                p->f1DC = 0x79;
            } else {
                p->f1DC = 0;
            }
            if (n < 5) {
                for (i = 0; i < n; i++) {
                    p->mode[i] = buf[i];
                }
                p->state = 1;
            } else {
                n = 0;
                p->state = 2;
            }
            for (i = n; i < 4; i++) {
                p->mode[i] = buf[i];
            }
            n = func_00125218(p->port, buf);
            if (n < 5) {
                for (i = 0; i < n; i++) {
                    p->act[i] = buf[i];
                }
            } else {
                n = 0;
            }
            for (i = n; i < 4; i++) {
                p->act[i] = buf[i];
            }
            ClearPadInput(p);
            p->f18C = 2;
            break;
        case 1:
            n = func_00124D18(p->port, buf);
            func_002181F0(p, buf, n);
            if (p->f18C != 0) {
                p->f18C--;
            }
            break;
        case 2:
            ClearPadInput(p);
            p->f18C = s;
            break;
        }
    } else {
        ClearPadInput(p);
        p->state = 0;
    }
    p->f188 = 0;
    p->f189 = 0;
}

/* Retail carries 4 bytes of inter-function padding after this endlabel. */
__asm__(".section .text\n\tnop\n");

/* ClearPadInput(PAD &) */
typedef struct {
    char unk_000[0x100];
    int a[16];      /* 0x100 */
    int b[16];      /* 0x140 */
    char unk_180[0x20];
    int f1A0, f1A4, f1A8, f1AC;
    int f1B0, f1B4, f1B8, f1BC;
    int f1C0, f1C4, f1C8, f1CC;
    int f1D0, f1D4, f1D8;
} PadClr;

/* ClearPadInput(PAD &). The stores are in the source order that gives
   retail's schedule. a and b are separate members, so each is its own
   loop giv; the second one (b) becomes the loop's base. */
void func_00218188(PadClr *p) {
    int i;

    p->f1B0 = 0;
    p->f1A0 = 0;
    p->f1A4 = 0;
    p->f1A8 = 0;
    p->f1D0 = 1;
    p->f1B4 = 0;
    p->f1B8 = 0;
    p->f1C0 = 0;
    p->f1C4 = 0;
    p->f1C8 = 0;
    p->f1D4 = 1;
    p->f1D8 = 0;
    for (i = 0; i < 16; i++) {
        p->a[i] = 0;
        p->b[i] = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_002181F0);

extern void func_00217F68(void *);

/* UpdatePad(void) */
void func_00218908(void) {
    func_00217F68(D_0013CA40);
}
