#include "common.h"

/* This unit is built with -G8: func_L20_00409480 needs the -G8 profile. Its register
   allocation differs from the -G0 one this cc1 makes (in a -G0 unit the address of
   D_L20_001FBB20[arg0] lands in $a1 instead of $v1, and the loads of D_L20_001B2FB8
   come out split instead of absolute). */

extern char *D_L20_001FBB20[];

extern int D_L20_001B2FB8;

extern char *D_L20_001B2F9C;

extern void func_L09_003F73E8(char *);

extern void func_L09_003F77D0(char *);

extern void func_0029F338(void *);

void func_L20_00409480(s32 arg0) {
    char *s1 = D_L20_001FBB20[arg0];
    s32 s4 = arg0 & 0xFF;
    char *s0;

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
        s0 = D_L20_001B2F9C + ((*(u16 *)s1 & 0x7FFF) << 8);

        if (s0 == 0) {
            goto end;
        }
        if (*(s16 *)(s0 + 0xAA) == 0xD0F) {
            if (*(u8 *)(s0 + 0x20) != 0xFE) {
                if (*(u8 *)(s0 + 0x20) != 0xFD) {
                    *(int *)(*(char **)(s0 + 0x68) + 0x20) = 0;
                    func_L09_003F73E8(s0);
                    func_L09_003F77D0(s0);
                    func_0029F338(s0);
                }
            }
        }
        s1 += 2;
        if ((arg0 & 0xFF) == 0xFF) {
            goto end;
        }
        if (D_L20_001B2FB8 < arg0) {
            goto end;
        }
        if (s1 == 0) {
            return;
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
    return;
}
