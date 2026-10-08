/* NON_MATCHING func_L00_0027D3D8 -- src/overlays/shared/pause_00277208.c
 * Best so far: SIZE ours 196 / retail 200, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fragment: branches to func_L00_0027D454 and func_L00_0027D464 (rest of the US function, 196 vs 124 bytes). Nee
 *   Joined packet supplies complete function; prior fragment diagnosis is superseded.
 *   p3/p5/p6 compile identically at 192/200 bytes: controller address full versus retained high part changes addiu
 *   Stopped at three distinct wordings; a controller-base lifetime preserving the high address would unblock.
 */
extern char D_0013CA40[] NOT_SDA;
extern int D_0013D48C MACRO_ADDR;
extern int D_L00_00184514;
extern char D_L00_001BA070[] NOT_SDA;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern int D_L00_0015F6A4 MACRO_ADDR;
extern int D_L00_0015F6BC MACRO_ADDR;
extern int D_L00_0015F650 MACRO_ADDR;
// Handles pause selection and transition flags.
int func_L00_0027D3D8(char *entry) {
 int flags = *(int *)(D_0013CA40 + 0x1C4);
 if (flags & 0xD00) {
  if (*(int *)(entry + 0x30) & 0x20) D_L00_00184514 = D_0015EE84_m;
  return -1;
 }
 if (flags & 0x10) {
  char *menu;
  char *selected;
  int value;
  if (*(int *)(entry + 0x30) & 0x20) D_L00_00184514 = D_0015EE84_m;
  menu = D_L00_001BA070;
  selected = *(char **)(menu + 4);
  value = *(int *)(selected + 0x38);
  if (value) { *(int *)(menu + 8) = value; return 0; }
  if (!*(int *)(menu + 0x124)) return -1;
 }
 if (!(*(int *)(D_0013CA40 + 0x1C4) & 0x20)) return 0;
 {
  D_0013D48C = 0;
  D_L00_0015F6A4 = -1;
  D_L00_0015F6BC = 1;
  D_L00_0015F650 = 1;
 }
 return 0;
}
