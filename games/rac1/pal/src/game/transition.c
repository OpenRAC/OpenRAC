#include "common.h"
#include "structs.h"

/*
 * transition.cpp in the original source; text 0x1E9E70-0x1EC038.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/*
 * Close but not yet byte-matching, new evidence for the sq/lq open
 * question below: this function only saves $ra (no $s0-$s7 at all), and
 * retail STILL spills it as `sq` here -- unlike every other function
 * seen so far, where retail consistently uses `sd` for a lone $ra save.
 * This compiler always uses `sd` for $ra regardless. Logic/instructions
 * otherwise identical (return func_0022C7E0(); ... 5 calls in a row,
 * body confirmed correct via objdump before reverting this to
 * INCLUDE_ASM):
 *   func_0022C7E0(); func_0022C188(); func_0022C870();
 *   func_00234C98(0x47, 0x5360B);
 *   func_00234C98(0x4E, 0x1000000 | (D_0015EF88 >> 13));
 * Means the sq/lq choice isn't purely "s-regs vs ra", it's something
 * more granular retail decides per-function (maybe per translation
 * unit, or some other property not yet isolated). See "Open toolchain
 * questions" in docs/DECOMP_PROGRESS.md.
 */
extern void func_0022C7E0(void);
extern void func_0022C188(void);
extern void func_0022C870(void);
extern void func_00234C98(int, long);
extern int D_0015EF88 MACRO_ADDR;

/* func_00234C98 is (int, long) and D_0015EF88 MACRO_ADDR; the older
   sq/sd note is obsolete. */
void func_001E9E70(void) {
    SetupSkyGifPaging();
    SkyLevelGeneric___maybe();
    DoSkyGifPaging();
    VU1_addGSregister(0x47, 0x5360B);
    VU1_addGSregister(0x4E, 0x1000000 | (D_0015EF88 >> 13));
}

INCLUDE_ASM("asm/nonmatchings/text", func_001E9EC8);

INCLUDE_ASM("asm/nonmatchings/text", func_001EABE8);

extern int D_0015F064 MACRO_ADDR;
extern int D_0015F060 MACRO_ADDR;
extern int D_001997FC;
/* gp-relative, no retail symbol: gp 0x166D00 - 0x7580 = 0x15F780
   (cursor into the table walked below). */
extern short D_0015F780;

/* Points the D_0015F780 cursor at entry arg0 of the table D_0015F064
   indexes into D_0015F060, past its 8-byte header, and keeps the
   header's first word in D_001997FC. Both table pointers are MACRO_ADDR
   (one register each), and the index goes first in the addition. */
void func_001EB300(int arg0) {
    int *entry = (int *)(arg0 * 4 + D_0015F064);
    int off = *entry;
    char *p = (char *)(D_0015F060 + off);

    D_001997FC = *(int *)p;
    p += 8;
    *(char **)&D_0015F780 = p;
}

extern char D_0018CC20[];
extern float D_0018CEB0;
extern char D_00187180[];
extern void func_001F3140(void);
extern void func_00125358(float *);
extern void func_001254A0(float *, float *, float);
extern void func_00125548(float *, float *, float);
extern void func_001253F8(float *, float *, float);

/* Transition_UpdateMovieCamera: the current keyframe (0x20 bytes: position,
   a flag byte at +0xC, then X/Y/Z angles) sets the camera position and
   its orientation matrix (negated first two rows); returns the flag. */
unsigned char func_001EB338(void) {
    char *t = D_0018CC20;
    char *key = *(char **)(t + 0x54) + *(int *)(t + 0x38) * 32;
    unsigned char flag = key[0xC];
    float *ang = (float *)(key + 0x10);
    char *pos;
    char *cam;
    float m[16];

    D_0018CEB0 = 0.63f;
    UpdateViewContext();
    pos = D_00187180;
    qcopy(pos, key);
    func_00125358(m);
    func_001254A0(m, m, *(float *)(key + 0x10));
    func_00125548(m, m, ang[1]);
    func_001253F8(m, m, ang[2]);
    cam = pos - 0x140;
    *(float *)(cam + 0x350) = -m[8];
    *(float *)(cam + 0x360) = -m[0];
    *(float *)(cam + 0x370) = m[4];
    *(float *)(cam + 0x354) = -m[9];
    *(float *)(cam + 0x364) = -m[1];
    *(float *)(cam + 0x374) = m[5];
    *(float *)(cam + 0x358) = -m[10];
    *(float *)(cam + 0x368) = -m[2];
    *(float *)(cam + 0x378) = m[6];
    return flag;
}

INCLUDE_ASM("asm/nonmatchings/text", func_001EB458); /* Transition_FUN_001eb0a8 */

