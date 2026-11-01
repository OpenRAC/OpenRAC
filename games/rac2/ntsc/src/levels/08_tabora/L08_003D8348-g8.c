#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef int s32;
typedef unsigned int u32;

/* the level's object-handle table: a pointer global holding the base of 0x100-byte entries */
typedef struct {
    u8 pad00[0x20];
    u8 f20;
    u8 f21;
    u8 pad22[0x68 - 0x22];
    void *f68;
    u8 pad6C[0x95 - 0x6C];
    u8 f95;
    u16 f96;
    u8 pad98[0xBC - 0x98];
    u8 fBC;
    u8 padBD[0x100 - 0xBD];
} Ent;

extern Ent *D_L08_001B29DC __attribute__((sda));

/* handle is valid and the entry's class byte is a live one */
s32 func_L08_003D8918(s32 a0)
{
    Ent *t;
    s32 i;

    if (a0 != -1) {
        t = D_L08_001B29DC;
        i = a0 << 8;
        return *(u8 *)((u8 *)t + i + 0x20) >= 3;
    }
    return 1;
}

/* handle is valid and the entry's class byte is a live one */
s32 func_L08_004229C8(s32 a0)
{
    Ent *t;
    s32 i;

    if (a0 != -1) {
        t = D_L08_001B29DC;
        i = a0 << 8;
        return *(u8 *)((u8 *)t + i + 0x20) >= 3;
    }
    return 1;
}

/* the entry's two faces are either unset or already gone */
s32 func_L08_003D8948(s32 a0)
{
    Ent *p;
    s32 r;

    if (a0 != -1) {
        p = D_L08_001B29DC + a0;
        r = 0;
        if (p == 0 || p->f20 == 0xFE || p->f20 == 0xFD) r = 1;
        return r;
    }
    return 1;
}

/* the entry's two faces are either unset or already gone */
s32 func_L08_003D8990(s32 a0)
{
    Ent *p;
    s32 r;

    if (a0 != -1) {
        p = D_L08_001B29DC + a0;
        r = 0;
        if (p == 0 || p->f20 == 0xFE || p->f20 == 0xFD) r = 1;
        return r;
    }
    return 1;
}

/* the entry's two faces are either unset or already gone */
s32 func_L08_004229F8(s32 a0)
{
    Ent *p;
    s32 r;

    if (a0 != -1) {
        p = D_L08_001B29DC + a0;
        r = 0;
        if (p == 0 || p->f20 == 0xFE || p->f20 == 0xFD) r = 1;
        return r;
    }
    return 1;
}

/* the entry's two faces are either unset or already gone */
s32 func_L08_00422A40(s32 a0)
{
    Ent *p;
    s32 r;

    if (a0 != -1) {
        p = D_L08_001B29DC + a0;
        r = 0;
        if (p == 0 || p->f20 == 0xFE || p->f20 == 0xFD) r = 1;
        return r;
    }
    return 1;
}

/* the entry is a live class or an unset one */
s32 func_L08_003D89D8(s32 a0)
{
    Ent *p;
    u32 b;
    s32 r;

    if (a0 != -1) {
        p = D_L08_001B29DC + a0;
        b = p->f20;
        r = 0;
        if (b >= 3u || p == 0 || b == 0xFE || b == 0xFD) r = 1;
        return r;
    }
    return 1;
}

/* the entry is a live class or an unset one */
s32 func_L08_00422A88(s32 a0)
{
    Ent *p;
    u32 b;
    s32 r;

    if (a0 != -1) {
        p = D_L08_001B29DC + a0;
        b = p->f20;
        r = 0;
        if (b >= 3u || p == 0 || b == 0xFE || b == 0xFD) r = 1;
        return r;
    }
    return 1;
}


extern volatile s32 D_001A7344[] __attribute__((sda));
extern void ref_L08_003F76C0_00(void *dest, s32 *position, void *table);
extern float ref_L08_003F76C0_01(void *work, void *table);
extern void ref_L08_003F76C0_02(s32 *position, s32 *x, s32 *y);
extern s32 ref_L08_003F76C0_03(s32 sentinel, u16 x, u16 y, s32 width, s32 rowBytes, s32 flags, float *out);
extern s32 D_001A7340;
extern float ref_L08_003F76C0_04 __attribute__((sda));
extern signed char ref_L08_003F76C0_05[];

typedef struct {
    s32 pad[4];
    s32 x;
    s32 y;
    s32 tail[2];
} QueryWork;

float func_L08_003F76C0(void *arg0) {
    QueryWork work;
    float temp_f20;
    s32 temp_s1;

    temp_s1 = *(s32 *)((signed char *)arg0 + 0x68) + 0x10;
    ref_L08_003F76C0_00(&work, (s32 *)temp_s1, ref_L08_003F76C0_05);
    temp_f20 = ref_L08_003F76C0_01(&work, ref_L08_003F76C0_05 + 0x230);
    ref_L08_003F76C0_02((s32 *)temp_s1, &work.x, &work.y);
    if (work.x < 0 ||
        work.x >= D_001A7340 - 0x10 ||
        work.y < 0 ||
        work.y >= D_001A7344[0] - 8 ||
        !(temp_f20 > 0.0f) ||
        ref_L08_003F76C0_03(-1, (u16)work.x, (u16)work.y, 0x10, 8, 0, &ref_L08_003F76C0_04) == -1) {
        ref_L08_003F76C0_04 = -1.0f;
    }
    return ref_L08_003F76C0_04;
}
