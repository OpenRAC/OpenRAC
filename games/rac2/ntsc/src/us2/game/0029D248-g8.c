#include "include_asm.h"
extern char *D_001A8CFC;
extern int func_0033EA60(void *, int);

int func_US2_0029D248(int value) {
    if (D_001A8CFC != 0)
        return func_0033EA60(D_001A8CFC + 0x3BA58, value);
    return 0;
}
