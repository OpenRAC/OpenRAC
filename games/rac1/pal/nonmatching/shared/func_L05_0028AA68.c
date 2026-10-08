/* NON_MATCHING func_L05_0028AA68 -- src/overlays/shared/mobyutil_0028AA68.c
 * Best so far: SIZE ours 148 / retail 144, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Selects the greatest float at offset 0x2C among four nullable objects.
 *   p2/p3/p5 compile identically: entry beql moves first fallback assignment into delay slot, and fallback b/c ass
 *   Stopped at three-wording scheduler tie; p4 fixes entry but changes third FP branch, so fallback/control-flow l
 */
// Selects the non-null object with the greatest value at offset 0x2C.
char *func_L05_0028AA68(char *a, char *b, char *c, char *d) {
 char *best = a;
 if (!best) goto fallback;
 if (b) {
  float x = *(float *)(best + 0x2C);
  float y = *(float *)(b + 0x2C);
  if (!(y < x)) best = b;
 }
third:
 if (c) {
  float x = *(float *)(best + 0x2C);
  float y = *(float *)(c + 0x2C);
  if (!(y < x)) best = c;
 }
fourth:
 if (!d) goto done;
 {
  float x = *(float *)(best + 0x2C);
  float y = *(float *)(d + 0x2C);
  if (y < x) goto done;
 }
 return d;
fallback:
 best = b;
 if (best) goto third;
 best = c;
 if (best) goto fourth;
 best = d;
done:
 return best;
}
