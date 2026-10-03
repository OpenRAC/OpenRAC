/* NON_MATCHING func_L00_00210E00 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: SIZE ours 488 / retail 480, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-frame update of the 7 slots at D_0013F450 (stride 0x50): mode 2 / mode 3 handling, then each live slot's u
 *   Best is p5.c (size 496 vs 480): loop shape, $21 = i+1 copy and li-1 constant match the retail structure. Diffe
 *   Unblock: find the source form that makes gcc emit madd for `slot = base + i*0x50` (g-local, &array[i], (int)ba
 *   Hint found later: declare the inner-loop arrays (g+0x18F0, g+0x1990) and other sym+offset operands as their ow
 */
extern char D_0013E633[];
extern short D_L00_0015F4F8;
extern int D_L00_0015F4F8_m __asm__("D_L00_0015F4F8") MACRO_ADDR;
extern void func_L00_00210558(int);
extern void func_L00_00205780(char *p);
extern void func_00213DE0(void *, int, int, int);
extern void func_L00_00210340(int, int);

/* Per-frame update of the seven slots: run each slot's mode handling and its update callback. */
void func_L00_00210E00(void) {
    int i;
    char *g = D_0013E633 + 0xE1D;

    for (i = 0; i < 7; i++) {
        char *e = g + i * 0x50;
        char *obj = *(char **)(e + 0x1090);

        if (obj != 0) {
            if (*(int *)(e + 0x10B4) == 2) {
                *(int *)(e + 0x10B0) += 1;
                func_L00_00210558(i);
                if (i == 0) {
                    char *p, *q = g + 0x18F0;
                    for (p = g; p < g + 0x210; p += 0xB0, q += 0xB0) {
                        if (*(short *)(p + 0x1990) != -1) {
                            *(short *)(p + 0x1992) = 5;
                            func_L00_00205780(q);
                        }
                    }
                }
                if ((*(unsigned char *)(obj + 0x70) & 2) && *(unsigned char *)(obj + 0x53) == 0) {
                    func_00213DE0(obj, 1, 0, 2);
                }
            } else if (*(int *)(e + 0x10B4) == 3) {
                int v;
                *(unsigned char *)(e + 0x10AB) += 1;
                v = *(int *)&D_L00_0015F4F8;
                if ((*(unsigned char *)(obj + 0x70) & 2) || i == 1) {
                    func_L00_00210340(i, v + 2);
                } else if (i == 0) {
                    func_L00_00210340(i, D_L00_0015F4F8_m + 2);
                }
            }
        } else {
            func_L00_00210558(i);
        }
        if (obj != 0 && *(unsigned char *)(obj + 0x20) != 0xFE && *(unsigned char *)(obj + 0x20) != 0xFD) {
            if (i != 6 && *(int *)(obj + 0x74) != 0) {
                (*(void (**)(char *))(obj + 0x74))(obj);
            }
        }
    }
}
