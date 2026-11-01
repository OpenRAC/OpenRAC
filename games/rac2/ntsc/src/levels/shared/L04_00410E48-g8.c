#include "common.h"

/* A -G8 unit (the profile from the file's name): a plain extern of at most eight bytes is then
   one assembler operand, and with its .extern only at the end of the file the assembler
   expands it to the retail's absolute lui + load through the destination register. -G0
   splits the address into a high-part register of its own. */

extern s32 D_001A79F0;

/* shared native body of family 8c88be3bc489ab9e */
int func_L04_00410E48(int a0, int a1, float f12)
{
    int a2 = 0;

    if (D_001A79F0 != 11) goto end;

    if (a0 == -1) {
        if (a1 >= 166) {
            if (a1 < 187) {
                if (f12 < 520.2f) {
                    a2 = 1;
                    goto end;
                }
            }
        }
    }

    if (a0 == 2) {
        if ((unsigned)(a1 - 1) < 6) {
            if (f12 < 537.0f) {
                a2 = 1;
                goto end;
            }
        }
        if ((unsigned)(a1 - 35) < 13) {
            if (f12 < 534.0f) {
                a2 = 1;
                goto end;
            }
        }
    }

end:
    return a2;
}
