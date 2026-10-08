/* NON_MATCHING func_L00_0024A490 -- src/overlays/shared/menu_00249720.c
 * Best so far: BYTES 2/124 (98.4% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tests a menu coordinate range and enable flag for large selections.
 *   Eight runs exhausted; best p3/p4/p7 differ only in lbu and sltu using v0 rather than retail v1. These three ex
 *   A way to retain a separate flag-load register without perturbing shared-return branch layout would unblock it.
 */
extern int D_0013D4E8 MACRO_ADDR;
/* tests menu range and the low-range enable flag */
int func_L00_0024A490(int a, float b, float c, float x) {
 if (a < 221) return x>=238.0f && x<=241.0f;
 return x<=180.0f && 0 < *(unsigned char *)&D_0013D4E8;
}
