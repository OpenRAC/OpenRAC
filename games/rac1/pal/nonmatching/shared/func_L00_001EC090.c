/* NON_MATCHING func_L00_001EC090 -- src/overlays/shared/camera_001EB508.c
 * Best so far: SIZE ours 220 / retail 216, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Samples three axis vectors of the active camera (func_L00_001FF4B0, 1.0f), copies two into D_L00_00166FF0+0x90
 *   p1.c compiles to the same instruction sequence as retail (checked by hand, 55 words vs 54): the only differenc
 *   Unblock: the lead should merge func_00219778 (4 bytes, exe/shared) into this function's size (220) in function
 */
extern char D_0013E633[];
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001EC8D8(float *out, void *p0, void *p1, void *dir0, void *dir1, void *axis);
extern char D_L00_00166FF0[];

// Samples three axes of the active camera, stores two and derives the look-at vector from them.
void func_L00_001EC090(void) {
    float a[4];
    float b[4];
    float c[4];
    char *g = D_0013E633 + 0xE1D;
    char *d = D_L00_00166FF0;

    func_L00_001FF4B0(a, *(char **)(g + 0x2080) + 0xC0, 1.0f);
    func_L00_001FF4B0(b, *(char **)(g + 0x2080) + 0xD0, 1.0f);
    func_L00_001FF4B0(c, *(char **)(g + 0x2080) + 0xE0, 1.0f);
    qcopy(d + 0x90, a);
    qcopy(d + 0xA0, c);
    func_001EC8D8((float *)(d + 0x70), d + 0xC0, g + 0x80, a, b, c);
    qcopy(d + 0xB0, d + 0xD0);
}
