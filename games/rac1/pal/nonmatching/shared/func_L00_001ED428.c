/* NON_MATCHING func_L00_001ED428 -- src/overlays/shared/camera_001EB508.c
 * Best so far: SIZE ours 388 / retail 392, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Updates camera state, copies transforms, and calls camera interpolation helpers.
 *   p1 uses actual independent vector globals, 404/392 bytes, and matches through 0x110. Remaining extra lui loads
 *   p2 uses prohibited cross-global region offsets and must not be landed. Lead instructed stop when independent v
 */
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_L00_001CA800;
extern char D_L00_00166FF0[];
extern S D_L00_00166D80;
extern int D_L00_0016C158[];
extern char D_L00_001670D0[];
extern char D_L00_00166EE0[];
extern char D_L00_00166ED0[];
extern char D_L00_00166EC0_c[] __asm__("D_L00_00166EC0");
extern char D_L00_001670E0[];
extern char D_L00_001670F0[];
extern unsigned char D_0015EEB4;
extern void func_001EDE08(void);
extern void func_L00_001ED2C0(void);
extern void func_001ED818(void);
extern void func_L00_001EBDA0(void);
extern void func_L00_001EC220(void *);
extern void func_001ED658(void *);
extern void func_001FA460(void *, void *);
extern void func_002153E8(void *, void *);
extern void func_001ED708(void *, int);
extern void func_001EDB98(void);
extern void func_001EE858(void *);
extern void func_001F9CA0(void *, void *, void *);
// Updates camera state, transform, and interpolation vectors.
void func_L00_001ED428(void) {
 char scratch[0x40];
 char *g;
 char *camera;
 if (D_L00_0015F6A8 == 5) {
  if (D_L00_001CA800) return;
  *(short *)D_L00_00166FF0 = 0;
  D_L00_00166FF0[2] = 0;
 }
 g = (char *)&D_L00_00166D80;
 (*(int *)(g + 0x398))++;
 func_001EDE08();
 func_L00_001ED2C0();
 func_001ED818();
 func_L00_001EBDA0();
 camera = *(char **)(g + 0x180);
 if ((unsigned int)(*(unsigned short *)(g + 0x270) - 1) < 2) func_L00_001EC220(*(char **)(g + 0x184));
 if (*(short *)(g + 0x270) == 3) func_001ED658(camera);
 else if (!D_L00_0016C158[5]) {
  qcopy(g + 0x140, camera + 0x30);
  qcopy(g + 0x350, camera);
  qcopy(g + 0x360, camera + 0x10);
  qcopy(g + 0x370, camera + 0x20);
 } else goto finish;
 if (!D_L00_0016C158[5]) {
  char *transform = D_L00_001670D0;
  func_001FA460(scratch, transform);
  func_002153E8(scratch, transform - 0x200);
 }
finish:
 g = D_L00_00166EE0;
 func_001ED708(g, 0);
 func_001ED708(g + 0x10, 1);
 func_001EDB98();
 func_001EE858(g - 0x20);
 if (D_0015EEB4) func_001F9CA0(g + 0x200, g + 0x210, g + 0x1F0);
}
