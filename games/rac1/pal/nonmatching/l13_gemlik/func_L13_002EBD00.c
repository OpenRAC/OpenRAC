/* NON_MATCHING func_L13_002EBD00 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: SIZE ours 828 / retail 824, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Best candidate p6.c (836 bytes, retail 824). It builds a GS packet, places the moby, then loops over the path 
 *   Wall: none found. Saved-register pressure (vector pointers hoisted across calls) is the main gap; p7 (declarin
 *   Budget: 10 runs spent (p0-p8 plus one reorder).
 */
extern int func_001F4868(int);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF548(void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001FA888(int);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_001FD1D8(void *, void *, int);
extern int D_L13_0015F6B0 MACRO_ADDR;
extern char D_L13_00160700[] MACRO_ADDR;

// Gemlik vendor moby: builds a GS packet, places the moby and steps its path along the loop.
void func_L13_002EBD00(char *moby) {
    char *data;
    u64 gs[4];
    int rgba[4];
    float fa[4];
    float fb[4];
    char v4[4][16];
    char vA[16];
    char vB[16];
    char vC[16];
    char vD[16];
    char vE[16];
    char vF[16];
    char vG[16];
    float f20, f21, f22, f23, f24;
    int r;
    int col;

    data = *(char **)(moby + 0x78);
    gs[1] = func_001F4868(0x34);
    f23 = 0.0f;
    f22 = 1.0f;
    gs[2] = 0xFF9000000260ULL;
    gs[0] = 4;
    gs[3] = 0x8000000048ULL;
    rgba[0] = 0x40808080;
    rgba[1] = 0x40808080;
    rgba[2] = 0x40808080;
    rgba[3] = 0x40808080;
    fa[0] = f23;
    fa[1] = f23;
    fa[2] = f22;
    fa[3] = f23;
    fb[0] = f23;
    fb[1] = f22;
    fb[2] = f22;
    fb[3] = f22;

    func_L00_00250800(moby, 0x13, vA);
    f20 = 100.0f;
    func_001F9BF0(vB, data + 0x80, vA);
    func_L00_001FF548(vB, vB, f20);
    f21 = func_001F9CB8(vB);
    f20 = f21 / f20;
    f24 = f20;
    if (f20 < f22) {
        f24 = f22;
    }
    func_L00_001FF4B0(vB, vB, f24);
    qcopy(vC, vA);
    f22 = f23;
    func_L00_001FF4B0(vD, vB, func_001FA888(D_L13_0015F6B0 % 20) / 20.0f);
    func_001F9BF0(vC, vC, vD);

    while (f22 < f21) {
        f20 = 0.0f;
        f20 = f22 * f20 / (f21 + f21) + 0.5f;
        func_001F9CA0(vE, vB, D_L13_00160700);
        func_L00_001FF4B0(vE, vE, f20);
        func_001F9CA0(vF, vB, vE);
        func_L00_001FF4B0(vF, vF, f20);
        func_001F9BF0(vG, vC, vF);
        func_001F9BF0(v4[0], vG, vE);
        func_001F9BD8(v4[1], vG, vE);
        func_001F9BD8(vG, vC, vF);
        func_001F9BF0(v4[2], vG, vE);
        func_001F9BD8(v4[3], vG, vE);
        if (f23 < f21) {
            if (f21 - f23 < f22) {
                r = func_001FA898_r((f21 - f22) * 6.4f);
                col = (r << 24) | 0x808080;
                rgba[0] = col;
                rgba[1] = col;
                rgba[2] = col;
                rgba[3] = col;
            }
        }
        func_L00_001FD1D8(v4[0], 0, 1);
        f22 += f24;
        func_001F9BD8(vC, vC, vB);
    }
}
