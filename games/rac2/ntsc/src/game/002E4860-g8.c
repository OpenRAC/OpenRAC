#include "common.h"

/* A -G8 unit (the profile from the file's name): a plain extern of at most eight bytes is then
   one assembler operand, and with its .extern only at the end of the file the assembler
   expands it to the retail's absolute lui + load through the destination register. -G0
   splits the address into a high-part register of its own. */

extern s32 D_001A7B9C;
extern s32 D_001A7BA0;
extern u8 D_00188660[];

void func_002E4860(void)
{
    struct Layout { char reserved[0x48]; int a,b,c,d,e,f; };
    struct Layout *layout = (struct Layout *)D_00188660;
    int width = D_001A7BA0;
    layout->a = width * 7 / 10;
    layout->b = D_001A7B9C;
    layout->c = width;
    layout->d = width * 6 / 10;
    layout->e = 614;
    layout->f = width * 25 / 32;
}
