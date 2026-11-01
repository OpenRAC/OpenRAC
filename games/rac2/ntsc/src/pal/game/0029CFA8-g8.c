#include "include_asm.h"
extern char *D_001A8CFC;
extern void func_0033E620(void *);

void func_PAL_0029CFA8(void) {
    if (D_001A8CFC != 0)
        func_0033E620(D_001A8CFC + 0x3C898);
}
