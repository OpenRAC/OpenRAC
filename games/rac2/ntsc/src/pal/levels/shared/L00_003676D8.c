#include "include_asm.h"

extern void func_002B7B18(int value, char *target);
extern void func_002B7CE0(int value);
extern int D_L00_001BCAC0[];
extern int D_0014B540[];

/* Same body as func_L00_003046D0: compares two words of a 332-byte-stride table and calls the
 * two helpers when the entry ahead is greater; returns 1. */
int func_PAL_L00_003676D8(int index)
{
    int j = index + 1;
    int *d = D_L00_001BCAC0;
    int *b = D_0014B540;
    int *t = (int *)((char *)b + 16040);
    int n = d[12] * 332;
    int hold = d[28];
    int j4 = j * 4;
    int cur = *(int *)((char *)t + (n + index * 4));
    int next = *(int *)((char *)t + (j4 + n));

    n = next - cur;
    if (n > 0) {
        func_002B7B18(hold, (char *)(cur + b[3977]));
        func_002B7CE0(0);
    }
    return 1;
}

