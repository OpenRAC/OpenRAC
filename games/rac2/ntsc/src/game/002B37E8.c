#include "include_asm.h"
#include "common.h"

extern void func_002A0F48(char *);

void func_002B37E8(char *obj, char *src)
{
    *(unsigned char *)(obj + 0x30) = 0xFF;
    *(short *)(obj + 0x32) = 0xFF;
    *(unsigned char *)(obj + 0x31) = 1;
    if (src != 0) {
        *(char **)(obj + 0xB8) = src;
        qcopy(obj + 0x10, src + 0x10);
        qcopy(obj + 0xF0, src + 0xF0);
        *(long long *)(obj + 0x38) = *(long long *)(src + 0x38);
        func_002A0F48(obj);
    }
    *(unsigned short *)(obj + 0x34) &= 0xFFF8;
}
