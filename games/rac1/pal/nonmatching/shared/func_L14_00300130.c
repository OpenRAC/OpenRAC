/* NON_MATCHING func_L14_00300130 -- src/overlays/shared/vendor_002B2A28.c
 * Best so far: SIZE ours 596 / retail 588, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns two bursts of debris particles (func_L00_0026DEA0) at a moby's position: 2 iterations with random sign,
 *   Best (p2.c, 121/588 bytes diff-counted; registers match): only the scheduling differs: retail puts `lw $a3,-0x
 *   Unblock: some form that makes the scheduler leave the gp load last; three wordings tied.
 */
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_001F9850(int);
extern short D_L14_0016209C;
extern short D_L14_001620A0;
extern short D_L14_001620A4;
extern short D_L14_001620A8;
extern short D_L14_001620B4;
extern char D_L14_0015F660[];

/* Spawns two bursts of debris particles at the moby's position. */
void func_L14_00300130(char *moby) {
    float a[4];
    float b[4];
    char *p;
    char *q;
    int i;
    int n;
    int spd;
    float f;
    func_001F9C30(a, moby + 0xC0, *(float *)&D_L14_0016209C);
    func_001F9BD8(b, moby + 0x10, a);
    func_001F9C30(a, moby + 0xE0, *(float *)&D_L14_001620A0);
    func_001F9BD8(b, b, a);
    for (i = 1; i >= 0; i--) {
        int x = func_002140B0(0x10);
        int y = func_002140B0(2);
        spd = -x;
        if (y == 0) spd = x;
        p = func_L00_0026DEA0(b, spd, D_L14_0015F660, *(int *)&D_L14_001620B4, 0.2f, 1.0f, 0.9f, 100000.0f);
        if (p != 0) {
            q = p + 0x20;
            *(short *)(p + 0xA) = func_001F9850(0xC);
            *(int *)(q + 4) = 2;
            q[0xA] = 0x7F;
            q[0xB] = p[0xA];
        }
    }
    func_001F9C30(a, moby + 0xC0, *(float *)&D_L14_001620A4);
    func_001F9BD8(b, moby + 0x10, a);
    spd = 0x10;
    func_001F9C30(a, moby + 0xE0, *(float *)&D_L14_001620A8);
    func_001F9BD8(b, b, a);
    n = func_001F9850(2);
    f = 80000.0f;
    for (i = 2; i >= 0; i--) {
        p = func_L00_0026DEA0(b, spd, D_L14_0015F660, 0x7FFFFFFF, 0.05f, 1.0f, 1.0f, f);
        spd = -spd;
        f -= 20000.0f;
        if (p != 0) {
            q = p + 0x20;
            *(short *)(p + 0xA) = n;
            p[8] = func_002140B0(0xFF);
            *(int *)(q + 4) = 2;
            q[0xA] = 0x7F;
            q[0xB] = p[0xA];
        }
        n <<= 1;
    }
}
