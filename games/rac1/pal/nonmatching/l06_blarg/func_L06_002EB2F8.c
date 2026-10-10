/* NON_MATCHING func_L06_002EB2F8 -- src/overlays/l06_blarg/vendor_002B5990.c
 * Best so far: SIZE ours 96 / retail 100, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Checks that every vendor-class object in a terminated ID list is inactive.
 *   p8 matches instruction structure except missing initial daddu $a2,$v0,$zero; budget exhausted at 8/8, 96 versu
 *   Tail-recursive wording prevents loop constant hoisting; an ID live-range change retaining the initial copy wou
 */
// Checks that every listed vendor object is inactive.
int func_L06_002EB2F8(unsigned short *list, int unused1, int unused2, char *base) {
 unsigned short id = *list;
 char *moby = base + ((id & 0x7FFF) << 8);
 if (moby && *(short *)(moby + 0xA6) == 0x33B) {
  int state = *(unsigned char *)(moby + 0x20);
  if (state == 0xFE) goto next;
  if (state == 0xFD) goto next;
  if (state != 0x10) return 0;
 }
next:
 if ((short)id < 0) return 1;
 return func_L06_002EB2F8(list + 1, unused1, unused2, base);
}
