#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_003478F0(void *);

void func_PAL_0029CB40(void) {
    if (D_001A8CFC != 0)
        func_003478F0(D_001A8CFC + 0x39620);
}

extern void func_003465D0(void *);

void func_PAL_0029CB70(void) {
    if (D_001A8CFC != 0)
        func_003465D0(D_001A8CFC + 0x39BA0);
}

extern int func_00346C60(void *, int);

int func_PAL_0029CBA0(int value) {
    if (D_001A8CFC != 0)
        return func_00346C60(D_001A8CFC + 0x39620, value);
    return 0;
}

extern int func_00345F70(void *, int, int);

int func_PAL_0029CBE0(int first, int second)
{
    if (D_001A8CFC != 0)
        return func_00345F70(D_001A8CFC + 0x39BA0, first, second);
    return 0;
}

extern int func_00339320(void *, int);

int func_PAL_0029CC28(int value) {
    if (D_001A8CFC != 0)
        return func_00339320(D_001A8CFC + 0x3B4C8, value);
    return 0;
}

extern void func_00339590(void *);

void func_PAL_0029CC68(void) {
    if (D_001A8CFC != 0)
        func_00339590(D_001A8CFC + 0x3B4C8);
}
