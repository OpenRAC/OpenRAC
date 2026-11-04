#include "include_asm.h"

extern char *D_001A8CFC[] __attribute__((sda));
extern char *D_001A8CFC[] __attribute__((sda));
extern char *D_001A8CFC[] __attribute__((sda));
extern void *func_00341558(void *);

int func_US2_002D5EC8(char *object) {
    if (D_001A8CFC[0] != 0)
        *(void **)(object + 0x34) = func_00341558(D_001A8CFC[0] + 0x3C160);
    return 0;
}

