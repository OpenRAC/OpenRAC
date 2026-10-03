/* NON_MATCHING func_L14_002F0A30 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 140 / retail 132, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Sets bit (id&31) of word (short)id>>5 in a per-level table at D_0014171B+0xAB75 (stride 256 bytes by D_0015EE8
 *   p3.c gets the operand orders of the adds right (38 bytes off); remaining difference is scheduling only: retail
 *   Tried local/2D-array/short-local wordings; all schedule the lhu first. A scheduler tie that rewording did not 
 */
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern unsigned char D_0014171B NOT_SDA;
extern int D_L14_001BAF60[];

/* marks the moby's flag bit in the level's and the global bit tables */
void func_L14_002F0A30(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int (*t)[64] = (int (*)[64])(&D_0014171B + 0xAB75);
    int bit = 1;
    short id;
    moby[0xBC] = 0;
    *(short *)(data + 0x124) = 0;
    id = *(short *)(moby + 0xB2);
    t[D_0015EE84_m][id >> 5] |= bit << (id & 0x1F);
    id = *(short *)(moby + 0xB2);
    D_L14_001BAF60[id >> 5] |= bit << (id & 0x1F);
}
