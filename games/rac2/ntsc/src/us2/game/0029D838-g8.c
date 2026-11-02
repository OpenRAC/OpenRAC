#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033DC30(void *, int);

int func_US2_0029D838(int value) {
    if (D_001A8CFC != 0)
        return func_0033DC30(D_001A8CFC + 0x3EB50, value);
    return 0;
}

extern void func_0033DCC8(void *);

void func_US2_0029D878(void) {
    if (D_001A8CFC != 0)
        func_0033DCC8(D_001A8CFC + 0x3EB50);
}

extern int func_003424C0(void *, int);

int func_US2_0029D8A8(int value) {
    if (D_001A8CFC != 0)
        return func_003424C0(D_001A8CFC + 0x3E760, value);
    return 0;
}

extern void func_00342758(void *);

void func_US2_0029D8E8(void) {
    if (D_001A8CFC != 0)
        func_00342758(D_001A8CFC + 0x3E760);
}

extern int func_00341EE8(void *, int);

int func_US2_0029D918(int value) {
    if (D_001A8CFC != 0)
        return func_00341EE8(D_001A8CFC + 0x3E9C8, value);
    return 0;
}
