#include "include_asm.h"

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;
typedef int TI128 __attribute__((mode(TI)));

extern s32 *D_L09_001FE6C0[];

typedef struct {
    char pad0[0x10];
    TI128 f10;
} T3F9568_Pos;

typedef struct {
    char pad0[0x1CA];
    s16 f1CA;
    char pad1[0x200 - 0x1CC];
    T3F9568_Pos *f200;
    char pad2[0x2DC - 0x204];
    s32 f2DC;
    char pad3[0x30F - 0x2E0];
    u8 f30F;
} T3F9568_Owned;

typedef struct {
    char pad0[0x24];
    char *f24;
} T3F9568_Hit;

typedef struct {
    char pad0[0x68];
    T3F9568_Owned *f68;
} T3F9568_Obj;

extern char D_L09_001BFE58[];
extern s32 func_002761F8(void *, void *, int, void *, int);
extern s32 func_00277258(void *, int, void *, float);

s32 func_L09_003F9568(T3F9568_Obj *a0, s32 a1, s32 a2, s32 a3)
{
    T3F9568_Owned *o = a0->f68;
    TI128 pos[1];
    TI128 aim[1];
    TI128 probe[1];
    s32 blocked;

    if (o->f30F != 0)
        return 1;
    pos[0] = *(TI128 *)((char *)a0 + 0x10);
    if (a2 != 0 && o->f2DC != -1 && o->f1CA != -1)
        pos[0] = *(TI128 *)((char *)D_L09_001FE6C0[o->f2DC] + o->f1CA * 16 + 0x10);
    if (a3 != 0)
        ((float *)pos)[2] += 1.0f;
    else
        ((float *)pos)[2] += 1.76f;
    aim[0] = o->f200->f10;
    blocked = 0;
    ((float *)aim)[2] += 1.18f;
    if (func_002761F8(pos, aim, 0, a0, 0) == 0 || *(T3F9568_Hit **)D_L09_001BFE58 == (T3F9568_Hit *)o->f200)
        blocked = 1;
    if (a1 != 0 && blocked == 0) {
        probe[0] = *(TI128 *)(D_L09_001BFE58 + 8);
        if (func_00277258(probe, 1, a0, 0.5f) != 0) {
            T3F9568_Hit *h = *(T3F9568_Hit **)D_L09_001BFE58;
            if (h == 0 || h->f24 == 0 || *(s16 *)(h->f24 + 0x46) != 5)
                blocked = 1;
        }
    }
    return blocked;
}
