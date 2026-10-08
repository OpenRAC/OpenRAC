/* NON_MATCHING func_L03_00293EB8 -- src/overlays/l03_kerwan/vendor_00293720.c
 * Best so far: SIZE ours 388 / retail 392, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates linked moving-object transforms, snapshots their positions/rotations, and marks the final associated o
 *   Budget exhausted at eight runs. p5/p6 match 392-byte size and trailing logic but save initial pointers too ear
 *   A pointer/snapshot lifetime form preserving initial temporary pointers until the successful branch would unblo
 */
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern int func_001E9730();
extern void func_0020D678(void *);
extern char *D_L03_00160058 MACRO_ADDR;
extern char D_L03_001E2700[];
// Updates linked moving-object transforms and their previous-frame snapshots.
void func_L03_00293EB8(char *moby) {
 float old_position[4], old_rotation[4], delta[4];
 char *data = *(char **)(moby + 0x78);
 char *position = moby + 0x10;
 char *rotation = moby + 0x40;
 char *update;
 int id;
 moby[0x72] = 0x40;
 qcopy(old_position, position);
 qcopy(old_rotation, rotation);
 id = *(int *)(data + 0xA0);
 if (id == -1) {
  func_001E9730(D_L03_001E2700, *(int *)(data + 0xB8));
  func_0020D678(moby);
  return;
 }
 update = data + 0x60;
 {
  do {
   char *other = D_L03_00160058 + (id << 8);
   char *other_data = *(char **)(other + 0x78);
   char *other_position = other + 0x10;
   char *previous_position = other_data + 0xA0;
   char *other_rotation = other + 0x40;
   char *previous_rotation = other_data + 0xB0;
   other[0x72] = 0x40;
   func_001F9BF0(delta, other_position, previous_position);
   func_L00_002617B0(other_data + 0x20, delta, previous_rotation, other_rotation);
   qcopy(previous_position, other_position);
   qcopy(previous_rotation, other_rotation);
   id = *(int *)(other_data + 0xC0);
  } while (id != -1);
  func_001F9BF0(delta, position, old_position);
  func_L00_002617B0(update, delta, old_rotation, rotation);
  {
   char *other = D_L03_00160058 + (*(int *)(data + 0xC4) << 8);
   *(unsigned short *)(other + 0x34) |= 6;
  }
 }
}
