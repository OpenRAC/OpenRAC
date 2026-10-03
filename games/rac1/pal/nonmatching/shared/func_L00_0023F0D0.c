/* NON_MATCHING func_L00_0023F0D0 -- src/overlays/shared/initonce_0023E4C8.c
 * Best so far: SIZE ours 256 / retail 252, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Point-light slot allocator: returns -1 if D_L00_0015F6B8 > 0.8f, scans 8 slots (0x30 stride, flag word at +0x1
 *   fills the 0x20-byte record at D_L00_001803C0 (qcopy pos to +0x10, five floats), clears/inits the 0x30 record, 
 *   Best: p2.c (pointer t = D_L00_001805C0 walked in a for loop): same instructions as retail, 248 vs 252 bytes. T
 *   of D_L00_001805C0, but retail (a) keeps pos in $a2 and the lui copy in $a1 (ours swapped), (b) loads the float
 *   (c) strength-reduces the loop to read 0($v1) after an addiu 0x10 (ours reads 0x10($v1) in the loop). Register-
 *   (array index, struct array, while loop, int* walk) did not move them. Unblock: find the source shape that make
 */
extern void func_001F99D8(void *, int);
extern unsigned char D_L00_001805C0[];
extern unsigned char D_L00_001803C0[];
extern float D_L00_0015F6B8;
extern char D_L00_00180740[];

// allocates a point light slot and fills it in; returns its index or -1
int func_L00_0023F0D0(void *pos, float a, float b, float c, float d, float e) {
    int i;
    char *q;
    char *p;
    int *t;
    if (0.8f < D_L00_0015F6B8) return -1;
    i = 0;
    t = (int *)(D_L00_001805C0 + 0x10);
    while (i < 8 && *t != 0) {
        t += 12;
        i++;
    }
    if (i == 8) return -1;
    p = (char *)D_L00_001803C0 + i * 0x20;
    qcopy(p + 0x10, pos);
    *(float *)(p + 0xC) = b;
    *(float *)(p + 0x1C) = a;
    *(float *)(p + 0) = c;
    *(float *)(p + 4) = d;
    *(float *)(p + 8) = e;
    q = (char *)D_L00_001805C0 + i * 0x30;
    func_001F99D8(q, 0x30);
    *(int *)(q + 0x10) = 1;
    *(char **)(q + 0xC) = D_L00_00180740 + i * 0x400;
    return i;
}
