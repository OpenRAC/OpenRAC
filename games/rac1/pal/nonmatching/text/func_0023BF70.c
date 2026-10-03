/* EXACT func_0023BF70 -- src/game/movie/movie.c
 * Matched: 2026-10-03, EXACT (masked).
 * MACRO_ADDR function that loads a global and calls func_0023C2C0.
 * The function passes (D_0016130C + 0xD9100) to func_0023C2C0.
 */
#include "common.h"

extern void func_0023C2C0(int);
extern int D_0016130C MACRO_ADDR;

void func_0023BF70(void) {
    int v = D_0016130C + 0xD9100;
    func_0023C2C0(v);
}
