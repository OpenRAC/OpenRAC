/* NON_MATCHING func_L03_002BC038 -- src/overlays/l03_kerwan/vendor_00293720.c
 * Best so far: BYTES 6/264 (97.7% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns class 0xEB moby bound to arg0 (extra unused int arg at $5; p5.c), copies pos/vel, sets data fields, cal
 *   p5.c is 16 bytes off: only scheduling of the final stores to data (0xE/0x14/0xC/0x4) and sra 8/16 order for fu
 *   Unblock: other order of the c>>16 / c>>8 args, store permutations.
 *   q28 t13: p7.c is BYTES 6/264 (best). A local `bl = (c>>16)&0xFF` assigned before the call fixed the sra a2/a3 
 */
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern float func_00214158(void);
extern void func_L00_00251328(void *, int, int, int);
extern void func_L00_00251E30(void *);
extern void qcopy(void *, void *);
typedef int u128_2BC038 __attribute__((mode(TI)));
// Spawns a class 0xEB moby attached to another and initialises its data.
void *func_L03_002BC038(char *a, float f1, float f2, int b, int c, int d, int e) {
    char *m = (char *)func_0020D348_m(0xEB);
    char *p;
    int bl;
    if (m != 0) {
        *(unsigned char *)(m + 0x30) = 0xFF;
        p = *(char **)(m + 0x78);
        *(short *)(m + 0x32) = 0x7F;
        m[0x31] = 1;
        *(float *)(m + 0x2C) = *(float *)(m + 0x2C) * f2;
        *(char **)p = a;
        qcopy(m + 0x10, *(char **)(a + 0x78) + 0xF0);
        *(u128_2BC038 *)(m + 0x40) = *(u128_2BC038 *)(a + 0x40);
        *(float *)(m + 0x40) = func_00214158();
        *(int *)(p + 8) = d;
        *(float *)(p + 0x14) = f1;
        *(short *)(p + 0xC) = e;
        *(int *)(p + 4) = c;
        *(short *)(p + 0xE) = e;
        bl = (c >> 16) & 0xFF;
        func_L00_00251328(m, c & 0xFF, (c >> 8) & 0xFF, bl);
        func_L00_00251E30(m);
    }
    return m;
}
