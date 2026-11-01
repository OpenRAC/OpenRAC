#include "include_asm.h"

typedef int s32;

typedef unsigned int u32;

typedef unsigned char u8;

typedef unsigned short u16;

typedef float f32;

typedef int L04Quad __attribute__((mode(TI)));

/* the object the level's spawner allocates: a sixteen-byte block, a flag byte,
   a pointer to the second object and the caller's own pointer */
typedef struct {
    u8  pad00[0x10];
    L04Quad field10;
    u8  pad20[0x10];
    u8  field30;
    u8  pad31[0x68 - 0x31];
    u32 field68;
    u8  pad6C[0xB8 - 0x6C];
    u32 fieldB8;
} L04Object;

/* the second object: four sixteen-byte blocks and the words that follow them */
typedef struct {
    L04Quad field00;
    L04Quad field10;
    L04Quad field20;
    L04Quad field30;
    u32 field40;
    u32 field44;
    u32 field48;
    u32 field4C;
    u32 field50;
    u32 field54;
    u32 field58;
    f32 field5C;
} L04Target;

/* the level's own object: the blocks the spawner copies into */
typedef struct {
    u8  pad00[0x10];
    L04Quad field10;
    u8  pad20[0x11];
    u8  field31;
    u8  pad32[2];
    u16 field34;
    u8  pad36[0x98 - 0x36];
    u32 field98;
    u8  pad9C[0xC0 - 0x9C];
    u8  padC0[0x30];
    L04Quad fieldF0;
} L04Level;

extern void *func_0029EFD0(s32 size);
extern void func_002833F0(f32 *dst, f32 *src);
extern f32 func_00283CE0(s32 value);
extern void func_002A04B8(void *a, s32 *b, s32 *c, s32 *d);

/* Builds the level's spawn record: the caller's blocks are copied into the
   level's object and into the one the spawner allocated, and the colour and
   the scale are computed from what the engine's helpers return. */
void *func_L04_0030C4C0(void *a0, void *a1, void *a2, void *a3, void *a4, void *a5,
                       s32 a6, void *a7, u32 a8) {
    L04Object *p = (L04Object *)(u32)func_0029EFD0(0xE44);
    L04Level *lv = (L04Level *)a0;
    L04Target *q;
    u32 r, g, b;
    u32 t;

    if ((u32)p != 0) {
        lv->field10 = *(L04Quad *)a3;
        lv->fieldF0 = *(L04Quad *)a5;
        func_002833F0((f32 *)((u8 *)lv + 0xC0), (f32 *)((u8 *)lv + 0xF0));
        p->fieldB8 = (u32)a0;
        p->field30 = 0xFF;
        p->field10 = lv->field10;
        q = (L04Target *)(u32)p->field68;
        q->field48 = (u32)a0;
        q->field50 = lv->field34;
        q->field00 = *(L04Quad *)a1;
        q->field20 = *(L04Quad *)a3;
        q->field10 = *(L04Quad *)a2;
        q->field30 = *(L04Quad *)a4;
        q->field40 = (u32)a7;
        q->field44 = a8;
        q->field5C = 1.0f / func_00283CE0(a6);
        func_002A04B8(a0, (s32 *)&r, (s32 *)&g, (s32 *)&b);
        t = (g << 8) + 0x80000000;
        q->field54 = (b << 16) + t + r;
        lv->field98 = 0;
        lv->field31 = 0;
        lv->field34 = (u16)(lv->field34 | 0x103);
    }
    return (u32)p;
}
