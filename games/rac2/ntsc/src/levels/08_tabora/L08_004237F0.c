#include "include_asm.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef int s32;
typedef float f32;

typedef struct { u8 pad[0x100]; } Ent;

extern int func_2F9A90();
extern s32 func_327898(s32, s32);
extern void func_329EC0(Ent *, f32);

void func_L08_004237F0(char *a0) {
    char *s1 = a0;
    char *s0 = *(char **)(s1 + 0x68);

    if (*(u8 *)(s1 + 0xBC) != 0) {
        *(u8 *)(s1 + 0xBC) = 0;
        if (*(s16 *)(s0 + 0x1BA) < 0xC8) {
            *(s16 *)(s0 + 0x1BA) = (s16)func_327898(0x12C, 0x168);
        }
    }
    if (*(s32 *)(s0 + 0x144) == 2) {
        func_2F9A90(s0 + 0x1BA);
    }
    if (*(u16 *)(s1 + 0x96) < 0x7FFD) {
        *(u16 *)(s1 + 0x96) += 1;
    }
    func_2F9A90(s0 + 0x1B8);
    func_2F9A90(s0 + 0x1BC);
    if (*(u8 *)(s1 + 0x20) >= 2) {
        func_329EC0((Ent *)s1, *(f32 *)(s0 + 0x1B0));
    }
}

