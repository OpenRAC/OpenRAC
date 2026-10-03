/* NON_MATCHING func_L17_002CD1B0 -- src/overlays/l17_fleet/vendor_002AA068.c
 * Best so far: SIZE ours 332 / retail 328, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   For a list of mobys in a level table, counts those of type 0x733, then gives each a slot (data id, offset) fro
 *   Best is p11.c (size matches, ~all logic right: loop 1 uses `int i = 0x10000; j = i; i += 0x10000; count = j >>
 *   Would need the source shape that keeps that load after loop 1; tried a local `sel`, other cast types in the lo
 *   l17s s05: p12-p17 (unsigned index, array-index form, early returns, base2/tab order, k assigned late, for(;;) 
 *   u05 (p18-p21, budget spent): the file now declares D_L17_00160058 as `char *` MACRO_ADDR, so p11's `short` dec
 *   v03 (l17n4, p22-p27, budget spent): p25 (loop-2 base read inline each iteration; tab index read through a fres
 */
extern short *D_L17_001AC440[];
extern char D_L17_001B10B0[];
extern char *D_L17_00160058 MACRO_ADDR;
extern char (*D_L17_00160058_t)[256] __asm__("D_L17_00160058") MACRO_ADDR;

/* spreads the list's matching mobys along the level's position table */
void func_L17_002CD1B0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int count = 0;
    short k = 0;
    int i;
    short *p;
    int idx = *(short *)(data + 0x80);
    if (idx != -1) {
        p = D_L17_001AC440[idx];
        if (p != 0) {
            unsigned short s;
            char *base = D_L17_00160058;
            i = 0x10000;
            do {
                s = *p;
                if (*(short *)(base + ((s & 0x7FFF) << 8) + 0xA6) == 0x733) {
                    int j = i;
                    i += 0x10000;
                    count = j >> 16;
                }
                p++;
            } while ((short)s >= 0);
            {
                int *tab = *(int **)(D_L17_001B10B0 + *(int *)(data + 0x84) * 4);
                short q = *tab / count;
                p = D_L17_001AC440[idx];
                do {
                    char *m = D_L17_00160058_t[*p & 0x7FFF];
                    if (*(short *)(m + 0xA6) == 0x733) {
                        char *d = *(char **)(m + 0x78);
                        qcopy(m + 0x10, (char *)tab + (k * 16 + 0x10));
                        *(int *)(d + 0x64) = k;
                        *(int *)(d + 0x60) = 5;
                        *(int *)(d + 0x68) = 0;
                        k = k + q;
                    }
                } while (*p++ >= 0);
            }
        }
    }
}
