#include "include_asm.h"

extern unsigned char D_001A7A47;
extern void func_0029F978(unsigned char *, unsigned int, unsigned char *);
extern void func_0029F9F0(char *, char *);

/* The initializer receives all three unchanged integer arguments from this caller. */
void func_L06_003660F0(unsigned char *object, unsigned int index,
                       unsigned char *node, float value)
{
    if (D_001A7A47 != 0) {
        if (node[1] == 0)
            func_0029F978(object, index, node);
        *(float *)(node + 0x20) = value;
        *(float *)(node + 0x24) = value;
        *(float *)(node + 0x28) = value;
    } else if (node[1] != 0) {
        func_0029F9F0((char *)object, (char *)node);
    }
}
