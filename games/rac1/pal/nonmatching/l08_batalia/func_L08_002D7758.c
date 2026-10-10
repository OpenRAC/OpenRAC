/* NON_MATCHING func_L08_002D7758 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: BYTES 13/248 (94.8% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L08_002D7758: walks a short list (D_L08_001AC340[group byte of owner]) of moby indexes (low 15 bits, bit 
 *   Best: p6.c, BYTES 13/248. Structure matches (MACRO_ADDR global, for(m = init; ; p++, m = recompute) { ...; if 
 *   Remaining diff: register allocation of the m recompute at loop top and in the preheader (retail lhu $v1 / andi
 */
extern char *D_L08_00160058 MACRO_ADDR;
extern short *D_L08_001AC340[];
extern void func_0020D678(void *);

/* delete the mobys listed for this moby's group, except itself, when their type is in range */
void func_L08_002D7758(char *moby) {
    short *p = D_L08_001AC340[((unsigned char *)D_L08_00160058)[(*(int *)(*(char **)(moby + 0x78) + 0x144) << 8) + 0x21]];
    char *m;
    if (p != 0) {
        for (m = D_L08_00160058 + ((unsigned short)(*(unsigned short *)p & 0x7FFF) * 256); ;
             p++, m = D_L08_00160058 + ((unsigned short)(*(unsigned short *)p & 0x7FFF) * 256)) {
            if (((unsigned char *)m)[0x20] < 0x7F && m != moby) {
                unsigned short t = *(unsigned short *)(m + 0xA6);
                if ((unsigned short)(t - 0x24C) < 0xB) {
                    func_0020D678(m);
                } else if ((short)t == 0x1D8) {
                    func_0020D678(m);
                }
            }
            if (*p < -1) break;
        }
    }
}
