/* NON_MATCHING func_L01_0030D380 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: SIZE ours 480 / retail 488, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a 4-vector basis from the moby's offsets (func_001F9C30/9BD8/9BF0/9CA0) and submits a stack DrawSet via
 *   p2.c is closest (72/488 bytes matching, same size class): sd constant stores now in retail order; left are ret
 *   p3/p4 (explicit pointer copy) shrank the function by 8 bytes instead; would need the right place/way to introd
 */
typedef int u128_30D380 __attribute__((mode(TI)));
typedef struct {
    u128_30D380 v[4];
    unsigned int col[4];
    float f[8];
    long a;
    long b;
    long c;
    long d;
} DrawSet;
extern char D_0013E633[];
extern char D_L01_0020BCC0[];
extern char D_L01_001672C0[];
extern short D_L01_001620F4;
extern short D_L01_001620FC;
extern int func_001F4868(int);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);

/* Builds a four-vector basis from the moby's offsets and submits it as a draw set. */
void func_L01_0030D380(char *moby) {
    DrawSet s;
    u128_30D380 in[4];
    char t0[16], t1[16], t2[16], t3[16], t4[16], t5[16];
    int i;
    int r;
    char *p;
    char *q;
    char *tp;
    in[0] = *(u128_30D380 *)(D_L01_0020BCC0);
    in[1] = *(u128_30D380 *)(D_L01_0020BCC0 + 0x10);
    in[2] = *(u128_30D380 *)(D_L01_0020BCC0 + 0x20);
    in[3] = *(u128_30D380 *)(D_L01_0020BCC0 + 0x30);
    r = func_001F4868(0xB);
    s.d = 0x8000000048L;
    s.a = 5;
    s.b = r;
    s.c = 0xFF90L << 32 | 0x260;
    s.f[0] = 0; s.f[1] = 0; s.f[2] = 0;
    s.f[3] = 1.0f; s.f[4] = 1.0f; s.f[5] = 0; s.f[6] = 1.0f; s.f[7] = 1.0f;
    func_001F9C30(t5, moby + 0xC0, *(float *)&D_L01_001620F4);
    func_001F9C30(t4, moby + 0xE0, *(float *)&D_L01_001620FC);
    func_001F9BD8(t5, t5, t4);
    func_001F9BD8(t3, t5, moby + 0x10);
    func_001F9BF0(tp = t2, D_L01_001672C0, t3);
    func_L00_001FF4B0(t2, t2, 1.0f);
    func_001F9CA0(t1, t2, D_0013E633 + 0x10AD);
    func_L00_001FF4B0(t1, t1, -1.0f);
    func_001F9CA0(t0, t1, t2);
    s.col[3] = 0x202020FF;
    s.col[2] = 0x202020FF;
    s.col[1] = 0x202020FF;
    s.col[0] = 0x202020FF;
    p = (char *)&s;
    q = (char *)in;
    for (i = 3; i >= 0; i--) {
        func_001F9C30(p, q, 0.4f);
        q += 0x10;
        func_001F9EE8(p, p, tp);
        p += 0x10;
    }
    func_L00_001FD1D8(&s, 0, 0);
}
