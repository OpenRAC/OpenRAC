/* Level 07 moby helper: tests whether an id matches the first moby of a group. */
#include "types.h"
#include "asm.h"

/* Test whether the current moby identifier matches the requested one. */
/* Ported from rac1-decomp (src/overlays/l07/mobyutil_00282EB0.c: func_L07_00282EB0), where it is exact; names translated to the US level program. */

extern void get_first_moby_in_group(int *, int, int, int) __asm__("FUN_L00_002591d0");

/* Returns whether the current id equals id; the lookup arguments pass straight through from the caller. */
int matches_first_moby_in_group(int id, int idx, int a, int b) __asm__("FUN_L07_00281ef8");

int matches_first_moby_in_group(int id, int idx, int a, int b) {
    int current;
    get_first_moby_in_group(&current, idx, a, b);
    return current == id;
}
