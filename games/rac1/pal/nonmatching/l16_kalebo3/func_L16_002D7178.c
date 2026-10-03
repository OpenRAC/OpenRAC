/* NON_MATCHING func_L16_002D7178 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 200 / retail 204, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the nearest class-0x28A moby (distance < 37.0) in a zone list of short ids (bit 15 = last), returning it
 *   Retail keeps the outer loop aligned (a real loop) but does NOT hoist li 0x28A out of it (a1, reloaded each pas
 *   Best: p6 (goto loops, MACRO_ADDR base) 200 vs 204. Unblock: a loop form where the constant stays unhoisted whi
 */
extern float func_001F9D10(void *, void *);
extern int *D_L16_001ABFC0[];
extern char *D_L16_00160098 MACRO_ADDR;

// Find the nearest moby of class 0x28A in a zone list, within distance 37.
char *func_L16_002D7178(int idx, void *pos) {
    short *p = (short *)D_L16_001ABFC0[idx];
    char *best = 0;
    float bd = 37.0f;
    char *m;
    float d;
    if (p == 0) return 0;
next:
    m = D_L16_00160098 + ((*(unsigned short *)p & 0x7FFF) << 8);
    if (*(short *)(m + 0xA6) != 0x28A) goto next;
    d = func_001F9D10(pos, m + 0x10);
    if (d < bd) {
        bd = d;
        best = m;
    }
    if (*p++ >= 0) goto next;
    return best;
}
