#include "include_asm.h"
typedef unsigned char u8;
typedef short s16;
extern char D_00189E20[];
extern u8 D_00139568[];
extern char D_L03_00266DA0[];
extern int func_L00_002ADEA0(int);
typedef struct {
    u8 pad[0x28];
    int v;
} WeaponEntry;

/* func_L00_002A4E48 without the 0x1492 fallback: 0x51 or 0. */
int func_L03_002AB5C8(char *a0)
{
    char *s0 = D_00189E20;

    if (*(u8 *)(s0 + 0x22B4) == 1)
        return 0;
    if (a0 != 0 && *(u8 *)(s0 + 0x22BA) != 0 && *(s16 *)(s0 + 0x24EC) == 0) {
        int idx = func_L00_002ADEA0(0);
        return ((WeaponEntry *)(D_L03_00266DA0 + D_00139568[idx] * 224))->v;
    }
    {
        char *p = D_00189E20;
        if (*(int *)(p + 0x24CC) != 1)
            return 0;
    }
    return 0x51;
}
