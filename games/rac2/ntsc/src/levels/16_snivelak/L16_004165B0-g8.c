#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef float f32;

extern char *D_L16_001B2B1C __attribute__((sda));
extern char *D_L16_001B2B1C_2;
extern char D_00189EA0[];
extern s32 func_00319BE8(char *, s32);

/* runs the object's start-up state once and marks the four sub-objects it carries as alive */
void func_L16_00418D28(char *arg0) {
    char *st = *(char **)(arg0 + 0x68);
    char *p;
    u8 x;

    if (*(u8 *)(arg0 + 0x20) == 0)
        *(u8 *)(arg0 + 0x20) = 1;
    if (*(u8 *)(arg0 + 0x20) != 1)
        return;

    p = (*(s32 *)st << 8) + D_L16_001B2B1C_2;
    if (p != 0) {
        x = *(u8 *)(p + 0x20);
        if (x != 0xFE) {
            if (x != 0xFD)
                return;
        }
    }
    if (func_00319BE8(D_00189EA0, *(s32 *)(st + 4)) == 0)
        return;
    if (*(s32 *)(st + 0x10) != -1)
        *(u8 *)(D_L16_001B2B1C + (*(s32 *)(st + 0x10) << 8) + 0xBC) = 1;
    if (*(s32 *)(st + 0x14) != -1)
        *(u8 *)(D_L16_001B2B1C + (*(s32 *)(st + 0x14) << 8) + 0xBC) = 1;
    if (*(s32 *)(st + 0x18) != -1)
        *(u8 *)(D_L16_001B2B1C + (*(s32 *)(st + 0x18) << 8) + 0xBC) = 1;
    if (*(s32 *)(st + 0x1C) != -1)
        *(u8 *)(D_L16_001B2B1C + (*(s32 *)(st + 0x1C) << 8) + 0xBC) = 1;
    *(u8 *)(arg0 + 0x20) = 2;
}

extern s32 D_L16_001B1FCC __attribute__((sda));
extern s32 D_L16_001B1FC4 __attribute__((sda));
extern s32 D_L16_001B1FC8 __attribute__((sda));
extern s32 D_L16_001B1FD0 __attribute__((sda));
extern s32 D_001B3AAC __attribute__((nosda));
extern f32 D_L16_001B3AAC __attribute__((sda));
extern s32 func_003219A0(char *);
extern void func_00313DD0(char *, s32, s32);
extern f32 func_002E73F8(f32);
extern void func_0041ECF8(char *, f32);

extern f32 D_L16_001B1C60 __attribute__((sda));
extern f32 D_001B1C64 __attribute__((nosda));
extern f32 D_001B1C68 __attribute__((nosda));
extern f32 D_001B1C6C __attribute__((nosda));
extern f32 D_L16_001B1C70 __attribute__((sda));
extern f32 D_001B1C74 __attribute__((nosda));
extern s32 D_L16_001B1C78 __attribute__((sda));
extern s32 D_001B1C7C __attribute__((nosda));
extern s32 D_L16_001B1C80 __attribute__((sda));
extern s32 D_001B1C84 __attribute__((nosda));
extern s32 D_001B1C88 __attribute__((nosda));
extern s32 D_001B1C8C __attribute__((nosda));
extern s32 D_L16_001B1C90 __attribute__((sda));
extern void func_003140C0(char *, f32, f32);
extern s32 func_00313EE0(s32, s32);
extern s32 func_00313EA0(s32);
extern s32 func_003320E0(char *, char *, u32, u32, f32, u32, u32);
extern f32 func_00313F38(f32, f32);
extern void func_002E7450(char *);
extern void func_002E7488(char *, char *, char *);

extern s32 D_0018B650 __attribute__((nosda));
extern s32 D_L16_001B16C0 __attribute__((sda));
extern void func_002C4368(s32);
extern void func_00433698(s32);

/* clears the level state word and reseeds the two engine subsystems */
void func_L16_00416E88(void) {
    D_0018B650 = 0;
    D_L16_001B16C0 = 0;
    func_002C4368(0);
    func_00433698(3);
}

extern f32 D_L16_001B1E90 __attribute__((sda));
extern f32 D_L16_001B1E94 __attribute__((sda));
extern void func_0031C698(char *, char *, char *, f32, f32, f32);

/* submits the sub-object at 0x68 with the level's angle scale */
void func_L16_0041C948(char *arg0) {
    char *p = arg0;
    char *st = *(char **)(p + 0x68);

    func_0031C698(p, st + 4, st, D_L16_001B1E90, D_L16_001B1E94 * 0.000290888245f, -1.0f);
}

extern char D_00189EA0[];
extern s32 D_L16_001B1A80 __attribute__((sda));
extern void func_002E7968(char *, s32 *, char *);
extern void func_002E7488(char *, char *, char *);

/* installs the two level matrices on the shared render object */
void func_L16_00416E30(char *arg0) {
    char *obj = D_00189EA0;

    func_002E7968(obj, &D_L16_001B1A80, arg0 + 0xC0);
    func_002E7488(obj, obj, arg0 + 0x10);
    *(f32 *)(obj + 0x18) = *(f32 *)(arg0 + 0xF8);
}

extern f32 D_L16_001B1C28 __attribute__((sda));
extern f32 D_L16_001B1C2C __attribute__((sda));
extern f32 D_L16_001B1C30 __attribute__((sda));
extern f32 D_L16_001B1C34 __attribute__((sda));

