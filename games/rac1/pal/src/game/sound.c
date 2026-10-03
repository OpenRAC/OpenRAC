#include "common.h"
#include "structs.h"

/*
 * sound.cpp in the original source; text 0x22D8C0-0x22F128.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char D_0013E650[];
extern int D_0015F694 MACRO_ADDR;
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
extern void func_00217F68(void *);
extern int D_0015EF90;
extern char D_001D4B90[];
extern char D_001D4BC0[];
extern char D_001D5F70[] NOT_SDA;
extern char D_001D603B[];
extern int D_001A0414;
extern int D_001CFBF4;
extern int D_001CFAD8;
extern void func_0020C7A0(void *);
extern int func_0020CA50(void *, void *, void *, int);
extern int D_00141FA0[];
extern char D_001D0A50[];
extern char D_001D0A88[];
extern int D_001A0418 NOT_SDA;
extern void func_00226D50(int);
extern float func_001FA748(float, float);
extern char *D_001D5F74 NOT_SDA;
extern void func_0020E180(int, int);
extern char D_00187040[];
extern void func_00220128(void *);
extern void *func_00226720_a(int) __asm__("func_00226720");
extern int func_002267C0(int);
extern void func_00234C98(int, int);
extern void func_00205E70(void);
extern void func_001F4630(int);
extern void func_001F4748(void);
extern void func_001F68E8_c(int, int, long, void *, int)
    __asm__("func_001F68E8");
extern void *func_001FE540_id(int) __asm__("func_001FE540");
extern short D_001602B0;
extern void func_00201640(int, int, int, int, long, long);
extern int func_00200198(int, int);
extern void func_00200468(int, int, int, int, int, int);
extern void func_001F5800(int, int, int, int, int, int, int, int, long,
                          long);
extern short D_00151880[];
extern long D_001A0448;
extern int func_00226EA8(int);
extern int func_00226F68(int);
extern int D_0013CC04 NOT_SDA;
extern char D_001D2678[];
extern char *D_001D5F78 NOT_SDA;
extern void func_001FDF78(int, int, int, int);
extern unsigned char D_001B3E40[] NOT_SDA;
extern void *func_0020D348(void);
extern void func_0020ED48(void *);
extern void func_0020E340(void *, int, int, int, int);
typedef struct {
    int key;
    int flags;
} PadBind;
extern PadBind D_001D6448_t[] __asm__("D_001D6448");
extern int func_00227018(int handle);
extern int D_001D6448[];
extern char D_001D5D58[] NOT_SDA;
extern char *D_001B3580[] NOT_SDA;
extern int D_001D6860[];
extern int D_001D74C0[];
extern int D_001D6760[];

extern char D_00187180_a[] __asm__("D_00187180");
extern char D_00194220[];
extern int D_0013E6BC;
extern void func_002141A8(void *, float, float);
extern void func_001F9BD8_a(void *, void *, void *) __asm__("func_001F9BD8");
extern void func_001F9C30_a(void *, void *, float) __asm__("func_001F9C30");
extern void func_001F9BF0_a(void *, void *, void *) __asm__("func_001F9BF0");
extern int func_001EFE10_a(void *, void *, int, int, int) __asm__("func_001EFE10");

/* Jitter the vector at arg0 between 0.5 and 6.0, fold in the reference
   frame at D_00187180, and if the emitter accepts it, re-derive it from
   D_00194220, damp to 3/4 and fold the frame in again. */
void func_0022D8C0(void *arg0) {
    func_002141A8(arg0, 0.5f, 6.0f);
    func_001F9BD8_a(arg0, arg0, D_00187180_a);
    if (func_001EFE10_a(D_00187180_a, arg0, 0x82, D_0013E6BC, 0) != 0) {
        func_001F9BF0_a(arg0, D_00194220, D_00187180_a);
        func_001F9C30_a(arg0, arg0, 0.75f);
        func_001F9BD8_a(arg0, arg0, D_00187180_a);
    }
}

