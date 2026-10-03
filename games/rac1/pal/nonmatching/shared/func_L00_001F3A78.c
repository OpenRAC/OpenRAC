/* NON_MATCHING func_L00_001F3A78 -- src/overlays/shared/draw_001F3A78.c
 * Best so far: SIZE ours 124 / retail 116, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Queue drain loop: calls func_L00_001F3AF0(p[0..3]) for each 16-byte entry of D_L00_0016A940 while i < count (D
 *   Best p1/p3: only 6 prologue instructions differ (retail: count load first, lui/addiu of the list after the ble
 *   The gp-rel int needed `extern short X;` accessed as *(int *)&X (a plain int or MACRO_ADDR int gives lui+lw); a
 *   Unblock: source shape that sinks the list lui past blez without the extra hoist. Budget spent.
 */
extern short D_L00_0015F0C0;
extern short D_L00_0015F0C4;
extern int D_L00_0016A140;
extern int D_L00_0016A940[];
extern void func_L00_001F3AF0(int, int, int, int);

// draws each queued entry of the 16-byte-stride list, then resets the queue
void func_L00_001F3A78(void) {
    int i = 0;
    int *p;
    if (*(int *)&D_L00_0015F0C4 > 0) {
        p = D_L00_0016A940;
        do {
            i++;
            func_L00_001F3AF0(p[0], p[1], p[2], p[3]);
            p += 4;
        } while (i < *(int *)&D_L00_0015F0C4);
    }
    *(int *)&D_L00_0015F0C4 = 0;
    *(int **)&D_L00_0015F0C0 = &D_L00_0016A140;
}
