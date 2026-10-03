/* NON_MATCHING func_L09_0030BB28 -- src/overlays/shared/vendor_002C6B30.c
 * Best so far: SIZE ours 616 / retail 620, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a child moby (func_L00_0025A208 into stack slot `n`), copies pos in, inits fields from its template at 
 *   Best p2.c (612 vs 620 bytes; same as p5): retail keeps moby in $s0 copy with res in $s2 and branches `bnez` on
 *   The last difference is the tail `sh 0,0x68(m)` ordering in the func_L00_002DCD40 block (one store after the ot
 */
extern int func_L00_0025A208(int *, int, int, int);
extern void func_00213D28(void *, int, int);
extern char *func_L00_002DCD40(char *);
extern void func_L00_0025E210(void *);
extern void func_L00_00251E30(void *);
extern char *func_L00_0025D390(char *);
extern int func_001E9730();
extern char D_L09_00209570[];
extern int D_L09_0015F6B0 MACRO_ADDR;

// Spawns a child moby from a template at the given position, splitting the parent's counter between them.
char *func_L09_0030BB28(char *moby, void *pos) {
    float v[4];
    char *res;
    char *d;
    char *n;
    char *m;
    char *p;
    int t;
    qcopy(v, pos);
    if (moby == 0) return 0;
    d = *(char **)(moby + 0x78);
    res = 0;
    n = 0;
    if (func_L00_0025A208((int *)&n, *(int *)(d + 0x94), 1, 1) != 0) {
        func_001E9730(D_L09_00209570, *(short *)(moby + 0xB2), D_L09_0015F6B0);
    } else {
        d[0x91] = d[0x91] + 1;
        qcopy(n + 0x10, v);
        *(int *)(n + 0x1C) = 0;
        n[0x20] = 0;
        n[0xBC] = 0;
        *(unsigned short *)(n + 0x34) = *(unsigned short *)(*(char **)(n + 0x24) + 0x44);
        *(float *)(n + 0x2C) = *(float *)(*(char **)(n + 0x24) + 0x24);
        *(unsigned char *)(n + 0x30) = 0xFF;
        *(short *)(n + 0x32) = 0xFF;
        n[0x31] = 1;
        if (*(int *)(*(char **)(n + 0x24) + 0x40) != 0) *(unsigned short *)(n + 0x34) |= 0x10;
        if (*(unsigned char *)(*(char **)(n + 0x24) + 0xF) != 0) *(unsigned short *)(n + 0x34) |= 0x400;
        *(short *)(n + 0x36) = 0x7F80;
        *(unsigned char *)(n + 0x71) = 0xFF;
        *(unsigned char *)(n + 0x72) = 0xFF;
        *(unsigned char *)(n + 0xA4) = 0xFF;
        if (*(unsigned char *)(d + 0x93) != 0) *(float *)(n + 0x2C) = *(float *)(*(char **)(n + 0x24) + 0x24) * 0.1f;
        func_00213D28(n, 0, 0);
        m = func_L00_002DCD40(n);
        if (m != 0) {
            *(int *)(m + 0x6C) = 0;
            *(int *)(m + 0x94) = 0;
            *(short *)(m + 0x68) = 0;
        }
        if (*(short *)(moby + 0xB4) < *(short *)(n + 0xB4)) *(short *)(n + 0xB4) = *(unsigned short *)(moby + 0xB4);
        if (*(short *)(n + 0xB4) <= 0) *(short *)(n + 0xB4) = 1;
        t = *(unsigned short *)(moby + 0xB4) - *(unsigned short *)(n + 0xB4);
        *(short *)(moby + 0xB4) = t;
        if ((short)t < 0) *(short *)(moby + 0xB4) = 1;
        *(int *)(n + 0x94) = *(int *)(*(char **)(n + 0x24) + 0x10);
        func_L00_0025E210(n);
        func_L00_00251E30(n);
        res = n;
        p = func_L00_0025D390(n);
        if (p != 0) {
            *(int *)(p + 0x34) = 0;
            *(int *)(p + 0x30) = 0;
            *(float *)p = *(short *)(p + 4);
        }
    }
    return res;
}
