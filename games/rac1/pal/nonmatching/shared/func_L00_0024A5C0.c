/* NON_MATCHING func_L00_0024A5C0 -- src/overlays/shared/menu_00249720.c
 * Best so far: BYTES 2/152 (98.7% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tests the menu coordinate range and enable flag.
 *   Stopped after three different Boolean wordings produce identical bytes: lbu/sltu use v0 where retail uses v1.
 *   Need a register-allocation idiom preserving the remaining exact shared-return layout.
 */
extern int D_0013D4EB MACRO_ADDR;
/* tests the menu coordinate range and enable flag */
int func_L00_0024A5C0(int a,float b,float c,float x) {
 if(a<351) return x>=228.0f && x<=230.0f;
 return x>=233.0f && x<=235.0f && !!*(unsigned char *)&D_0013D4EB;
}
