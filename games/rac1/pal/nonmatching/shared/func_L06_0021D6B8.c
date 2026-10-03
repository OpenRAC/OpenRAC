/* NON_MATCHING func_L06_0021D6B8 -- src/overlays/shared/help_0021D6B8.c
 * Best so far: SIZE ours 148 / retail 156, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Returns 0 / 0x54 / table[f(0)].v (0x4C-byte entries, field 0x24) from game-state bytes at D_0013E633+0xE1D+off
 *   Best p4.c: same logic, 148 vs 156 bytes. Retail keeps lui(hi) in a copy ($6) and recomputes the lo for the lat
 *   Tried: local base pointer, separate second pointer, switch for the 1/3 test, direct addend expressions (folds 
 */
#include "common.h"
extern unsigned char D_0013E633[] NOT_SDA;
typedef struct { char pad[0x24]; int v; char pad2[0x24]; } Entry;
extern char D_L06_0017A340[];
extern int func_L00_0020DB30(int);

/* Returns a value for the current game state, or zero when a condition fails. */
int func_L06_0021D6B8(int flag) {
    unsigned char *g = D_0013E633 + 0xE1D;
    unsigned char *h;
    if (g[0x20A4] == 1 || g[0x20A4] == 3)
        return 0;
    h = D_0013E633 + 0xE1D;
    if (*(int *)(h + 0x22A8) == 1)
        return 0x54;
    if (flag == 0)
        return 0;
    if (h[0x20A8] == 0)
        return 0;
    if (h[0x20AA] == 0)
        return 0;
    if (*(short *)(h + 0x22C8) != 0)
        return 0;
    return ((Entry *)D_L06_0017A340)[func_L00_0020DB30(0)].v;
}
