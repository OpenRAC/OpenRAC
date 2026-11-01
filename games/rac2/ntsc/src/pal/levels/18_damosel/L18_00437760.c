#include "include_asm.h"

typedef unsigned char u8;
typedef short s16;
typedef int s32;
extern u8 D_001C97C0[];
extern u8 *D_L18_001B281C[] __attribute__((sda));
extern s32 D_L18_001B2838[] __attribute__((sda));
extern s32 D_001A79F0 __attribute__((sda));
extern u8 D_0019B2E8[];
extern s32 func_002ACBC8(u8 *, s32);
extern s32 func_002761F8(void *arg0, void *arg1, s32 arg2, s32 arg3, s32 arg4);
extern u8 *D_L18_001B281C[] __attribute__((sda));
extern s32 D_L18_001B2838[] __attribute__((sda));
extern u8 D_00190000[];

s32 func_PAL_L18_00437760(s32 a0) {
    u8 *v1 = (u8 *)*(s32 *)(a0 + 0x68);
    s32 r = 0;
    if (*(s16 *)(v1 + 0x7FC) != 0) {
        if (*(s16 *)(v1 + 0x804) < 0x7E) goto done;
    }
    r = 1;
done:
    return r;
}

