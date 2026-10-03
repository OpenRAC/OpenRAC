/* NON_MATCHING func_L00_002633D8 -- src/overlays/shared/mobyutil_00261B00.c
 * Best so far: SIZE ours 284 / retail 288, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Keyframe table lookup: idx = clamp(func_001FA898(x/y)), frac = x - func_001FA888(i)*y, interpolates via F0/FF2
 *   p4.c: structure right (284 vs 288 bytes before parenthesising); left: tab/frac saved regs s2/s3 swapped, offse
 *   Unblock: reorder param use / how *idx<<4 offset is held (retail keeps it in the arg regs, i.e. no separate loc
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
    int i = func_001FA898_r(x / y);
    int off;
    *idx = i;
    if (i >= *(int *)tab - 1) {
        *idx = *(int *)tab - 1;
        goto z;
    }
    if (i < 0) {
        *idx = 0;
z:
        *frac = 0;
        *(u128_2633D8 *)out = *(u128_2633D8 *)(tab + (*idx << 4) + 0x10);
    } else {
        *frac = x - func_001FA888(i) * y;
        off = *idx << 4;
        func_001F9BF0(tmp, tab + off + 0x20, tab + off + 0x10);
        *(u128_2633D8 *)out = *(u128_2633D8 *)tmp;
        func_L00_001FF4B0(out, out, *frac);
        off = *idx << 4;
        func_001F9BD8(tmp, out, tab + off + 0x10);
        *(u128_2633D8 *)out = *(u128_2633D8 *)tmp;
    }
}
