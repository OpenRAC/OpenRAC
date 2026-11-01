#include "include_asm.h"

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;
typedef int TI128 __attribute__((mode(TI)));

extern s32 *D_L12_001FEAC0[];

typedef struct {
    char pad0[0x10];
    TI128 f10;
} Todano_Pos;

typedef struct {
    char pad0[0x1CA];
    s16 f1CA;
    char pad1[0x200 - 0x1CC];
    Todano_Pos *f200;
    char pad2[0x2DC - 0x204];
    s32 f2DC;
    char pad3[0x30F - 0x2E0];
    u8 f30F;
} Todano_Owned;

typedef struct {
    char pad0[0x24];
    char *f24;
} Todano_Hit;

typedef struct {
    char pad0[0x68];
    Todano_Owned *f68;
} Todano_Obj;

extern char D_L12_001B1860[];
extern char D_L12_001C0258[];
extern s32 func_002761F8(void *, void *, int, void *, int);
extern s32 func_00277258(void *, int, void *, float);
extern void func_0026FEB8(void *);

s32 func_L12_00414AB8(Todano_Obj *a0, s32 a1, s32 a2, s32 a3)
{
    Todano_Owned *o = a0->f68;
    TI128 pos[1];
    TI128 aim[1];
    TI128 probe[1];
    s32 blocked;

    if (o->f30F != 0)
        return 1;
    pos[0] = *(TI128 *)((char *)a0 + 0x10);
    if (a2 != 0 && o->f2DC != -1 && o->f1CA != -1)
        pos[0] = *(TI128 *)((char *)D_L12_001FEAC0[o->f2DC] + o->f1CA * 16 + 0x10);
    if (a3 != 0)
        ((float *)pos)[2] += 1.0f;
    else
        ((float *)pos)[2] += 1.76f;
    aim[0] = o->f200->f10;
    blocked = 0;
    ((float *)aim)[2] += 1.18f;
    if (func_002761F8(pos, aim, 0, a0, 0) == 0 || *(Todano_Hit **)D_L12_001C0258 == (Todano_Hit *)o->f200)
        blocked = 1;
    func_0026FEB8(D_L12_001B1860);
    if (a1 != 0 && blocked == 0) {
        probe[0] = *(TI128 *)(D_L12_001C0258 + 8);
        if (func_00277258(probe, 1, a0, 0.5f) != 0) {
            Todano_Hit *h = *(Todano_Hit **)D_L12_001C0258;
            if (h == 0 || h->f24 == 0 || *(s16 *)(h->f24 + 0x46) != 5)
                blocked = 1;
        }
    }
    return blocked;
}