extern char D_00187180[];
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9E58(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001EFE10(void *, void *, int, int, int);

/* Aim the vector from the frame at D_00187180 toward arg0+0x20, scaled to
   3/4 and 64, and return the line test from arg1 to its end. The int
   return keeps retail's argument order for func_001F9BD8. */
int func_0022D970(void *arg0, void *arg1) {
    float v[4];

    FastVecSub(v, (char *)arg0 + 0x20, D_00187180);
    FastVecScale(v, v, 0.75f);
    func_001F9E58(v, v, 64.0f);
    FastVecAdd(v, v, D_00187180);
    return func_001EFE10_a(arg1, v, 0x82, *(int *)((char *)arg0 + 0x18), 0);
}

extern float func_001FA888(int);
extern int func_001FA898_i(float) __asm__("func_001FA898");

/* Interpolates between the ints at +0x8 and +0xC as x runs from lo to
   hi, squared when bit 0 of +0x19 is set. Each arm is an if/else-if
   chain setting one `r` with a single return; early returns let jump.c
   move the last arm's return block to the end. It returns int, and its
   one caller here uses a void alias. */
int func_0022DA10(void *arg0, float x, float lo, float hi) {
    char *s = (char *)arg0;
    int r;
    if (*(unsigned char *)(s + 0x19) & 1) {
        if (x <= lo) r = *(int *)(s + 0xC);
        else if (hi <= x) r = *(int *)(s + 0x8);
        else r = *(int *)(s + 0x8) + func_001FA898_i((hi - x) * (hi - x) * func_001FA888(*(int *)(s + 0xC) - *(int *)(s + 0x8)) / ((hi - lo) * (hi - lo)));
    } else {
        if (x <= lo) r = *(int *)(s + 0xC);
        else if (hi <= x) r = *(int *)(s + 0x8);
        else r = *(int *)(s + 0x8) + func_001FA898_i((hi - x) * func_001FA888(*(int *)(s + 0xC) - *(int *)(s + 0x8)) / (hi - lo));
    }
    return r;
}

extern char D_00187180[];
extern float func_001F9D10(int, void *);
/* This caller's view of func_0022DA10 above: it ignores the result, and
   declaring it int would move its next temporary to $v1. */
extern void func_0022DA10_v(void *, float, float, float) __asm__("func_0022DA10");

void func_0022DB00(void *arg0, int arg1) {
    char *s = (char *)arg0;
    float v = FastVecDist(arg1, D_00187180);
    float *p = *(float **)(s + 0x8);
    func_0022DA10_v(p, v, p[0], p[1]);
}

extern void func_001F9EE8(void *, void *, void *);
extern float func_001F9CE8(void *);
extern float func_001F9BB0(float, float, float);
extern float func_001FA058_a(float, float) __asm__("func_001FA058");
extern void func_001FA898(float);

/* Build the vector at arg1 in the D_00187180 frame, project it against
   arg2, and steer by its heading: atan2 of the xy part, negated and
   converted from radians to degrees (180 * 1/pi), scaled by how far the
   projection runs past 1.0, clamped to 0..1. arg0 is unused. */
void func_0022DB48(void *arg0, void *arg1, void *arg2) {
    float v[4];
    float t;

    func_001F9BF0_a(v, arg1, D_00187180_a);
    func_001F9EE8(v, v, arg2);
    t = func_001F9BB0(func_001F9CE8(v) - 1.0f, 0.0f, 1.0f);
    truncate_float_to_s32(-func_001FA058_a(v[0], v[1]) * 180.0f * t * 0.31830987f);
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022DBE8);

INCLUDE_ASM("asm/nonmatchings/text", func_0022DD68); /* sound_update */

extern void func_00120F30(int);
extern int func_0012E060(void *, int);
extern int func_0012EE70(int);
extern void func_0012EF48(int);
extern void func_0012E2E8(void);

/* Reentrancy-guarded: base+0x44 is held at 1 for the duration. The
   return value is func_0012E060's, captured in the delay slot of the
   *next* call. That callee, func_0012EE70 (snd_StreamSafeCheckCDIdle),
   returns int: a value call resets $v0's readers, which lets the
   capture sink into its slot. */
int func_0022EA20(void *arg0) {
    char *base = D_0013E650;
    int r = 0;

    if (*(int *)(base + 0x44) == 0) {
        *(int *)(base + 0x44) = 1;
        func_00120F30(0);
        r = func_0012E060(arg0, 0);
        func_0012EE70(1);
        func_0012EF48(0);
        func_0012E2E8();
        *(int *)(base + 0x44) = 0;
    }
    return r;
}

/* 12 bytes of post-endlabel nop padding in retail -- see func_001F6668. */
__asm__(".section .text\n\tnop\n\tnop\n\tnop\n");

void func_0022EAB0(int idx) {
    if (idx >= 0) {
        char *e = D_0013E650 + idx * 0x70;
        unsigned char st = *(unsigned char *)(e + 0x74);
        if (st == 7) {
            *(int *)(e + 0x88) = 0;
            *(int *)(e + 0x8C) = 0;
            *(unsigned char *)(e + 0x74) = 0;
        } else if (st != 0 && st != 6) {
            *(unsigned char *)(e + 0x74) = 4;
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022EB08);

extern int func_0022EB08(void *, int, int, int, int);

/* Starts entry idx of the sound bank arg2 points at; the same shape as
   its siblings func_0022EE28 and func_0022EEB8. */
int func_0022ED80(int idx, int arg1, int arg2) {
    char *p;
    char *tbl;
    int h;

    if (arg2 == 0) {
        return -1;
    }
    p = *(char **)(arg2 + 0x24);
    if (p == 0) {
        return -1;
    }
    tbl = *(char **)(p + 0x28);
    if (tbl == 0) {
        return -1;
    }
    if (idx >= *(unsigned char *)(p + 0xD)) {
        return -1;
    }
    h = func_0022EB08(tbl + idx * 32, arg1, arg2, 0, 0x400);
    if (h >= 0) {
        char *rec = D_0013E650 + h * 0x70;
        *(int *)(rec + 0x88) = arg2;
        *(short *)(rec + 0x7E) = idx;
    }
    return h;
}

extern char *D_0015F714 MACRO_ADDR;
extern int func_0022EB08(void *, int, int, int, int);

int func_0022EE28(int idx, int arg1, int arg2) {
    int h;

    if (idx >= D_0015F694) {
        return -1;
    }
    h = func_0022EB08(D_0015F714 + idx * 32, arg1, arg2, 0, 0x400);
    if (h >= 0) {
        char *rec = D_0013E650 + h * 0x70;
        *(int *)(rec + 0x88) = arg2;
        *(short *)(rec + 0x7E) = idx;
    }
    return h;
}

extern int D_0015F710 MACRO_ADDR;

int func_0022EEB8(int rel, int arg1, int arg2) {
    int idx = rel + D_0015F694;
    int h;

    if (idx >= D_0015F710) {
        return -1;
    }
    h = func_0022EB08(D_0015F714 + idx * 32, arg1, arg2, 0, 0x400);
    if (h >= 0) {
        char *rec = D_0013E650 + h * 0x70;
        *(int *)(rec + 0x88) = arg2;
        *(short *)(rec + 0x7E) = idx;
    }
    return h;
}

INCLUDE_ASM("asm/nonmatchings/text", func_0022EF50);

static inline char *SndSys(void) {
    return D_0013E650;
}

/*
 * Calls each entry's +4 callback in the sound system's 0x90-byte entry
 * table (count at +0xD90, table at +0xD94), re-reading both every
 * iteration so a callback may grow the table. The inline accessor gives
 * each read of D_0013E650 its own pseudo: the duplicated exit test
 * keeps a temporary and the loop a callee-saved copy, as in retail.
 * `i * 0x90 + (int)table` (not an offset variable) gives the
 * offset-first addu.
 */
void func_0022EF68(void) {
    int i;
    for (i = 0; i < *(int *)(SndSys() + 0xD90); i++) {
        char *e = (char *)(i * 0x90 + *(int *)(SndSys() + 0xD94));
        void (*fn)(char *) = *(void (**)(char *))(e + 4);
        if (fn != 0) {
            fn(e);
        }
    }
}

/* 8 bytes of post-endlabel nop padding in retail -- see func_001F6668. */
__asm__(".section .text\n\tnop\n\tnop\n");

INCLUDE_ASM("asm/nonmatchings/text", func_0022EFE8); /* sound_StopAllSounds(void) */

void func_0022F090(int arg0, long arg1) {
    int *p = (int *)(int)arg1;
    if (p != 0) {
        *p = arg0;
    }
}

void func_0022F0A8(int arg0, long arg1) {
    unsigned char *p = (unsigned char *)(int)arg1;
    if (p != 0) {
        *(int *)p = arg0;
        if (arg0 != 0) {
            if (p[4] == 1) {
                p[4] = 2;
            }
        } else {
            *(int *)(p + 0x18) = 0;
            *(int *)(p + 0x1C) = 0;
            p[4] = 0;
        }
    }
}

void func_0022F0F0(int arg0, long arg1) {
    int *p = (int *)(int)arg1;
    if (p != 0) {
        *p = arg0;
        if (arg0 == 0) {
            *(int *)((char *)p + 0x18) = 0;
            *(int *)((char *)p + 0x1C) = 0;
            *(unsigned char *)((char *)p + 4) = 0;
        }
    }
}

LINKER_REMNANT("asm/remnants/text", func_0022F120);
