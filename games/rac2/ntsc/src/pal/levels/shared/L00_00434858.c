#include "include_asm.h"

extern void func_0033AE18(unsigned char *owner, int value);

/* Forward owner+8 and the supplied signed word to the measured two-argument setter. */
void func_PAL_L00_00434858(unsigned char *owner, int value)
{
    func_0033AE18(owner + 8, value);
}

