#include "include_asm.h"
typedef struct {
    char reserved00[0x1C0];
    int message;
    int flags;
} MenuGameState;
typedef struct {
    char reserved00[0x14];
    char *source;
    int value;
    char reserved1c[0x118];
    int fallback;
} CallbackState;
extern MenuGameState D_00138180;
extern CallbackState D_001F2840;
extern void func_0029C5D0(int);

int func_US2_002D43B0(void)
{
    int flags = D_00138180.flags;
    int result = 0;
    if (flags & 0x10) {
        int value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            value = -1;
            goto return_value;
        }
        value = 0;
return_value:
        return value;
    }
    if (flags & 0x900)
        result = 1;
    else
        func_0029C5D0(D_00138180.message);
    return result;
}
