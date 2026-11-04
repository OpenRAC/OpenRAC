#include "include_asm.h"
typedef unsigned char u8;
typedef struct {
    u8 pad0[0x242C];
    int slot[9];
    int want[9];
} Owner_2B9BB0;
extern Owner_2B9BB0 D_00189E20;
extern char D_00188660[];
extern int func_002E5CF8(int index, int mode, void *object);

void func_L00_002B9BB0(int index, int wanted, int handle)
{
    char *record = D_00188660 + D_00189E20.slot[index] * 0x70;

    if (*(int *)(record + 0x88) != wanted || *(unsigned char *)(record + 0x74) == 0)
        D_00189E20.slot[index] = func_002E5CF8(handle, 4, (void *)wanted);
    D_00189E20.want[index] = wanted;
}
