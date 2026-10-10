/* NON_MATCHING func_L05_002F62E8 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: SIZE ours 564 / retail 560, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Applies or reverses signed vendor commands controlling moby render flags and animation pointer.
 *   p1/p3/p5 compile identically at 548/560 bytes: unsigned command copies and subsequent sign shifts are folded a
 *   Stopped at three distinct wordings; command live ranges preserving unsigned copies without changing positive c
 */
extern short D_L05_00160098;
extern int D_L05_00160098_m __asm__("D_L05_00160098") MACRO_ADDR;
// Applies or reverses signed vendor commands to moby flags and animation data.
void func_L05_002F62E8(int id, int command_arg, int apply) {
 signed char *commands = (signed char *)command_arg;
 unsigned char command;
 char *moby = (char *)D_L05_00160098_m + (id << 8);
 if (!moby) return;
 if (apply) {
  if (commands[0x7C] > 0) *(unsigned short *)(moby + 0x34) &= 0xFFFD;
  else if (commands[0x7C] < 0) *(unsigned short *)(moby + 0x34) |= 2;
  command = *(unsigned char *)(commands + 0x7D);
  if (commands[0x7D] > 0) {
   int type = *(short *)(moby + 0xA6);
   if (type != 0x386 && type != 0x3AE && type != 0x4EF && type != 0x571) {
    moby[0x31] = 1;
    *(unsigned short *)(moby + 0x34) &= 0xFFFE;
   }
  } else if (((int)command << 24) < 0) {
   int type = *(short *)(moby + 0xA6);
   if (type != 0x386 && type != 0x3AE && type != 0x4EF && type != 0x571) {
    moby[0x31] = 0;
    *(unsigned short *)(moby + 0x34) |= 1;
   }
  }
  if (commands[0x7F] > 0) {
   char *model = *(char **)(moby + 0x24);
   if (model) { *(int *)(moby + 0x94) = *(int *)(model + 0x10); return; }
  }
  if (commands[0x7F] < 0 && *(char **)(moby + 0x24)) *(int *)(moby + 0x94) = 0;
 } else {
  command = *(unsigned char *)(commands + 0x7C);
  if (commands[0x7C] > 0) {
   int type = *(short *)(moby + 0xA6);
   if (type != 0x386 && type != 0x3AE && type != 0x4EF) *(unsigned short *)(moby + 0x34) |= 2;
  } else if (((int)command << 24) < 0) {
   int type = *(short *)(moby + 0xA6);
   if (type != 0x386 && type != 0x3AE && type != 0x4EF) *(unsigned short *)(moby + 0x34) &= 0xFFFD;
  }
  command = *(unsigned char *)(commands + 0x7D);
  if (commands[0x7D] > 0) {
   int type = *(short *)(moby + 0xA6);
   if (type != 0x386 && type != 0x3AE && type != 0x4EF && type != 0x571) {
    moby[0x31] = 0;
    *(unsigned short *)(moby + 0x34) |= 1;
   }
  } else if (((int)command << 24) < 0) {
   int type = *(short *)(moby + 0xA6);
   if (type != 0x386 && type != 0x3AE && type != 0x4EF && type != 0x571) {
    moby[0x31] = 1;
    *(unsigned short *)(moby + 0x34) &= 0xFFFE;
   }
  }
  if (commands[0x7F] > 0) *(int *)(moby + 0x94) = 0;
  else if (commands[0x7F] < 0) *(int *)(moby + 0x94) = *(int *)(*(char **)(moby + 0x24) + 0x10);
 }
}
