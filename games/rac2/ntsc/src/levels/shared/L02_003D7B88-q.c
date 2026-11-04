#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern char *D_001FEAC0[];
extern char *D_001B359C MACRO_ADDR;
extern void func_002A7750(void *, int, int);

/* when the object is idle, retargets it at its state's table entry: takes the slot's limit
   and the entry's position, restarts the timer and switches to state 1 */
void func_L02_003D7B88(char *object)
{
    char *q = *(char **)(object + 0x68);
    char *e;

    if (*(u8 *)(object + 0x20) != 0)
        return;
    *(u8 *)(object + 0x30) = 0x80;
    *(f32 *)(q + 0xC) = -1.0f;
    *(s32 *)(q + 0x8) = *(s32 *)(D_001B359C + (*(s32 *)q << 8) + 0x68);
    e = D_001FEAC0[*(s32 *)(q + 4)];
    *(char **)(q + 0x18) = e;
    *(s32 *)(q + 0x14) = 0;
    *(TI128 *)(object + 0x10) = *(TI128 *)(e + 0x10);
    func_002A7750(object, 1, -1);
}
