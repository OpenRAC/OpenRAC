#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_003424C0(void *, int);

int func_PAL_0029D408(int value) {
    if (D_001A8CFC != 0)
        return func_003424C0(D_001A8CFC + 0x3E810, value);
    return 0;
}

extern void func_00342758(void *);

void func_PAL_0029D448(void) {
    if (D_001A8CFC != 0)
        func_00342758(D_001A8CFC + 0x3E810);
}

extern int func_00341EE8(void *, int);

int func_PAL_0029D478(int value) {
    if (D_001A8CFC != 0)
        return func_00341EE8(D_001A8CFC + 0x3EA78, value);
    return 0;
}

extern void func_00341F38(void *);

void func_PAL_0029D4B8(void) {
    if (D_001A8CFC != 0)
        func_00341F38(D_001A8CFC + 0x3EA78);
}

extern void func_0028A2F8(void);

void func_PAL_0029D4F0(void) {
    if (D_001A8CFC != 0)
        func_0028A2F8();
}

extern void func_0028A670(void);

void func_PAL_0029D518(void) {
    if (D_001A8CFC != 0)
        func_0028A670();
}
