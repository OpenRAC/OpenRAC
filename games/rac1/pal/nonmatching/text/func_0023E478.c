/* EXACT func_0023E478 -- src/game/movie/movie.c
 * Matched: 2026-10-03, EXACT (masked).
 * Calls func_0023BB40 and func_0023D340 with computed arguments.
 */
#include "common.h"

extern void func_0023BB40(void);
extern void func_0023D340(int);
extern int D_0016130C MACRO_ADDR;

void func_0023E478(void) {
    int v;
    func_0023BB40();
    v = D_0016130C + 0xD9090;
    goto call;
call:
    func_0023D340(v);
}
