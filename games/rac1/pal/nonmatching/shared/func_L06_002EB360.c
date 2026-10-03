/* NON_MATCHING func_L06_002EB360 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: SIZE ours 608 / retail 616, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns two rings of debris around a moby (4 then 8 particles via func_L00_002B0B98), then deletes the owner mo
 *   Best p1.c: 608 vs 616 bytes; D_L06_0015F660 needs MACRO_ADDR on the array decl to stop the symbol being hoiste
 */
typedef int u128 __attribute__((mode(TI)));
extern int func_0022ED80(int, int, int);
extern float func_002140F8(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern unsigned char *func_L00_002B0B98(unsigned char *owner, void *vel, void *pos, void *rot, int type, int n, float lo, float hi);
extern int func_002140B0(int);
extern void func_0020D678(void *);
extern float D_0015EE6C MACRO_ADDR;
extern char D_L06_0015F660[] MACRO_ADDR;

// Bursts two rings of debris from a moby, then deletes the owner reference it holds.
void func_L06_002EB360(char *moby) {
    float p[4];
    float A[4];
    float B[4];
    float tmp[4];
    float *a1, *b1, *a2, *b2;
    float r;
    int i, j;
    char *d = *(char **)(moby + 0x78);
    func_0022ED80(4, 0, (int)moby);
    qcopy(p, moby + 0x10);
    p[2] = p[2] + 0.5f;
    a1 = A;
    b1 = B;
    i = 3;
    do {
        r = func_002140F8(1.0f, 4.0f) * D_0015EE6C;
        *(u128 *)tmp = 0;
        tmp[0] = func_002140F8(-0.625f, 0.625f);
        tmp[1] = func_002140F8(-0.625f, 0.625f);
        tmp[2] = func_002140F8(0.0f, 1.25f);
        *(u128 *)B = *(u128 *)tmp;
        func_L00_001FF4B0(a1, b1, r);
        func_001F9BD8(b1, b1, p);
        func_L00_002B0B98(*(unsigned char **)(d + 0x20C), a1, b1, D_L06_0015F660, 0x165, 0, 0.3f, 0.6f);
    } while (--i >= 0);
    a2 = A;
    b2 = B;
    j = 7;
    do {
        r = func_002140F8(1.0f, 4.0f) * D_0015EE6C;
        *(u128 *)tmp = 0;
        tmp[0] = func_002140F8(-0.625f, 0.625f);
        tmp[1] = func_002140F8(-0.625f, 0.625f);
        tmp[2] = func_002140F8(0.0f, 1.25f);
        *(u128 *)B = *(u128 *)tmp;
        func_L00_001FF4B0(a2, b2, r);
        func_001F9BD8(b2, b2, p);
        func_L00_002B0B98(*(unsigned char **)(d + 0x20C), a2, b2, D_L06_0015F660, 0x166 + func_002140B0(2), 0, 0.5f, 0.7f);
    } while (--j >= 0);
    func_0020D678(*(void **)(d + 0x20C));
    *(int *)(d + 0x20C) = 0;
}
