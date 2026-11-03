#include "include_asm.h"

#include "common.h"
extern void func_002A7750(void *, int, int);
extern void func_002A7470(void *, int, int, int);

void func_PAL_L02_003D5548(char *self) {
    char *p = *(char **)(self + 0x68);
    if (*(int *)(p + 0x2E4) != 0) {
        func_002A7750(self, 4, 2);
        if (*(unsigned char *)(self + 0x43) != 0)
            func_002A7470(self, 0, 0, 0x8);
    } else {
        func_002A7750(self, 5, 2);
        if (*(unsigned char *)(self + 0x43) != 1)
            func_002A7470(self, 1, 0, 0x8);
    }
}

