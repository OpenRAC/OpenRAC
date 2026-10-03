/* NON_MATCHING func_L00_002DBB10 -- src/overlays/shared/vendor_002D9438.c
 * Best so far: SIZE ours 300 / retail 304, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Sweeps 5 or 10 slots at D_0013E633+0xE1D+0x2050 (flag byte in D_0013E15A): entries whose object is in state 3 
 *   Best p5.c (for loop, per-iteration base local, i in $s2 and index in $s1 as retail) is 292 vs 304 bytes: retai
 *   Would unblock: the source shape that makes the compiler rematerialise the base only on that path (maybe the ba
 *   Wave lb1/p05: p8 (Lombyte's struct-slot form with a macro base), p9 (short k), p10 (0x6A loaded into a temp), 
 *   Difference: retail fills the delay slots with branch-likely copies (beql/bnel with lui $a0 / addiu 3 from the 
 *   Would unblock: a source form that keeps `-1` materialised after the 0x6A load (perhaps the constant comes from
 */
extern char D_0013E633[] NOT_SDA;
extern unsigned char D_0013E15A[];
extern char *func_L00_002DCD40(char *);
extern int func_001E9730();
extern char D_L00_001EA368[];

/* sweep the 5 or 10 slots, releasing the entries that are not in state 3 or 4 and counting those that are */
void func_L00_002DBB10(void) {
    int i;
    char *o;
    int h;
    char *b0 = D_0013E633 + 0xE1D;
    *(int *)(b0 + 0x207C) = 0;
    *(int *)(b0 + 0x2078) = 0;
    i = 0;
    while (i < (*(unsigned char *)(D_0013E15A + 0x4C6 + 9) ? 10 : 5)) {
        char *b = D_0013E633 + 0xE1D;
        if (*(int *)(b + i * 4 + 0x2050) == 0) {
            i++;
            continue;
        }
        o = func_L00_002DCD40(*(char **)(b + i * 4 + 0x2050));
        if (o != 0 && (h = *(short *)(o + 0x68)) >= 3 && h < 5 && *(short *)(o + 0x6A) != -1) {
            if (h == 3) {
                *(int *)(b + 0x207C) += 1;
            } else if (h == 4) {
                *(int *)(b + 0x2078) += 1;
            }
            i++;
        } else {
            int *s = (int *)(b + 0x2050);
            char *q = *(char **)((char *)s + i * 4);
            func_001E9730(D_L00_001EA368, *(short *)(q + 0xB2), *(short *)(q + 0xA6));
            *(int *)((char *)s + i * 4) = 0;
            i++;
        }
    }
}
