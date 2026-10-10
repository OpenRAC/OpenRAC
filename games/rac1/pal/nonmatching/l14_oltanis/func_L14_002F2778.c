/* NON_MATCHING func_L14_002F2778 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: BYTES 9/112 (92.0% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   - Lombyte: `tools/lombyte.py` reports no Lombyte checkout here (it wants $LOMBYTE or
 *   ~/Projects/Lombyte, and neither exists), so nothing was ported.
 *   ### Lead notes
 *   - Needs the joined.tsv line above before p5 can show EXACT or a byte count.
 *   - A callee with a `char *` return and an `int` state is right for the caller.
 *   - Declarations used (all new to the file, none already in vendor_002E0538.c):
 *   `extern int D_L14_001600B4;`, `extern unsigned short *D_L14_001AC2C0[];`,
 *   `extern char *D_L14_00160098 MACRO_ADDR;` (the last is in a delay slot, so gp-relative, as in retail).
 */
extern int D_L14_001600B4;
extern unsigned short *D_L14_001AC2C0[];
extern char *D_L14_00160098 MACRO_ADDR;

// Walks the moby-id list for group moby[0x21]; returns the first moby whose data
// state (+0xB4) equals STATE, or 0. The found moby is returned from a block after
// the failure return, as the retail tail does.
char *func_L14_002F2778(char *moby, int state)
{
    unsigned short *p;
    unsigned char idx = (unsigned char)moby[0x21];

    if (idx < D_L14_001600B4) {
        p = D_L14_001AC2C0[idx];
        if (p != 0) {
            char *base = D_L14_00160098;
            do {
                int off = (*p & 0x7FFF) << 8;
                char *m = (char *)(off + (int)base);
                if (*(short *)(*(char **)(m + 0x78) + 0xB4) == state)
                    goto found;
            } while ((short)*p++ >= 0);
        }
    }
    return 0;
found:
    return D_L14_00160098 + ((*p & 0x7FFF) << 8);
}
