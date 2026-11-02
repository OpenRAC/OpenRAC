#include "include_asm.h"

extern int D_001A79F8;
extern int D_001B1948;
extern int D_001B194C;
extern int D_001B1950;
extern int D_001B1954;
extern int D_001B1958;
extern int D_001B1928[];
extern void func_0029CBA8(int value);

/* Initialize the pair of handles, then clear both sixteen-byte entries. */
void func_0028A278(void) {
    int v = D_001A79F8;
    D_001B1948 = v;
    D_001B194C = v;
    D_001B1950 = 0;
    func_0029CBA8((int)&D_001B1948);
    D_001B1958 = 0;
    D_001B1954 = -1;
    {
        int *p = D_001B1928;
        int i = 1;
        do {
            *(unsigned short *)((char *)p + 0xE) = 0;
            *p = 0;
            p += 4;
            i--;
        } while (i >= 0);
    }
}
