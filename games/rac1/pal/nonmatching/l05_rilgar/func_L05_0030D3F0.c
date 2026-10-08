/* NON_MATCHING func_L05_0030D3F0 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: BYTES 6/272 (97.8% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L05_00160098_t __asm__("D_L05_00160098") MACRO_ADDR;
extern int func_L00_002616E0_u(char *, char *, void *, void *, void *, void *) __asm__("func_L00_002616E0");
extern int func_L00_00261568_u(char *, char *, void *, void *, void *, void *) __asm__("func_L00_00261568");
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_002617B0(char *a, void *b, void *c, void *d);

/* UpdateMoby_812 (names.tsv role). State 0: marks the data block (+0x5C = 1) and, when its
 * entry index at +0x80 is valid, runs func_L00_002616E0 for that entry and goes to state 1.
 * State 1: remembers the moby's position and velocity quads, runs func_L00_00261568, and feeds
 * the position change to func_L00_002617B0. */
void func_L05_0030D3F0(unsigned char *m) {
    char *d = *(char **)(m + 0x78);
    char *p;
    char *q;
    float v0[4] __attribute__((aligned(16)));
    float v10[4] __attribute__((aligned(16)));
    float v20[4] __attribute__((aligned(16)));
    switch (m[0x20]) {
    case 0:
        *(int *)(d + 0x5C) = 1;
        if (*(int *)(d + 0x80) >= 0) {
            func_L00_002616E0_u((char *)m, D_L05_00160098_t + (*(int *)(d + 0x80) << 8), (char *)m + 0x10, (char *)m + 0x40, d + 0x60, d + 0x70);
            m[0x20] = 1;
        }
        break;
    case 1:
        p = (char *)m + 0x10;
        qcopy(v0, p);
        q = (char *)m + 0x40;
        qcopy(v10, q);
        func_L00_00261568_u((char *)m, D_L05_00160098_t + (*(int *)(d + 0x80) << 8), d + 0x60, d + 0x70, p, q);
        func_001F9BF0(v20, p, v0);
        func_L00_002617B0(d + 0x20, v20, v10, q);
        break;
    }
}
