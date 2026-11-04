/* func_L00_003CA650 in its own unit (flags in config/cflags.txt: -O2 -G0 -ffunction-sections). */
#include "include_asm.h"

extern void func_L00_003CA688(char *);

void func_L00_003CA650(char *object)
{
    if (*(unsigned char *)(object + 0x20) == 1)
        func_L00_003CA688(object);
}
