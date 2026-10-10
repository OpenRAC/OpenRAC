/* NON_MATCHING func_L18_002FDCA0 -- src/overlays/l18_veldin2/vendor_002F9D48.c
 * Best so far: SIZE ours 124 / retail 128, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Counts class 0x772 objects whose state is not 8, with a signed short count.
 *   p0/p1/p2 compile identically: missing initial ID daddu copy and class compare uses bne with swapped temporarie
 *   Stopped at three-wording allocator/scheduler tie; different ID lifetime or class condition structure would unb
 */
extern unsigned short *D_L18_001AC540[];
extern unsigned char *D_L18_00160058_m __asm__("D_L18_00160058") MACRO_ADDR;
// Counts non-state-eight objects of class 0x772 in an ID list.
int func_L18_002FDCA0(int index) {
 unsigned short *list = D_L18_001AC540[index];
 short count = 0;
 unsigned char *base;
 if (!list) return 0;
 base = D_L18_00160058_m;
 do {
  unsigned short id = *list;
  unsigned char *moby = base + ((id & 0x7FFF) << 8);
  if (*(short *)(moby + 0xA6) == 0x772) { if (moby[0x20] != 8) count = count + 1; }
  list++;
  if ((short)id < 0) return count;
 } while (1);
}
