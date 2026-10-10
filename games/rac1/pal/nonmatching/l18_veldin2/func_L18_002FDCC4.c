/* NON_MATCHING func_L18_002FDCC4 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: SIZE ours 88 / retail 92, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Counts mobys of class 0x772 (lh 0xA6) not in state 8 (lbu 0x20) along an id list (ushort, bit15 ends), args (a
 *   Closest: p1/p5 (100 bytes vs 92): shape matches (bnel, same branches) but gcc strength-reduces the (short) n i
 *   Short param, K&R short param, int param with cast, short temp all give the hoisted form; something stops loop.
 */

// Counts mobys of class 0x772 not in state 8 along a list ended by a negative id.
int func_L18_002FDCC4(int a, unsigned short *list, int b, int n) {
    char *base = D_L18_00160058;
    unsigned short id;
    do {
        char *m;
        id = *list;
        m = base + ((id & 0x7FFF) << 8);
        if (*(short *)(m + 0xA6) == 0x772 && *(unsigned char *)(m + 0x20) != 8) {
            n++;
            n = (short)n;
        }
        list++;
    } while ((short)id >= 0);
    return n;
}
