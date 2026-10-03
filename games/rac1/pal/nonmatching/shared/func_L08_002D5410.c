/* NON_MATCHING func_L08_002D5410 -- src/overlays/shared/vendor_002D3DF8.c
 * Best so far: SIZE ours 468 / retail 464, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a 0x90-byte effect struct on stack (4 scaled vectors + colour words) and submits it via func_L00_001FD1
 *   p3 is close (468 vs 464 bytes): compiler hoists &gp float (addiu $s4,$gp) out of the loop where retail reloads
 *   each iteration, and orders the colour-word stores/D_L08_001D2050 lui differently; extra ra save position. Need
 */
typedef int u128_2D5410 __attribute__((mode(TI)));
extern int func_001F4868(int);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L08_00167640[];
extern char D_L08_001D2050;
extern short D_L08_00161928;

// Builds and submits a lit quad effect for a moby from its position.
void func_L08_002D5410(char *moby)
{
    struct {
        float q[16];
        int col[4];
        float f50[8];
        long l70, l78, l80, l88;
    } s;
    float z[4];
    float a[4], b[4], c[4], d[4], e[4];
    char *p;
    char *src;
    float *dst;
    int i;
    int cv;

    *(u128_2D5410 *)z = 0;
    z[2] = 1.0f;
    z[3] = 1.0f;
    p = *(char **)(moby + 0x78);
    s.l78 = func_001F4868(11);
    s.l88 = 0x80000048L;
    s.l80 = 0xFF90000000000260L;
    s.l70 = 5;
    s.f50[0] = 0; s.f50[1] = 0; s.f50[2] = 0;
    s.f50[3] = 1.0f; s.f50[4] = 1.0f; s.f50[5] = 0;
    s.f50[6] = 1.0f; s.f50[7] = 1.0f;
    qcopy_dummy:
    qcopy(e, p + 0x220);
    func_001F9BF0(a, D_L08_00167640, e);
    i = 3;
    func_L00_001FF4B0(a, a, 1.0f);
    func_001F9CA0(b, a, z);
    func_L00_001FF4B0(b, b, 1.0f);
    func_001F9CA0(c, b, a);
    func_001F9C30(d, a, *(float *)&D_L08_00161928);
    func_001F9BD8(e, e, d);
    cv = (*(int *)(moby + 0x90) & 0xFFFFFF) | 0x40000000;
    s.col[0] = cv;
    s.col[3] = cv;
    s.col[2] = cv;
    s.col[1] = cv;
    src = &D_L08_001D2050;
    dst = s.q;
    for (; i >= 0; i--) {
        func_001F9C30(dst, src, *(float *)((char *)&D_L08_00161928 + 4));
        src += 16;
        func_001F9EE8(dst, dst, a);
        dst += 4;
    }
    func_L00_001FD1D8(&s, 0, 0);
}
