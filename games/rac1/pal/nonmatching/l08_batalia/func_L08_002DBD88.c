/* NON_MATCHING func_L08_002DBD88 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: BYTES 1/476 (99.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Picks best moby from short list by angle/dist. Best is p4.c (1 word differs: offset of the b into the shared '
 */
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9D10(void *, void *);
extern short *D_L08_001AC340[];
extern char *D_L08_00160058 MACRO_ADDR;
extern char D_0013E633[] NOT_SDA;

/* Picks the best candidate moby from the moby's list by heading and distance. */
char *func_L08_002DBD88(char *moby) {
    short *t = D_L08_001AC340[(unsigned char)moby[0x21]];
    char *best;
    char *p;
    float bestD, bestA;
    int idx;
    char *base;
    if (t == 0) return 0;
    best = 0;
    idx = *(unsigned short *)t & 0x7FFF;
    p = D_L08_00160058 + (idx << 8);
    bestA = 0.0f;
    bestD = bestA;
    for (;;) {
        if (*(short *)(p + 0xA6) != 0x14D && *(unsigned char *)(p + 0xBC) == 0) {
            float ang, d, a;
            a = func_L00_001FF860(*(float *)(p + 0x10) - *(float *)(moby + 0x10),
                                  *(float *)(p + 0x14) - *(float *)(moby + 0x14));
            base = D_0013E633 + 0xE1D;
            ang = func_001FA850(a, func_L00_001FF860(*(float *)(base + 0x80) - *(float *)(moby + 0x10),
                                                     *(float *)(base + 0x84) - *(float *)(moby + 0x14)));
            d = func_001F9D10(moby + 0x10, p + 0x10);
            if (ang < 1.5707964f) {
                if (bestA < ang) {
                    best = p;
                    bestD = d;
                    bestA = ang;
                }
            } else if ((d >= 16.0f && (d < bestD || bestD < 16.0f)) || (d < 16.0f && bestD < d)) {
                best = p;
                bestD = d;
                bestA = ang;
            }
        }
        if (*t < -1) return best;
        t++;
        idx = *(unsigned short *)t & 0x7FFF;
    p = D_L08_00160058 + (idx << 8);
    }
}
