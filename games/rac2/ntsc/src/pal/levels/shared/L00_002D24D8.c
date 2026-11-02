#include "include_asm.h"

extern char D_00189E20_2[];
extern void func_002A7470(void *, int, int, int);

/* Routes the object the state word names to the mode the helper reports: the helper's value
   and the caller's index are handed over with the caller's float as an int, otherwise the
   object's own flag picks the fixed mode 7. Same two gates as its sibling below. */
void func_PAL_L00_002D24D8(int a0, int a1, float f12)
{
    char *base = D_00189E20_2;
    int out;

    if (*(int *)(base + 0x2494) == 0)
        return;
    if (*(int *)(base + 0x2498) == 0)
        return;
    if (func_L00_002D1F78(a0, &out) != 0) {
        func_002A7470(*(void **)(base + 0x2494), out, a1, (int)f12);
    } else if (*(unsigned char *)(*(char **)(base + 0x2494) + 0x43) != 0) {
        func_002A7470(*(void **)(base + 0x2494), 0, 0, 6);
    }
}

