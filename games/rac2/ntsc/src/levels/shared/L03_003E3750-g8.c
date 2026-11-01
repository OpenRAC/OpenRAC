/* func_L03_003E3750 (172 bytes, 8 levels): whether a1 is one of the targets the
   controller's current entry names: kinds 1 and 2 keep one target per step, kind 3 two
   fixed ones (only at step 0). */
#include "common.h"

extern s32 D_L03_001B0B50 __attribute__((sda));
extern s32 D_L03_0029BCD0[];

s32 func_L03_003E3750(char *a0, s32 a1)
{
    char *a2 = *(char **)(a0 + 0x68);
    s32 off;
    s32 kind;
    s32 i;

    if (*(s16 *)(a2 + 0x5E) == D_L03_001B0B50 - 1)
        return 0;
    off = *(s16 *)(a2 + 0x62) * 16;
    kind = *(s32 *)((char *)D_L03_0029BCD0 + off);
    switch (kind) {
    case 1:
    case 2:
        i = *(s16 *)(a2 + 0x64) + 1;
        if (a1 != *(s32 *)((i * 4 + off) + (char *)D_L03_0029BCD0))
            break;
        return 1;
    case 3:
        if (*(s16 *)(a2 + 0x64) != 0)
            return 0;
        if (a1 == *(s32 *)((char *)D_L03_0029BCD0 + off + 4))
            return 1;
        if (a1 == *(s32 *)((char *)D_L03_0029BCD0 + off + 8))
            return 1;
        break;
    }
    return 0;
}
