/* NON_MATCHING func_L00_00235FF8 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 128 / retail 148, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   `-G8`); `MACRO_ADDR` was considered for the last field to make the store
 *   "count as one instruction" for delay-slot purposes, but under the `-G8`
 *   needed for the two resident globals a 4-byte `MACRO_ADDR` field would
 *   itself become truly gp-relative (wrong -- retail uses `lui`/`lo` for it),
 *   so it isn't a safe thing to combine here.
 *   Best candidate: `p5.c`, run with
 *   `TRY_CFLAGS='-G8 -mno-split-addresses' python tools/try_func.py func_L00_00235FF8 build-sn/try/func_L00_00235F
 *   (a plain `p5.c` run without those flags will not reproduce this result).
 */
#include "common.h"

extern int D_0015F808;
extern int D_00160098;

extern volatile int D_L00_0015FB28 NOT_SDA;
extern volatile int D_L00_0015FB2C NOT_SDA;
extern volatile int D_L00_0015FB30 NOT_SDA;
extern int D_L00_0015FB34 NOT_SDA;
extern volatile int D_L00_00160098 NOT_SDA;
extern volatile int D_L00_0016009C NOT_SDA;
extern volatile int D_L00_001600A0 NOT_SDA;
extern volatile int D_L00_001600A8 NOT_SDA;

void func_L00_00235FF8(int arg0)
{
    int r, a, b, c, d, e, f, g;

    if (arg0 == D_0015F808)
        return;

    r = D_00160098;
    D_0015F808 = D_0015F808 ^ 1;
    a = D_L00_0015FB28;
    D_L00_0015FB28 = r;
    b = D_L00_0016009C;
    c = D_L00_0015FB2C;
    D_L00_0015FB2C = b;
    d = D_L00_001600A0;
    e = D_L00_0015FB30;
    f = D_L00_001600A8;
    g = D_L00_0015FB34;
    D_L00_00160098 = a;
    D_L00_0016009C = c;
    D_L00_001600A0 = e;
    D_L00_0015FB30 = d;
    D_L00_001600A8 = g;
    D_L00_0015FB34 = f;
}
