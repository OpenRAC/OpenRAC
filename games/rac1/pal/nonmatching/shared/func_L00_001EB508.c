/* NON_MATCHING func_L00_001EB508 -- src/overlays/shared/camera_001EB508.c
 * Best so far: BYTES 11/108 (89.8% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Dispatches queued camera callbacks and clears the count; includes joined reset tail.
 *   p1, p2, and p3 compile to identical bytes: saved counter and callback pointer registers s0/s1 are swapped thro
 *   Unblock requires an allocator idiom changing the saved-register assignment.
 */
extern int D_L00_0015F04C MACRO_ADDR;
extern void (*D_L00_001690F0[])(void);
/* dispatches queued camera callbacks then clears the count */
void func_L00_001EB508(void) {
 void (**p)(void);
 int i = 0;
 if (D_L00_0015F04C > 0) {
 p = D_L00_001690F0;
 do { (*p++)(); ++i; } while (i < D_L00_0015F04C);
 }
 D_L00_0015F04C = 0;
}
