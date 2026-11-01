#include "include_asm.h"

/* func_002D0F00: a -G8 unit, so a word load of a global gives the base and the value one
 * register as retail does. The loop pointers are set up in the order whose schedule puts
 * the constant 1 before the destination, as retail has it. The clear address is an
 * integer sum so its operands come out in retail's order. */

extern char *D_001A8CFC;

extern int D_001F2848[];

extern short D_0026185A[];

extern unsigned int D_00139768[];

extern unsigned char D_001A7A0A;

int func_002D0F00(void)
{
    unsigned int *mask;
    unsigned char flag;
    char *table;
    char *end;
    int clr;
    char *src;
    char *dst;
    int off;
    int one;

    if (D_001A8CFC == 0)
        goto done;
    D_001F2848[0] = 1;
    table = (char *)D_0026185A;
    mask = D_00139768;
    flag = D_001A7A0A;
    clr = (int)table + 2;
    one = 1;
    dst = table + 8;
    src = table + 6;
    end = table + 0xC6;
    off = 6;
    do {
        int v;
        if (flag != 0) {
            *(short *)dst = one;
        } else {
            v = *(short *)src;
            if (mask[(unsigned int)v >> 2] & (one << (v & 31)))
                *(short *)dst = one;
            else
                *(short *)(off + clr) = 0;
        }
        src += 6;
        dst += 6;
        off += 6;
    } while ((int)src < (int)end);
done:
    return 0;
}
