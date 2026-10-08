/* NON_MATCHING func_L00_002633D8 -- src/overlays/shared/mobyutil_00261B00.c
 * Best so far: BYTES 11/288 (96.2% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Keyframe table lookup: idx = clamp(func_001FA898(x/y)), frac = x - func_001FA888(i)*y, interpolates via F0/FF2
 *   p4.c: structure right (284 vs 288 bytes before parenthesising); left: tab/frac saved regs s2/s3 swapped, offse
 *   Unblock: reorder param use / how *idx<<4 offset is held (retail keeps it in the arg regs, i.e. no separate loc
 *   w12: Keyframe lookup/interpolate. p6/p7/p8 (no off local, offsets written tab + ((*idx<<4)+0x20)) fix the size
 */
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001FA888(int);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
typedef int u128_2633D8 __attribute__((mode(TI)));

/* Interpolates between keyframes of a table by a time ratio. */
void func_L00_002633D8(char *tab, int *idx, float *frac, char *out, float x, float y) {
    float tmp[4];
    int n;
    int i = func_001FA898_r(x / y);
    *idx = i;
    n = *(int *)tab - 1;
    if (i >= n) {
        *idx = n;
        goto z;
    }
    if (i < 0) {
        *idx = 0;
z:
        *frac = 0;
        *(u128_2633D8 *)out = *(u128_2633D8 *)(tab + (*idx << 4) + 0x10);
    } else {
        *frac = x - func_001FA888(i) * y;
        func_001F9BF0(tmp, tab + ((*idx << 4) + 0x20), tab + ((*idx << 4) + 0x10));
        *(u128_2633D8 *)out = *(u128_2633D8 *)tmp;
        func_L00_001FF4B0(out, out, *frac);
        func_001F9BD8(tmp, out, tab + ((*idx << 4) + 0x10));
        *(u128_2633D8 *)out = *(u128_2633D8 *)tmp;
    }
}
