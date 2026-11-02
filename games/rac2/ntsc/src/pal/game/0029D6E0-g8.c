#include "include_asm.h"
extern char *D_001A8CFC;
extern int D_001A8C64;

void func_PAL_0029D7D0(void)
{
    extern int D_001A8C64;
    extern void func_0028E138(int);
    extern void func_0029BCD8(void);
    if (D_001A8CFC != 0) {
        if (D_001A8C64 == 0) {
            func_0028E138(1);
            func_0029BCD8();
        }
    }
}
