/* EXACT func_0023E068 -- src/game/movie/movie.c
 * Matched: 2026-10-03, EXACT (masked).
 * Calls func_0023DBE0 with computed arguments.
 */
#include "common.h"

extern void func_0023DBE0(int, int, int, int, int, int);
extern int D_0016130C MACRO_ADDR;

void func_0023E068(char *a, int b, int c, int d, int e, int f) {
    int v = D_0016130C + 0xD9090;
    int g = e - *(int *)(d + 0x48);
    func_0023DBE0(v, b, c, d, f, g);
}
