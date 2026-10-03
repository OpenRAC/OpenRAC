/* NON_MATCHING func_L14_002ECC70 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 532 / retail 540, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws up to 48 table entries (flag shorts at data+0x320) as lit, tinted vectors: builds a stack block (char L[
 *   Best p4.c: SIZE 532 vs 540. Left: retail keeps a separate copy $fp = s3 (the L+0x90 vector) used as 3rd arg of
 *   Would unblock: a wording that keeps the copy pointer live (copy-prop folds `w = a`); the original probably use
 */
typedef int u128 __attribute__((mode(TI)));
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L14_001675C0[];
extern char D_L14_001E0090[];
extern short D_L14_00161CDC;

/* Draws each active entry of a 48-slot table of lit, tinted vectors for an Oltanis moby. */
void func_L14_002ECC70(char *moby) {
    char L[0xE0];
    char *data = *(char **)(moby + 0x78);
    char *dstp;
    char *q;
    char *v;
    char *t;
    int i, j, col, o;
    char *w, *a, *b;
    *(long *)(L + 0x78) = func_001F4868(0x30);
    *(long *)(L + 0x80) = 0xFF9000000260L;
    *(long *)(L + 0x88) = 0x8000000048L;
    *(long *)(L + 0x70) = 0;
    func_00234C98(0x4A, 0);
    func_00234C98(0x47, 0x51001);
    dstp = L + 0xC0;
    a = L + 0x90;
    b = L + 0xA0;
    *(int *)(L + 0x58) = 0;
    *(float *)(L + 0x50) = 1.0f;
    *(float *)(L + 0x54) = 1.0f;
    *(float *)(L + 0x5C) = 1.0f;
    *(float *)(L + 0x60) = 1.0f;
    *(int *)(L + 0x64) = 0;
    *(int *)(L + 0x68) = 0;
    *(int *)(L + 0x6C) = 0;
    for (i = 0; i < 0x30; i++) {
        if (*(short *)(data + i * 2 + 0x320) != 0) {
            col = *(int *)&D_L14_00161CDC | (*(short *)(data + i * 2 + 0x3E0) << 24);
            *(int *)(L + 0x40) = col;
            *(int *)(L + 0x4C) = col;
            *(int *)(L + 0x48) = col;
            *(int *)(L + 0x44) = col;
            o = i * 16;
            qcopy(dstp, o + data + 0x20);
            q = data + o;
            *(u128 *)(L + 0xD0) = 0;
            *(float *)(L + 0xD8) = 1.0f;
            *(float *)(L + 0xDC) = 1.0f;
            func_001F9BF0(a, data + (o + 0x20), D_L14_001675C0);
            w = a + 0;
            func_L00_001FF4B0(a, a, 1.0f);
            func_001F9CA0(b, a, L + 0xD0);
            func_L00_001FF4B0(b, b, 1.0f);
            func_001F9CA0(L + 0xB0, b, a);
            v = L;
            t = D_L14_001E0090;
            for (j = 3; j >= 0; j--) {
                func_001F9C30(v, t, *(float *)(q + 0x2C));
                t += 0x10;
                func_001F9EE8(v, v, w);
                v += 0x10;
            }
            func_L00_001FD1D8(L, 0, 0);
        }
    }
}
