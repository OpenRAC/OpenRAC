/* NON_MATCHING func_L00_00211908 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: SIZE ours 264 / retail 272, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-frame bookkeeping: bumps D_0015EFA4+8 and a per-world counter table entry, sets D_L00_0015F684 when the cu
 *   Budget spent; best is p4.c (size matches, 59/272 bytes): register numbering is shifted by one from the start (
 *   Would unblock: getting D_0015EE84 gp-relative while the first read stays lui+lw, and a way to stop the 0xD375/
 */
extern char D_0013E633[] NOT_SDA;
extern char D_0013DE6E[] NOT_SDA;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern short D_0015EE84;
extern int D_0015EFA4[] MACRO_ADDR;
extern int D_L00_0015F684 MACRO_ADDR;
extern void func_L00_00204040();
extern int func_001F9850(int);
extern void func_001F4E08(int);
/* Per-frame bookkeeping: bumps counters, then ends the frame with a wait. */
void func_L00_00211908(void) {
    int w = D_0015EE84_m;
    char *g = D_0013E633 + 0xE1D;
    int *c = (int *)(D_0013DE6E + 0x272) + w;
    char *o;
    char *g2;
    char *q;
    D_0015EFA4[2] = D_0015EFA4[2] + 1;
    *c = *c + 1;
    o = *(char **)(g + 0x2280);
    if (o != 0 && *(short *)(o + 0xA6) == 0x4D && w == 0xF)
        D_L00_0015F684 = 1;
    func_L00_00204040();
    g2 = D_0013E633 + 0xE1D;
    o = *(char **)(g2 + 0x2280);
    if (o != 0 && *(unsigned char *)(o + 0xB0) != 0xFF) {
        int *p = (int *)(D_0014171B + 0xD375) + (*(unsigned char *)(o + 0xB0) + *(int *)&D_0015EE84 * 16);
        *p = *p + 1;
        p = (int *)(D_0014171B + 0xD875) + *(unsigned char *)(o + 0xB0);
        *p = *p + 1;
    }
    func_001F4E08(func_001F9850(0x10));
    q = D_0013E633 + 0xE1D;
    q[0x20B1] = 1;
}
