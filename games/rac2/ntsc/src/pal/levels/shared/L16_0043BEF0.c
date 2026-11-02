#include "include_asm.h"

extern void func_L16_00435CE8(void);

/* runs the level's one-shot start-up; the caller uses this only for its side effects */
int func_PAL_L16_0043BEF0(void)
{
    func_L16_00435CE8();
    return 0;
}

