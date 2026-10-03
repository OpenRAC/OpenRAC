/* NON_MATCHING func_L07_00270BE0 -- src/overlays/l07_umbris/map_00270BE0.c
 * Best so far: BYTES 5/304 (98.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Clears set bits in a (x..x+w, y..y+h) rectangle of a bit grid (D_L07_001842F0 +0xC pointer), calling func_L00_
 *   p4.c is 5 bytes off: only the final lbu/nor/and/sb triple has $v0/$v1 swapped (retail loads into $v1, nor into
 *   Key: `i < x + w` inline and `y < y0 + h` inline (not hoisted locals) gave the spills.
 */
extern void func_L00_0024A798(int);
extern struct { int a, b, c; char *p; } D_L07_001842F0;

/* Clears each set bit in a rectangle of a per-row bit grid, notifying for each. */
void func_L07_00270BE0(int x, int y0, int w, int h) {
    int y = y0;
    for (; y < y0 + h; y++) {
        int rowbase = (y >> 5) << 4;
        int i;
        for (i = x; i < x + w; i++) {
            unsigned char *p = (unsigned char *)D_L07_001842F0.p + (y << 6) + i / 8;
            if (*p != 0) {
                int bit = 1 << (i & 7);
                if (*p & bit) {
                    func_L00_0024A798((i >> 5) + rowbase);
                    *p = *p & ~bit;
                }
            }
        }
    }
}
