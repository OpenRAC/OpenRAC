#include "common.h"

/* This unit is built with -G8: func_L20_0039E0A0 needs the -G8 profile. Its register
   allocation differs from the -G0 one this cc1 makes: in a -G0 unit the address of
   D_L20_001FBB20[arg0] lands in $t0 instead of $v1 (the retail coalesces it with the
   shift), the loads of D_L20_001B2FB8 come out split instead of absolute. */

extern char *D_L20_001FBB20[];

extern int D_L20_001B2FB8;

extern char *D_L20_001B2F9C;

s32 func_L20_0039E0A0(s32 arg0) {
    char *s1 = D_L20_001FBB20[arg0];
    s32 s4 = arg0 & 0xFF;
    s32 count = 0;

    if (s4 == 0xFF) {
        goto end;
    }
    if (D_L20_001B2FB8 < arg0) {
        goto end;
    }
    if (s1 == 0) {
        goto end;
    }
    for (;;) {
        char *s0 = D_L20_001B2F9C + ((*(u16 *)s1 & 0x7FFF) << 8);

        if (s0 == 0) {
            goto end;
        }
        if (*(u8 *)(s0 + 0x20) != 6) {
            count++;
        }
        s1 += 2;
        if ((arg0 & 0xFF) == 0xFF) {
            goto end;
        }
        if (D_L20_001B2FB8 < arg0) {
            goto end;
        }
        if (s1 == 0) {
            goto end;
        }
        if (*(s16 *)(s1 - 2) >= 0) {
            continue;
        }
        if (s1 == D_L20_001FBB20[arg0]) {
            continue;
        }
        break;
    }
end:
    return count;
}
