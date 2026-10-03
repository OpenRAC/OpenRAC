/* NON_MATCHING func_L14_00306A88 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: BYTES 16/128 (87.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L14_00306A88: walks a halfword list (table D_L14_001AC2C0 indexed by *moby->data), for each id (low 15 bi
 *   Body and loop match; only the preheader hoist order differs (16 bytes: retail lui hi(EE920) first, then lui hi
 *   Pointer-walk, indexed, -=, swapped-init-order wordings (p0,p3,p4,p5) all give the same bytes: a scheduler tie.
 */
extern unsigned short *D_L14_001AC2C0[];
extern float D_L14_001EE920[];
extern int D_L14_00160098;

/* For each entry in a moby-id list, saves the moby's Y and lowers it by 20. */
void func_L14_00306A88(char *moby) {
    unsigned short *p = D_L14_001AC2C0[**(int **)(moby + 0x78)];
    float *out;
    if (p != 0) {
        int base = D_L14_00160098;
        out = D_L14_001EE920;
        do {
            float *f = (float *)(base + ((*p & 0x7FFF) << 8));
            *out++ = f[6];
            f[6] -= 20.0f;
            *(unsigned short *)((char *)f + 0x34) |= 0x41;
        } while (*(short *)p++ >= 0);
    }
}
