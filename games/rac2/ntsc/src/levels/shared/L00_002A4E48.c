#include "include_asm.h"
typedef unsigned char u8;
typedef short s16;
extern char D_00189E20[];
extern u8 D_00139568[];
extern char D_L00_00262570[];
extern int func_L00_002ADEA0(int);
typedef struct {
    u8 pad[0x28];
    int v;
} WeaponEntry;

int func_L00_002A4E48(char *a0)
{
    char *s0 = D_00189E20;

    if (*(u8 *)(s0 + 0x22B4) == 1)
        return 0;
    if (a0 != 0 && *(u8 *)(s0 + 0x22BA) != 0 && *(s16 *)(s0 + 0x24EC) == 0) {
        int idx = func_L00_002ADEA0(0);
        return ((WeaponEntry *)(D_L00_00262570 + D_00139568[idx] * 224))->v;
    }
    a0 = D_00189E20;
    if (*(int *)(a0 + 0x24CC) != 1)
        return *(u8 *)(a0 + 0x1492) ? 0x67 : 0;
    return 0x51;
}
