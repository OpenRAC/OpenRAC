/* NON_MATCHING func_L00_002E6CE0 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 1536 / retail 1532, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E6CE0: camera-follow update for a moby (blends vectors through func_L00_002E5618/6498/6BE0, six fu
 *   Remaining difference is register allocation: retail keeps gl ($30) and the sp+0x60 vector ($17) live, re-mater
 */
typedef float F4[4] __attribute__((aligned(16)));
extern void func_L00_002E5618(char *, void *);
extern void func_L00_002E5DE0(char *);
extern float func_001F9C78(void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BF0(float *, float *, float *);
extern float func_001EC120(void *, float, float, float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern void func_001F9C08(float, void *, void *, void *);
extern int func_001F9938(void *);
extern void func_L00_002E6BE0(char *m, float *in);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern float func_001F9CB8(void *);
extern void func_L00_002E9968(float x, float y);
extern char D_0013E633[];
extern char D_L00_00166F10[] NOT_SDA;
extern char *D_L00_00166F00;
extern float D_L00_00173F60[4] __attribute__((aligned(16)));
extern int D_L00_0015F058 MACRO_ADDR;
extern short D_L00_00161DF0;
extern short D_L00_00161DA8;

/* updates the camera follow state of a moby: blends vectors, clips against collision and places the eye */
void func_L00_002E6CE0(char *m) {
    F4 a, b, c, d, e, f, g, h, w, u, v;
    char *gl = D_L00_00166F10;
    char *p = *(char **)(m + 0x70);
    char *r = p + 0x130;
    char *q = p + 0x40;
    char *s;
    char *pad;
    int st;
    float f20, f21;
    func_L00_002E5618(m, b);
    func_L00_002E5DE0(m);
    func_L00_002E6498(m, a);
    qcopy(p + 0x50, a);
    qcopy(c, q);
    f20 = func_001F9C78(gl + 0x30, p + 0xA0);
    func_001F9C30(d, gl + 0x30, f20);
    func_001F9BF0(e, (float *)(p + 0xA0), d);
    d[0] = func_001EC120(p + 0xC0, d[0], *(float *)(q + 0x10), *(float *)(q + 0xE4), *(float *)(q + 0xE8), 0.0f);
    d[1] = func_001EC120(p + 0xC4, d[1], *(float *)(q + 0x14), *(float *)(q + 0xE4), *(float *)(q + 0xE8), 0.0f);
    d[2] = func_001EC120(p + 0xC8, d[2], *(float *)(q + 0x18), *(float *)(q + 0xE4), *(float *)(q + 0xE8), 0.0f);
    e[0] = func_001EC120(p + 0xB0, e[0], *(float *)(p + 0x40), *(float *)(q + 0xDC), *(float *)(q + 0xE0), 0.0f);
    e[1] = func_001EC120(p + 0xB4, e[1], *(float *)(q + 4), *(float *)(q + 0xDC), *(float *)(q + 0xE0), 0.0f);
    e[2] = func_001EC120(p + 0xB8, e[2], *(float *)(q + 8), *(float *)(q + 0xDC), *(float *)(q + 0xE0), 0.0f);
    func_001F9BD8(q, q, p + 0x50);
    func_001F9BD8(p + 0xA0, e, d);
    func_001F9C30(f, gl + 0x20, *(float *)(q + 0xB0));
    if (*(unsigned char *)(q + 0xD6)) {
        int k = *(unsigned short *)(q + 0xD8) + 1;
        short n;
        *(short *)(q + 0xD8) = k;
        n = k;
        if (func_001F9850(*(int *)&D_L00_00161DF0) < n) {
            *(short *)(q + 0xD8) = func_001F9850(*(int *)&D_L00_00161DF0);
        }
        func_001F9C08((float)*(short *)(q + 0xD8) / func_001FA888(func_001F9850(*(int *)&D_L00_00161DF0)), g, q, p + 0xA0);
        func_001F9BD8(p + 0x80, f, g);
    } else {
        func_001F9938(p + 0x118);
        if (*(short *)(q + 0xD8) < 0) {
            *(short *)(q + 0xD8) = 0;
        }
        func_001F9C08((float)*(short *)(q + 0xD8) / func_001FA888(func_001F9850(*(int *)&D_L00_00161DF0)), g, q, p + 0xA0);
        func_001F9BD8(p + 0x80, f, g);
    }
    func_L00_002E6BE0(m, f);
    pad = D_0013E633 + 0xE1D;
    st = *(unsigned char *)(pad + 0x20A4);
    if (st == 2) {
        func_001F9C30(f, gl + 0x20, 3.0f);
    } else if (st == 1) {
        func_001F9C30(f, gl + 0x20, 0.4f);
    } else {
        func_001F9C30(f, gl + 0x20, 0.5f);
    }
    func_001F9BD8(g, b, f);
    s = q + 0x50;
    func_001F9C30(f, gl + 0x20, *(float *)(r + 0x30));
    func_001F9BD8(s, f, q);
    f20 = *(float *)&D_L00_00161DA8 + *(float *)(*(char **)(D_L00_00166F00 + 0x70) + 0x214) * *(float *)(*(char **)(D_L00_00166F00 + 0x70) + 0x20C) * (*(float *)(*(char **)(D_L00_00166F00 + 0x70) + 0x210) - 1.0f);
    func_001F9C30(h, gl + 0x20, 20.0f);
    func_001F9BD8(h, g, h);
    if (func_L00_001EFFF0(g, h, D_L00_0015F058, *(int *)(pad + 0x2080), 0)) {
        qcopy(w, D_L00_00173F60);
    } else {
        qcopy(w, h);
    }
    func_001F9C30(h, gl + 0x20, 20.0f);
    func_001F9BF0(h, g, h);
    if (func_L00_001EFFF0(g, h, D_L00_0015F058, *(int *)(pad + 0x2080), 0)) {
        qcopy(h, D_L00_00173F60);
    }
    func_001F9BF0((float *)u, (float *)h, (float *)w);
    if (func_001F9CB8(u) < f20 + f20) {
        func_001F9BD8(v, w, h);
        func_001F9C30(v, v, 0.5f);
    } else {
        func_001F9BF0(u, w, q);
        f21 = *(float *)(r + 0x30);
        if (!(func_001F9CB8(u) <= f21 + f20)) {
            goto third;
        }
        func_001F9C30(v, gl + 0x20, f20);
        func_001F9BF0(v, w, v);
    }
    qcopy(s, v);
    func_001F9BF0(u, v, q);
    func_L00_002E9968(func_001F9C78(u, gl + 0x20), 0.003f);
    return;
third:
    f20 = func_001F9C78(q, gl + 0x30);
    if (func_001F9C78(h, gl + 0x30) < f20) {
        qcopy(h, q);
    }
    func_001F9C30(f, gl + 0x20, f21);
    func_001F9BD8(s, f, h);
}
