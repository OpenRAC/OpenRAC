#include "include_asm.h"

extern void func_002FDBD8(void *, void *, int, float);

void func_PAL_00335D10(char *object)
{
    char *data = *(char **)(object + 0x68);
    unsigned int mode = *(unsigned char *)(object + 0x20);
    if (mode - 2 < 2 && *(int *)(data + 0x40) != 0)
        func_002FDBD8(data + 0x10, data + 0x20, 0, 1.0f);
}

