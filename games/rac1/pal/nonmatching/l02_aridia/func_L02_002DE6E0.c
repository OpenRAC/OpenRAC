/* NON_MATCHING func_L02_002DE6E0 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: SIZE ours 192 / retail 200, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds a class 0x244 state 0x1A object and transfers its count to/from the caller.
 *   p2/p3/p4 compile identically: missing saved base/ID copies and second indexed address calculation, while const
 *   Stopped at three-wording allocator tie; a distinct base/index live range would unblock.
 */
extern int D_L02_001AC140[];
extern int D_L02_00160058_m __asm__("D_L02_00160058") MACRO_ADDR;
// Finds a waiting object and transfers its remaining count.
char *func_L02_002DE6E0(unsigned char *moby) {
 unsigned short *list;
 char *base;
 unsigned short id;
 if (moby[0x21] == 0xFF) return 0;
 list = (unsigned short *)D_L02_001AC140[moby[0x21]];
 base = (char *)D_L02_00160058_m;
 do {
  int offset;
  char *other;
  id = *list;
  offset = (id & 0x7FFF) << 8;
  other = base + offset;
  if (*(short *)(other + 0xA6) != 0x244) goto next;
  if (*(unsigned char *)(other + 0x20) == 0x1A) {
   *(unsigned char **)(other + 0xB8) = moby;
   if (*(short *)(moby + 0xB4) < *(short *)(other + 0xB4)) *(short *)(other + 0xB4) = *(unsigned short *)(moby + 0xB4);
   other = base + offset;
   *(unsigned short *)(moby + 0xB4) -= *(unsigned short *)(other + 0xB4);
   if (*(short *)(moby + 0xB4) <= 0) *(short *)(moby + 0xB4) = 1;
   return other;
  }
next:;
 } while (list++, (short)id >= 0);
 return 0;
}
