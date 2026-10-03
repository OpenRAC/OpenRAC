/* NON_MATCHING func_L03_002D5650 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 320 / retail 316, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby class 0x341, fills its data block, calls func_L00_001EFFF0 with a stack vec and resets on hit.
 *   Best p5.c (320 vs 316 bytes): left diffs are lw of moby+0x78 placed one slot early, store order of d+0x24/d+0x
 *   Unblock: declare the stack vec as a different type (float[4] with qcopy-free copy) so a0 is plain sp.
 */
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern float func_L00_001FF860(float, float);
extern int func_001F9850(int);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern void func_L00_00251E30(void *);
extern char D_L03_00173FE0[];

typedef int u128 __attribute__((mode(TI)));

// Spawns a moby of class 0x341 at a position, initialises its data and registers it.
char *func_L03_002D5650(char *a, char *pos, char *parent, int n, float f0, float f1, float f2) {
    char *m = (char *)func_0020D348_m(0x341);
    char *d;
    if (m != 0) {
        u128 tmp;
        *(unsigned char *)(m + 0x30) = 0xFF;
        *(short *)(m + 0x32) = 0x7E;
        m[0x20] = 1;
        d = *(char **)(m + 0x78);
        m[0x31] = 1;
        qcopy(m + 0x10, pos);
        qcopy(d, a);
        *(float *)(m + 0x48) = func_L00_001FF860(*(float *)d, *(float *)(d + 4));
        *(int *)(d + 0x14) = func_001F9850(n);
        *(float *)(d + 0x18) = f2;
        *(float *)(d + 0x1C) = f0;
        *(float *)(d + 0x24) = f1;
        *(char **)(d + 0x10) = parent;
        *(int *)(d + 0x28) = 0;
        tmp = *(u128 *)(parent + 0x10);
        *(float *)((char *)tmp + 8) = *(float *)(pos + 8);
        if (func_L00_001EFFF0(tmp, pos, 2, parent, 0) != 0) {
            *(int *)(d + 0x14) = 0;
            *(u128 *)(m + 0x10) = *(u128 *)D_L03_00173FE0;
            *(int *)(d + 0x24) = 0;
        }
        func_L00_00251E30(m);
    }
    return m;
}
