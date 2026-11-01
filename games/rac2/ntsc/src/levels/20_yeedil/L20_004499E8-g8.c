/* Initializes the movement state, full position and sixteen-byte zeroed vector. */
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern float D_L20_001B23E8 __attribute__((sda));

extern void func_002A7750(char *, int, int);

void func_L20_004499E8(char *arg0, char *arg1, char *arg2)
{
    char *state = *(char **)(arg0 + 0x68);

    *(s32 *)(state + 0xA8) = (s32)(D_L20_001B23E8 * 60.0f);
    *(char **)(state + 0xA4) = arg1;
    *(s32 *)(state + 0xAC) = 0;
    *(u16 *)(arg0 + 0x34) = (*(u16 *)(arg0 + 0x34) & 0xFFBE) | 0x1000;
    *(s32 *)(arg0 + 0x98) = *(s32 *)(*(char **)(arg0 + 0x24) + 0x10);
    qcopy((arg0 + 0x10), arg2);
    *(f32 *)(arg0 + 0x18) = *(f32 *)(arg0 + 0x18) + 0.5f;
    qzero((state + 0x50));
    func_002A7750(arg0, 2, -1);
}
