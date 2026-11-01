/* func_L08_0036EFC8 (100 bytes; levels 08, 19): sets the low two flag bits of the current
   object and, when there is one, of every object in the null-terminated list, and clears
   each one's class word. The counterpart of func_L08_0036F030. */
#include "common.h"

extern u8 *D_L08_00152C00;
extern u8 *D_L08_00221AE0[];

void func_L08_0036EFC8(void)
{
    s32 i;

    if (D_L08_00152C00 != 0) {
        *(u16 *)(D_L08_00152C00 + 0x34) |= 3;
        *(s32 *)(D_L08_00152C00 + 0x98) = 0;
        for (i = 0; D_L08_00221AE0[i] != 0; i++) {
            *(u16 *)(D_L08_00221AE0[i] + 0x34) |= 3;
            *(s32 *)(D_L08_00221AE0[i] + 0x98) = 0;
        }
    }
}
