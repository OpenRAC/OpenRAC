#include "include_asm.h"

typedef int s32;

typedef unsigned int u32;

typedef unsigned char u8;

typedef unsigned short u16;

typedef float f32;

/* the object the level's spawner builds: a 0x67-byte record with a few flag bytes,
   a halfword, a float and a sixteen-byte block copied from the caller */
typedef int L04Quad __attribute__((mode(TI)));

typedef struct {
    u8 pad00;
    u8 field01;
    u8 field02;
    u8 field03;
    u32 field04;
    u8 field08;
    u8 field09;
    u16 field0A;
    f32 field0C;
    L04Quad field10;
} L04Spawned;

extern void *func_002B9030(s32 kind);

extern s32 func_00283CF0(f32 value);

extern s32 func_002A77E0(s32 limit);

/* the level's pointer to the byte the record carries in its second flag byte */
extern u32 ref_L04_00363CA0_00;

void *func_L04_00363CA0(void *a0, void *a1, f32 f0) {
    L04Spawned *p = (L04Spawned *)(u32)func_002B9030(0x67);

    if ((u32)p != 0) {
        p->field04 = (u32)a1;
        p->field09 = (u8)(func_00283CF0(2.0f) - 0x10);
        p->field03 = 0x48;
        p->field01 = 0;
        p->field02 = *(u8 *)ref_L04_00363CA0_00;
        p->field0C = f0;
        p->field08 = (u8)func_002A77E0(0xFF);
        p->field0A = 2;
        p->field10 = *(L04Quad *)a0;
    }
    return (u32)p;
}
