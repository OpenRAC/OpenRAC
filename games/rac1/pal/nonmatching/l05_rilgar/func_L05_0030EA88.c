/* NON_MATCHING func_L05_0030EA88 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: SIZE ours 180 / retail 184, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L05_0030EA88(moby): walks the short list D_L05_001AC040[moby[0x21]] (ends at a negative entry), for each 
 *   Best p9 (38 bytes differ): body, branch shape and addu operand order match; left: list pointer and d2 swapped 
 *   Wording tried: int base with index-first add, inverted if/else (needed for bc1fl shape), local tab pointer (no
 */
extern char D_L05_001D6BC0[];
extern short *D_L05_001AC040[];

// copies the moby's x position onto each linked moby of type 0x33F, following the parent's offset
void func_L05_0030EA88(char *moby) {
    char *data = *(char **)(moby + 0x78);
    short *p = D_L05_001AC040[*(unsigned char *)(moby + 0x21)];
    char (*base)[256] = (char (*)[256])D_L05_00160098;
    do {
        int idx = *p & 0x7FFF;
        char *m = base[idx];
        if (*(short *)(m + 0xA6) == 0x33F) {
            char *d2 = *(char **)(m + 0x78);
            char *q;
            float f = *(float *)(moby + 0x18);
            if (*(int *)(data + 0xC) == 0 && *(float *)(d2 + 4) != *(float *)(data + 4)) {
                f = *(float *)(d2 + 4) + (f - *(float *)(data + 4));
            }
            *(float *)(m + 0x18) = f;
            q = D_L05_001D6BC0 + *(int *)(d2 + 0x14) * 0x1190;
            *(float *)(q + 8) = *(float *)(base[idx] + 0x18);
        }
    } while (*p++ >= 0);
}
