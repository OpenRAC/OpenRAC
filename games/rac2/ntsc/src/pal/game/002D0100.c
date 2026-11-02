#include "include_asm.h"

extern char *D_001A8CFC[] __attribute__((sda));
extern char *D_001A8CFC[] __attribute__((sda));
extern void *func_00341558(void *);
extern char *D_001A8CFC[] __attribute__((sda));

int func_PAL_002D0100(char *object) {
    if (D_001A8CFC[0] != 0)
        *(void **)(object + 0x34) = func_00341558(D_001A8CFC[0] + 0x3C210);
    return 0;
}