/* initialises the level object's scale/limit table */
void func_L16_00418218(char *p) {
    f32 a = D_L16_001B1C28;
    f32 b = D_L16_001B1C2C;
    f32 c = D_L16_001B1C30;
    f32 d = D_L16_001B1C34;

    *(long long *)(p + 112) = 0LL;
    *(int *)(p + 80) = 0;
    *(int *)(p + 84) = 0;
    *(float *)(p + 88) = 1.0f;
    *(int *)(p + 92) = 0;
    *(int *)(p + 96) = 0;
    *(float *)(p + 100) = 1.0f;
    *(float *)(p + 104) = 1.0f;
    *(float *)(p + 108) = 1.0f;
    *(float *)(p + 8) = a;
    *(float *)(p + 24) = b;
    *(long long *)(p + 128) = 0x0000FF9000000260LL;
    *(float *)(p + 40) = a;
    *(float *)(p + 4) = c;
    *(float *)(p + 36) = d;
    *(int *)(p + 0) = 0;
    *(int *)(p + 16) = 0;
    *(int *)(p + 32) = 0;
    *(int *)(p + 48) = 0;
    *(float *)(p + 12) = 1.0f;
    *(float *)(p + 28) = 1.0f;
    *(float *)(p + 44) = 1.0f;
    *(float *)(p + 56) = b;
    *(float *)(p + 20) = c;
    *(float *)(p + 52) = d;
    *(float *)(p + 60) = 1.0f;
}

/* The id-list walk below reads D_L16_001B2B1C as a bare symbol (a second name for the 4-byte
   `sda` pointer above, declared as a 12-byte array so the assembler keeps its loads absolute),
   while D_L16_001B2B38 is a plain int: its `.extern` comes last in the file, which leaves a load
   in a branch delay slot on $gp and every other load on `lui`/`lw`. */
extern unsigned char *D_L16_001B2B1C_base[3] __asm__("D_L16_001B2B1C") __attribute__((sda));
extern s32 D_L16_001B2B38;
extern unsigned short *D_L16_001FB660[];
extern s32 D_L16_001B1FE0 __attribute__((sda));
extern void func_00283098(char *, char *, char *);
extern void func_00282CC0(char *, char *, char *);
extern void func_00282CF0(char *, char *, char *);
extern void func_002AFDD8(char *, char *, char *, s32);
extern void func_002833F0(char *, char *);

/* Builds a transform in buf0 from the object's +0xC0 and +0x10 blocks, then, for every entity of
   the object's group (byte 0x21): derives buf1 from buf0 and the entity's position, has
   func_002AFDD8 fill buf2, stores the argument at +0xF4 (negated for class 0x125D) and rewrites
   the entity's +0x10 block from its +0xC0 frame and the two buffers. */
void func_L16_0041ECF8(char *self, f32 spin) {
    char buf0[0x10];
    char buf1[0x10];
    char buf2[0x10];
    char buf3[0x10];
    unsigned short *p;
    char *e;
    f32 v;

    func_00283098(buf0, (char *)&D_L16_001B1FE0, self + 0xC0);
    func_00282CC0(buf0, buf0, self + 0x10);
    for (p = D_L16_001FB660[*(u8 *)(self + 0x21)];
         *(u8 *)(self + 0x21) != 0xFF && *(u8 *)(self + 0x21) <= D_L16_001B2B38 && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L16_001FB660[*(u8 *)(self + 0x21)]);
         p++) {
        e = (char *)D_L16_001B2B1C_base[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        func_00282CF0(buf1, buf0, e + 0x10);
        func_002AFDD8(e, buf2, buf1, 0);
        v = spin;
        if (*(short *)(e + 0xAA) == 0x125D)
            v = -v;
        *(f32 *)(e + 0xF4) = v;
        func_002833F0(e + 0xC0, e + 0xF0);
        func_00283098(buf2, buf2, e + 0xC0);
        func_00282CF0(buf3, buf2, buf1);
        func_00282CF0(e + 0x10, e + 0x10, buf3);
    }
}

extern s32 D_001A7A00;
extern void func_002E7788(char *, char *, f32);
extern char D_00189E20[];
extern void func_00315728(char *, char *, s32, char *, f32);
extern void func_00315948(char *, char *);

/* next frame: arms the object once the shared render state names it, then hands its box to the collision pass */
void func_L16_0041C6A0(char *arg0) {
    char buf[0x30];
    char v[0x10];
    char *q;
    char *p;

    if (*(u8 *)(arg0 + 0x20) == 0) {
        *(u8 *)(arg0 + 0x20) = 1;
        *(u8 *)(arg0 + 0x31) = 0;
        *(u16 *)(arg0 + 0x34) |= 1;
    }
    if (*(u8 *)(arg0 + 0x20) != 1)
        return;
    q = D_00189E20;
    p = *(char **)(q + 0x33C);
    if (p == 0 || p != arg0)
        return;
    if (*(s32 *)(q + 0x340) == 0)
        return;
    *(s32 *)(v + 0) = 0;
    *(f32 *)(v + 4) = 1.0f;
    *(s32 *)(v + 8) = 0;
    *(f32 *)(v + 0xC) = 1.0f;
    func_002E7788(v, v, 10.0f);
    func_00315728(buf, arg0, 1, v, (f32)(D_001A7A00 * 200));
    *(u16 *)(buf + 0x1A) = *(u16 *)(arg0 + 0xAA);
    func_00315948(*(char **)(q + 0x2290), buf);
}
