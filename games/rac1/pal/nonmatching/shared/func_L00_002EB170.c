/* NON_MATCHING func_L00_002EB170 -- src/overlays/shared/vendor_002EB0D8.c
 * Best so far: BYTES 2/268 (99.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a view matrix (func_L00_001FF4B0), scales it by a per-mode factor (mode byte at g+0x20A4: 2 -> 9.5, 1 -
 *   EXACT except 2 instructions: the `lbu` of D_0015EEB0+3 uses $v1 in ours, $v0 in retail (register allocator tie
 *   Would need some other way to free $v1 for that load; not found.
 *   Wave lb1 p12: Lombyte's port (p5/p6, char a[16] aligned, p += 0x30, unsigned char flag), a local copy of the f
 */
extern char D_0013E633[] NOT_SDA;
extern unsigned char D_0015EEB0[] NOT_SDA;
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
/* Builds a view matrix and multiplies it into the camera and projection matrices. Adapted from Lombyte (MIT) for PAL: overlays/shared/gameplay_vendor_002e9010.c, FUN_L00_002e9cc0. */
void func_L00_002EB170(char *p) {
    char a[16] __attribute__((aligned(16)));
    char b[16] __attribute__((aligned(16)));
    char *g = D_0013E633 + 0xE1D;
    int m;
    func_L00_001FF4B0(a, *(char **)(g + 0x2080) + 0xE0, 1.0f);
    m = *(unsigned char *)(g + 0x20A4);
    if (m == 2) {
        func_001F9C30(b, a, 9.5f);
    } else if (m == 1 && D_0015EEB0[3]) {
        func_001F9C30(b, a, 1.2f);
    } else if (m == 1) {
        func_001F9C30(b, a, 0.9f);
    } else {
        func_001F9C30(b, a, 1.6f);
    }
    p += 0x30;
    func_001F9BD8(p, D_0013E633 + 0xE9D, b);
    func_001F9BD8(p, D_0013E633 + 0xE9D + 0xC0, p);
}
