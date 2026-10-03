/* NON_MATCHING func_L00_001EB380 -- src/overlays/shared/bloaders_001EB380.c
 * Best so far: SIZE ours 108 / retail 104, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Loads the debug-font texture (D_L00_0018F040) via func_002176C8 + func_001E94E8 and caches the returned TEX0 w
 *   Wall: retail is 26 words and ends at `jr $31`; its delay slot (the `addiu $sp,$sp,0x40` restore) is the separa
 *   Unblock: a way to split a function from its delay slot, or treat it with func_001FFD98 as one 108-byte unit.
 */
extern int D_00137C80[];
extern char D_L00_0018F040[];
extern void func_002176C8(void *, int, int);
extern int D_0015EF88;
extern void func_001E94E8(void *arg0, void *arg1, int arg2, int arg3);
extern long D_0015EFC8;

/* Loads a texture at D_L00_0018F040 into VRAM and caches its TEX0 word. */
void func_L00_001EB380(void) {
    long localbuf[3];

    func_002176C8(D_L00_0018F040, D_00137C80[2], D_00137C80[3]);
    func_001E94E8(D_L00_0018F040, localbuf, D_0015EF88 + 0xC0000, 0x3FFC00);
    D_0015EFC8 = localbuf[0];
}