struct M2c_D_0016045C
{
  u8 pad_0[0x4];
  s16 unk4;
  u8 pad_6[0x2];
};
extern u8 D_00100AE0[];
extern s32 D_0013E604[];
extern short D_0015EE88;
extern short D_0015F048;
extern s32 D_0015F050 MACRO_ADDR;
extern s32 D_0015F054 MACRO_ADDR;
extern f32 D_0015F53C MACRO_ADDR;
extern s32 D_0015F564 MACRO_ADDR;
extern s32 D_0015F6E8 MACRO_ADDR;
extern s32 D_0015F704 MACRO_ADDR;
extern struct M2c_D_0016045C * D_0016055C MACRO_ADDR;
extern s32 D_0018A3E8[];
extern u8 D_001940C0[];
extern u8 D_001D9240[];
extern u8 D_001E1600[];
extern u8 D_001E3500[];
extern s32 func_00235290();
extern s32 func_001F99B0();
extern s32 func_00118D80();
extern s32 func_001E9E70_EB7C0() __asm__("func_001E9E70");
extern s32 func_001EB338_EB7C0() __asm__("func_001EB338");
extern s32 func_001F2608();
extern s32 func_001F2930();
extern void func_001F3C10();
extern s32 func_001F4630();
extern s32 func_001F4748();
extern u64 func_001F4868();
extern s32 func_001F4A00();
extern s32 func_001F55C0();
extern s32 func_001F5800();
extern s32 func_001FA898(f32);
extern s32 func_001FB530();
extern s32 func_001FB848();
extern s32 func_001FBE80();
extern s32 func_0020DAB0();
extern s32 func_0020DD48();
extern s32 func_0020E2B0();
extern void func_00218B10();
extern s32 func_0021A610();
extern s32 func_00229D48();
extern s32 func_00229E50();
extern s32 func_0022B8F8();
extern s32 func_00234620();
extern s32 func_002346C0();
extern s32 func_002347F0();
extern s32 func_00234AC8();
extern s32 func_00234C98_EB7C0() __asm__("func_00234C98");
extern void func_00234F40();
extern s32 func_002362B0();
extern s32 func_00236A98();
extern s32 func_00236BE0();
extern s32 func_00238688();
void func_001EB7C0(s32 *arg0);

/* Adapted from Lombyte (MIT) for PAL by OpenRAC's tools/port.py: src/gameplay/state/transition_default_draw.c, transition_default_draw. */
void func_001EB7C0(s32 *arg0)
{
    s32 n;

    if (D_0016055C == 0 || D_0016055C->unk4 != 0) {
        func_001FB530();
    }
    func_001F99B0(D_001940C0, -1, 0x80);
    func_001EB338_EB7C0();
    func_001F2608();
    func_0020DAB0();
    func_001F3C10();
    D_0015F704 = -1;
    if (D_0016055C != 0) {
        func_001E9E70_EB7C0();
    }
    func_002346C0();
    func_00235290(0x02010000);
    func_00236BE0();
    func_00235290(0x02020000);
    func_00229E50();
    func_00235290(0x02040000);
    if (D_0015F6E8 == 3) {
        func_0021A610();
    } else {
        func_0020E2B0();
    }
    func_00235290(0x02080000);
    func_001F4630(0);
    func_00234F40();
    if (D_0015F564 != 0) {
        func_001F4A00();
    }
    func_00234F40();
    if (D_0018A3E8[0] != 0) {
        func_00234C98_EB7C0(8, 5);
        func_00234F40();
        func_00118D80(0);
        func_00218B10();
        D_0015F704 = 8;
    }
    func_001FB848();
    func_001F3C10();
    if (D_0015F050 != 0) {
        func_001F5800(0xEC, 0x10, 0x100, 0x80, 0, 0, 0x100, 0x80,
                      (long)(D_0015F050 << 24 | 0x808080), (*(s64 *)&D_0015F048));
    }
    if (D_0015F054 != 0) {
        n = (*(s32 *)&D_0015EE88) - 1;
        if (n < 0) {
            n = 0;
        }
        func_001F5800(0xA0, D_0013E604[0] - 0x50, 0xC0, 0x60, 0, 0, 0x100, 0x80,
                      (long)(D_0015F054 << 24 | 0x808080), func_001F4868(n + 4));
    }
    func_001F4748();
    if (D_0015F53C > 0.0f) {
        if (D_0015F53C > 1.0f) {
            D_0015F53C = 1.0f;
        }
        func_001F55C0(0, 0, 0, func_001FA898(D_0015F53C * 128.0f));
    }
    func_002347F0(D_00100AE0);
    func_00118D80(0);
    if (D_0015F6E8 == 4) {
        func_001FBE80();
    }
    func_00234AC8(2);
    func_002362B0(D_001E1600);
    func_00234620();
    func_00234AC8(4);
    func_00238688(D_001E3500);
    func_00236A98();
    func_00234AC8(8);
    func_0022B8F8(D_001D9240);
    func_00229D48();
    func_00234AC8(0x10);
    func_0020DD48();
    func_001F2930();
}

extern char D_0013E650[];
extern int D_0015F694;

extern int D_0015F694_m __asm__("D_0015F694") MACRO_ADDR;

/* The older register residual was the split load of D_0015F694; read
   through a MACRO_ADDR alias it is retail's one-register load. */
int func_001EBAF0(int arg0, int arg1) {
    if (arg0 >= 0) {
        char *p = D_0013E650 + arg0 * 0x70;
        if (*(short *)(p + 0x7E) == arg1 + D_0015F694_m) {
            unsigned char s = p[0x74];
            if (s == 1 || s == 2) {
                return 1;
            }
        }
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/text", func_001EBB48);

/* Not a standalone function: single `addiu $sp,$sp,0x30`, no `jr $31` --
   fallthrough fragment, same category as func_00113AD8 in core_text. */
LINKER_REMNANT("asm/remnants/text", func_001EC030);
