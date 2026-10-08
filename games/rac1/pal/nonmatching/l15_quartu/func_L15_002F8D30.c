/* NON_MATCHING func_L15_002F8D30 -- src/overlays/l15_quartu/vendor_002EDB50.c
 * Best so far: BYTES 2/120 (98.3% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tests selected vendor data state against availability float and state 3.
 *   p1/p2/p3 give identical bytes: table base load uses v0 and index shift a0, opposite retail, with remaining ins
 *   Stopped at three-wording allocator tie; a different index/base lifetime would unblock.
 */
extern char *D_L15_00167480;
extern float D_L15_0015F4FC MACRO_ADDR;
extern char *D_L15_0015F050 MACRO_ADDR;
// Tests whether the selected vendor entry is available.
int func_L15_002F8D30(int index) {
 char *other = D_L15_00167480;
 char *entry;
 char *sub;
 char *moby;
 char *data;
 if (*(short *)(other + 0x86) != 0x13) return 1;
 { int offset = index << 5; char *base = D_L15_0015F050; entry = base + offset; }
 sub = *(char **)(entry + 0x1C);
 moby = *(char **)(sub + 4);
 data = *(char **)(moby + 0x70) + 0x40;
 if (*(short *)(data + 0x14) == 1 && D_L15_0015F4FC != 0.0f) return 1;
 return *(short *)(data + 0x14) == 3;
}
