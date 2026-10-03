/* NON_MATCHING func_L00_002D3F40 -- src/overlays/shared/vendor_002D1168.c
 * Best so far: SIZE ours 920 / retail 916, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws fading ring sprites along a timer (d+0x24..0x28): builds a GIF-like packet (long[4] at sp+0x70, packed f
 *   Left: retail keeps constants 0.0/1.0 in $f0/$f20 for the float stores (swc1) and the sd/sw stores are schedule
 */
typedef int u128 __attribute__((mode(TI)));
extern int func_001F4868(int);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_001FD1D8(void *, void *, int);
extern float D_0015EE6C MACRO_ADDR;
extern short D_L00_001619E8;
extern short D_L00_001619D4;
extern short D_L00_001619D0;
extern short D_L00_001619D8;
extern short D_L00_001619DC;
extern short D_L00_001619E0;
extern short D_L00_001619E4;


// Draws the trail of fading ring sprites for a moby.
void func_L00_002D3F40(char *a) {
    u128 m[4];
    int col[4];
    float fl[8];
    long pk[4];
    u128 tt[6];
    char *d;
    float f20, f21, f22, f23;
    char *p3, *p4, *p5;
    u128 *m1, *m2, *m3;
    long r1;
    int c0;
    d = *(char **)(a + 0x78);
    r1 = func_001F4868(*(int *)&D_L00_001619E8);
    c0 = *(int *)&D_L00_001619E4;
    pk[0] = 4;
    pk[1] = r1;
    pk[2] = 0xFF9000000260L;
    pk[3] = ((long)*(int *)&D_L00_001619D4 << 2) | (long)*(int *)&D_L00_001619D0 | ((long)*(int *)&D_L00_001619D8 << 4) | ((long)*(int *)&D_L00_001619DC << 6) | ((long)*(int *)&D_L00_001619E0 << 32);
    col[0] = c0;
    col[3] = c0;
    col[2] = c0;
    col[1] = c0;
    fl[3] = 0.0f;
    fl[0] = 0.0f;
    fl[4] = 0.0f;
    fl[2] = 1.0f;
    fl[6] = 1.0f;
    fl[1] = 0.0f;
    fl[5] = 1.0f;
    fl[7] = 1.0f;
    *(float *)(d + 0x24) = *(float *)(d + 0x24) + D_0015EE6C;
    if (1.0f < *(float *)(d + 0x24)) *(float *)(d + 0x24) = *(float *)(d + 0x24) - 1.0f;
    if (*(float *)(d + 0x28) < *(float *)(d + 0x24)) *(float *)(d + 0x28) = *(float *)(d + 0x24);
    f21 = *(float *)(d + 0x24);
    tt[1] = 0;
    *(float *)&tt[1] = 1.0f;
    func_L00_001FF4B0(&tt[0], d + 0x30, 1.0f);
    func_001F9CA0(&tt[2], &tt[0], &tt[1]);
    func_L00_001FF4B0(&tt[2], &tt[2], 1.0f);
    func_001F9CA0(&tt[1], &tt[0], &tt[2]);
    func_L00_001FF4B0(&tt[1], &tt[1], 1.0f);
    f22 = *(float *)(d + 0x24);
    f23 = 1.0f;
    p3 = (char *)&tt[3];
    p4 = (char *)&tt[4];
    p5 = (char *)&tt[5];
    m1 = &m[1];
    m2 = &m[2];
    m3 = &m[3];
    for (; *(float *)(d + 0x24) - 1.0f < f22; f22 -= 0.1f) {
        if (f21 <= *(float *)(d + 0x28)) {
            f20 = f21 * 0.5f + 0.23f;
            func_L00_001FF4B0(p3, &tt[0], f21);
            func_L00_001FF4B0(p4, &tt[1], f20);
            func_L00_001FF4B0(p5, &tt[2], f20);
            func_001F9BF0(&m[0], d + 0x10, p3);
            *m1 = m[0];
            *m2 = m[0];
            *m3 = m[0];
            func_001F9BF0(&m[0], &m[0], p4);
            func_001F9BD8(m1, m1, p4);
            func_001F9BF0(m2, m2, p4);
            func_001F9BD8(m3, m3, p4);
            func_001F9BF0(&m[0], &m[0], p5);
            func_001F9BD8(m2, m2, p5);
            func_001F9BF0(m1, m1, p5);
            func_001F9BD8(m3, m3, p5);
            col[0] = (*(int *)(a + 0x90) & 0xFFFFFF) | (func_001FA898_r((f23 - f21) * 255.0f) << 24);
            col[3] = col[0];
            col[2] = col[0];
            col[1] = col[0];
            func_L00_001FD1D8(&m[0], 0, 1);
        }
        f21 -= 0.1f;
        if (f21 < 0.0f) f21 += f23;
    }
}
