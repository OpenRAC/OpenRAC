/* NON_MATCHING func_L00_00235FF8 -- src/overlays/shared/hud_00235960.c
 * Best so far: BYTES 73/148 (50.7% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   no statement order I tried kept the stores in place. This is the same wall round 1 found.
 *   2. The `r` load (`lw $r,D_00160098__gp`) is last in our schedule, first in retail (in the beq delay slot). So 
 *   slot of the beq holds a different load than retail's.
 *   3. Registers differ throughout (allocation follows the schedule).
 *   Verdict for the lead: no EXACT and no plain-C candidate reaches a size match plus exact order in this tree. Be
 *   p10.c (BYTES 73/148). The earlier flag candidate p5.c is still the one that gets the retail ordering from vola
 *   but it can't be verified here (run 1). If the lead wants it re-checked, it needs `func_L00_00235FF8` in config
 *   with `-mno-split-addresses` and the neighbour's ps2eeas_nops refusal fixed first.
 */
#include "common.h"

extern int D_L00_0015F808 SDATA(D_L00_0015F808);
extern int D_L00_00160098 SDATA(D_L00_00160098);

extern int D_L00_0015FB28 NOT_SDA;
extern int D_L00_0015FB2C NOT_SDA;
extern int D_L00_0015FB30 NOT_SDA;
extern int D_L00_0015FB34 NOT_SDA;
extern int D_L00_0016009C NOT_SDA;
extern int D_L00_001600A0 NOT_SDA;
extern int D_L00_001600A8 NOT_SDA;

extern int S_0015FB28 __asm__("D_L00_0015FB28") MACRO_ADDR;
extern int S_0015FB2C __asm__("D_L00_0015FB2C") MACRO_ADDR;
extern int S_0015FB30 __asm__("D_L00_0015FB30") MACRO_ADDR;
extern int S_0015FB34 __asm__("D_L00_0015FB34") MACRO_ADDR;
extern int S_00160098 __asm__("D_L00_00160098") MACRO_ADDR;
extern int S_0016009C __asm__("D_L00_0016009C") MACRO_ADDR;
extern int S_001600A0 __asm__("D_L00_001600A0") MACRO_ADDR;
extern int S_001600A8 __asm__("D_L00_001600A8") MACRO_ADDR;

void func_L00_00235FF8(int arg0)
{
    int r, a, b, c, d, e, f, g, flag;

    if (arg0 == D_L00_0015F808)
        return;

    r = D_L00_00160098;
    flag = D_L00_0015F808 ^ 1;
    a = D_L00_0015FB28;
    S_0015FB28 = r;
    b = D_L00_0016009C;
    c = D_L00_0015FB2C;
    S_0015FB2C = b;
    d = D_L00_001600A0;
    e = D_L00_0015FB30;
    f = D_L00_001600A8;
    g = D_L00_0015FB34;
    D_L00_0015F808 = flag;
    S_00160098 = a;
    S_0016009C = c;
    S_001600A0 = e;
    S_0015FB30 = d;
    S_001600A8 = g;
    S_0015FB34 = f;
}
