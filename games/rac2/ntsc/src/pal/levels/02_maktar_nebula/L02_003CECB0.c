#include "include_asm.h"

#include "common.h"
extern int func_L02_0032E998(char *, int, float *, char *, short, char *, char *, float, float);
extern int func_002B50E0(char *, int, float);

int func_PAL_L02_003CECB0(char *self, int arg1) {
    float out;
    char *p = *(char **)(self + 0x68);
    int r = func_L02_0032E998(self + 0x10, arg1, &out, p + 0x290, *(short *)(p + 0x3B6),
                              p + 0x3B8, p + 0x3BC, 0.0f, *(float *)(p + 0x50));
    func_002B50E0(self, arg1, out);
    return r;
}

