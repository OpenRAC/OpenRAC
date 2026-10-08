/* NON_MATCHING func_L01_00278FA8 -- src/overlays/shared/mobyutil_0026E8E0.c
 * Best so far: SIZE ours 80 / retail 88, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tests hero support moby in two movement-state paths. p2/p3/p4 compile identically despite conditional, goto, a
 *   Remaining allocator/scheduler tie changes argument/result registers and emits xor/sltiu instead of retail fina
 */
extern char D_0013F450[] MACRO_ADDR;
// Tests whether the hero stands on the specified moby.
int func_L01_00278FA8(int moby) {
 char *hero = D_0013F450;
 if (*(int *)(hero + 0x208C) == 3 || *(int *)(hero + 0x2084) == 0x1C) {
  if (*(int *)(hero + 0x4F8) != moby) return 0;
  return 1;
 }
 if (*(short *)(hero + 0x30E) != 0) return 0;
 if (*(int *)(hero + 0x2FC) == moby) return 1;
 return 0;
}
