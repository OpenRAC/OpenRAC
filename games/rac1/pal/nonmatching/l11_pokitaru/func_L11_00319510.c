/* NON_MATCHING func_L11_00319510 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 316 / retail 324, checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   func_L11_00319510: counts the type-0x527 mobys in the list D_L11_001AC540[moby data+0xE4], then hands each one
 *   Best candidate p0.c (BYTES 68/324, same size, structure right): retail hoists `lw D_L11_00160058` out of the s
 *   Would unblock: a qcopy variant without the memory clobber (retail's copy macro evidently had none) so that loo
 */
extern short *D_L11_001AC540[];
extern int D_L11_00160058_m __asm__("D_L11_00160058") MACRO_ADDR;

/* distributes the entries of a table over the mobys of type 0x527 in a list */
void func_L11_00319510(char *moby, int arg) {
    int idx = *(short *)(*(char **)(moby + 0x78) + 0xE4);
    short *p = D_L11_001AC540[idx];
    short count = 0;
    short pos = 0;
    if (p != 0) {
        int *tab;
        char *src;
        short step;
        char *base;
        do {
            if (*(short *)(((*(unsigned short *)p & 0x7FFF) << 8) + D_L11_00160058_m + 0xA6) == 0x527) {
                count = count + 1;
            }
        } while (*p++ >= 0);
        tab = D_L11_001B11B0[arg];
        src = (char *)tab + 0x10 + pos * 16;
        step = *tab / count;
        base = (char *)D_L11_00160058_m;
        p = D_L11_001AC540[idx];
        do {
            char *m = base + ((*(unsigned short *)p & 0x7FFF) << 8);
            if (*(short *)(m + 0xA6) == 0x527) {
                char *d = *(char **)(m + 0x78);
                qcopy(m + 0x10, src);
                *(int *)(d + 0x64) = pos;
                *(int *)(d + 0x68) = 0;
                *(int *)(d + 0x60) = arg;
                pos = pos + step;
                src += step * 16;
            }
        } while (*p++ >= 0);
    }
}
