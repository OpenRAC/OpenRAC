#include "common.h"
/* Adapted from RAC1's GPL-3.0 moby_ground_collision.c, get_next_moby_in_group. */
extern unsigned char *D_001B1D40 MACRO_ADDR;
extern short *D_001B1D38 MACRO_ADDR;
extern short D_001B1D3C;
extern int D_001B1B5C MACRO_ADDR;
extern int D_001B1B78 MACRO_ADDR;
extern int D_001EE5A0[];

/* Advances the group cursor to the next moby accepted by the state filter. */
int func_002A86E8(int *out, unsigned char *m, int includeInactive, int onlyInactive) {
    short *p;
    int neg;

    *out = 0;
    if (m != D_001B1D40) {
        if (D_001B1B78 < m[0x21]) {
            return -1;
        }
        D_001B1D40 = 0;
        D_001B1D38 = (short *)D_001EE5A0[m[0x21]];
        if (D_001B1D38 == 0) {
            return -1;
        }
        D_001B1D38--;
        {
            short k;

            do {
                D_001B1D38++;
                p = D_001B1D38;
                D_001B1D3C = *(unsigned short *)p & 0x7FFF;
                D_001B1D40 = (unsigned char *)(D_001B1B5C + (D_001B1D3C << 8));
                k = *p;
            } while (k >= 0 && m != D_001B1D40);
            if (k < 0) {
                return -1;
            }
        }
    } else {
        short n = *D_001B1D38;

        p = D_001B1D38;
        if (n < 0) {
            return -1;
        }
    }
    for (;;) {
        short n;

        D_001B1D38 = p + 1;
        D_001B1D3C = *(unsigned short *)D_001B1D38 & 0x7FFF;
        D_001B1D40 = (unsigned char *)(D_001B1B5C + (D_001B1D3C << 8));
        *out = (int)D_001B1D40;
        neg = (signed char)D_001B1D40[0x20] < 0;
        if ((includeInactive == 0 && onlyInactive == 0 && !neg) || (includeInactive != 0 && (onlyInactive == 0 || neg))) {
            return 0;
        }
        n = *D_001B1D38;
        p = D_001B1D38;
        if (n < 0) {
            break;
        }
    }
    *out = 0;
    return -1;
}
