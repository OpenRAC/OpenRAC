/* NON_MATCHING func_L01_002328A8 -- src/overlays/shared/help_002274A8.c
 * Best so far: BYTES 3/196 (98.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Clamps/sets hero speed (g+0x190) from table D_L01_0017C2B8 ([2] if speed < [3] else [6], times D_0015EE6C), th
 *   Best is p5/p9 (3 words differ): retail loads the table value into $f0 and D_0015EE6C into $f1; ours swaps them
 *   Ternary, if/else local, and operand orders all give same bytes; an allocator tie on $f0/$f1.
 */
extern unsigned char D_0013E633[];
extern void func_L00_00211F80(int, float);
extern float D_0015EE6C MACRO_ADDR;
extern float D_L01_0017C2B8[];

/* clamps hero speed from a table */
void func_L01_002328A8(void) {
    char *g;
    char *h;
    float s;
    func_L00_00211F80(0, 1.0f);
    g = (char *)D_0013E633 + 0xE1D;
    s = *(float *)(g + 0x190);
    if (0.0f < s) {
        *(float *)(g + 0x190) =
            D_0015EE6C * (s < D_L01_0017C2B8[3] ? D_L01_0017C2B8[2] : D_L01_0017C2B8[6]);
    }
    h = (char *)D_0013E633 + 0xE1D;
    if (*(int *)(h + 0x2084) == 0x73) {
        float f = *(float *)(h + 0x190) * 0.8f;
        float m = D_0015EE6C * 2.5f;
        *(float *)(h + 0x190) = f;
        if (f < m) *(float *)(h + 0x190) = m;
    }
}
