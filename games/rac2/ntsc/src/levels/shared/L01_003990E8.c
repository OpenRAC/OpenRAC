/* Clears the visible flag of an object and of its three linked parts, and copies each
   present part's 64-bit word at 0x38 back into the object. */
#include "common.h"

void func_L01_003990E8(char *a3)
{
    int i;
    char **p;
    char *q;

    q = *(char **)(a3 + 0x24);
    *(unsigned int *)(a3 + 0x98) = *(unsigned int *)(q + 0x10);
    *(unsigned short *)(a3 + 0x34) &= 0xFFFE;
    p = (char **)(*(char **)(a3 + 0x68) + 0x14);
    *(unsigned char *)(a3 + 0x31) = 1;
    for (i = 0; i < 3; i++) {
        if (*p != 0) {
            *(unsigned short *)(*p + 0x34) &= 0xFFFE;
            *(unsigned char *)(*p + 0x31) = 1;
            *(unsigned short *)(*p + 0x34) &= 0xFF7F;
            *(long long *)(a3 + 0x38) = *(long long *)(*p + 0x38);
        }
        p++;
    }
}
