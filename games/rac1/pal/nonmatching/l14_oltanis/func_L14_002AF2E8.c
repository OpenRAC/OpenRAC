/* NON_MATCHING func_L14_002AF2E8 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 444 / retail 440, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a transform/lighting block on the stack for an Oltanis moby (one char[0x100] local) and draws it via fu
 *   Best is p3.c (SIZE 444 vs 440). Remaining: ours turns the 0x7F40407F/0x7F7F7F7F choice (lh 0x202(data)) into m
 *   so data stays live in ours (extra saved reg s4, q cannot reuse s2); also the swc1 0xF8 store/jal order and the
 *   Would unblock: a wording that keeps the branch (all if/else, ternary, default+override forms gave movn); likel
 */
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_L00_001FF860(float, float);
extern float func_001F9CE8(void *);
extern void func_001FA1F8(void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L14_001675C0[];
extern char D_L14_001D8900[];
extern short D_L14_00161540;
extern short D_L14_0016153C;

/* Builds the transform and lighting block for an Oltanis moby and draws it. */
void func_L14_002AF2E8(char *moby) {
    char L[0x100];
    char *data = *(char **)(moby + 0x78);
    char *v;
    char *q;
    float f;
    int i;
    long r;
    int col;
    qcopy(L + 0x30, data + 0xD0);
    *(float *)(L + 0x3C) = 1.0f;
    func_001F9C30(L + 0x40, data + 0xA0, *(float *)&D_L14_00161540);
    func_001F9BD8(L + 0x30, L + 0x30, L + 0x40);
    r = func_001F4868(0xB);
    *(long *)(L + 0xC8) = r;
    *(long *)(L + 0xD0) = 0xFF9000000260L;
    *(long *)(L + 0xD8) = 0x8000000048L;
    *(long *)(L + 0xC0) = 0;
    func_00234C98(0x47, 0x51001);
    func_001F9BF0(L + 0xE0, D_L14_001675C0, L + 0x30);
    f = func_L00_001FF860(*(float *)(L + 0xE0), *(float *)(L + 0xE4));
    *(float *)(L + 0xF8) = f;
    f = func_L00_001FF860(func_001F9CE8(L + 0xE0), *(float *)(L + 0xE8));
    *(float *)(L + 0xF4) = -f;
    *(int *)(L + 0xF0) = 0;
    func_001FA1F8(L, L + 0xF0);
    *(float *)(L + 0xB0) = 1.0f;
    *(float *)(L + 0xA0) = 1.0f;
    *(float *)(L + 0xA4) = 1.0f;
    *(int *)(L + 0xA8) = 0;
    *(float *)(L + 0xAC) = 1.0f;
    *(int *)(L + 0xB4) = 0;
    *(int *)(L + 0xB8) = 0;
    *(int *)(L + 0xBC) = 0;
    col = *(short *)(data + 0x202) ? 0x7F40407F : 0x7F7F7F7F;
    *(int *)(L + 0x90) = col;
    *(int *)(L + 0x9C) = col;
    *(int *)(L + 0x98) = col;
    *(int *)(L + 0x94) = col;
    v = L + 0x50;
    q = D_L14_001D8900;
    for (i = 3; i >= 0; i--) {
        func_001F9C30(v, q, *(float *)&D_L14_0016153C);
        q += 0x10;
        func_001F9EE8(v, v, L);
        v += 0x10;
    }
    func_L00_001FD1D8(L + 0x50, 0, 0);
}
