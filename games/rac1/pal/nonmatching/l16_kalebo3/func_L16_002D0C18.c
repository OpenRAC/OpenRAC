/* NON_MATCHING func_L16_002D0C18 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: BYTES 16/380 (95.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws a four-vertex quad (vertices from D_L16_001D3440, uv from D_L16_001D3480) between two mobys and runs fun
 *   Best is p5.c (20/380 bytes): only a register-allocation swap remains. Retail has the copy loop counter in $a3,
 *   ours swaps i and s ($t1/$a3) (p9.c fixes that but then swaps s/qs). Declaration and init order did not move bo
 */
extern float D_L16_001D3480[];
extern float D_L16_001D3440[][4];
extern short D_L16_00161AB8;
extern short D_L16_00161AC0;
extern short D_L16_00161AAC;
extern short D_L16_00161AA8;
extern short D_L16_00161AB0;
extern short D_L16_00161AB4;
extern short D_L16_00161AC4;
extern short D_L16_00161ABC;
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BC0(void *);
extern void func_001F9CA0(void *, void *, void *);
extern int func_001F4868(int);
extern void func_L00_001FD1D8(void *, void *, int);
typedef float W[4] __attribute__((aligned(16)));
typedef struct {
    int c[4];
    float uv[8];
    long z4, q0, q1, q2;
} T;

/* Draws a four-vertex quad between two mobys, stepping it up four times. */
void func_L16_002D0C18(char *a, char *b) {
    W q[4];
    T t;
    W c, d, e, bb;
    int i;
    int col;
    float *u0;
    float *u1;
    int j;
    float (*qs)[4];
    float *s;
    int *cp;
    float (*qd)[4];
    func_001F9BF0(c, b + 0x10, a + 0x10);
    func_001F9BC0(e);
    e[2] = 1.0f;
    func_001F9CA0(d, c, e);
    qcopy(bb, a + 0x10);
    bb[2] += *(float *)&D_L16_00161AB8;
    bb[3] = 1.0f;
    t.q0 = func_001F4868(*(int *)&D_L16_00161AC0);
    t.q2 = (long)*(int *)&D_L16_00161AA8 | (long)*(int *)&D_L16_00161AAC << 2 | (long)*(int *)&D_L16_00161AB0 << 4 | (long)*(int *)&D_L16_00161AB4 << 6 | 0x8000000000L;
    t.q1 = 0xFF9000000260L;
    t.z4 = 0;
    col = *(int *)&D_L16_00161AC4;
    s = D_L16_001D3480;
    qs = D_L16_001D3440;
    qd = q;
    cp = t.c;
    u1 = &t.uv[1];
    u0 = &t.uv[0];
    for (i = 3; i >= 0; i--) {
        *u0 = s[0];
        *u1 = s[1];
        *cp = col;
        qcopy(qd, qs);
        qs++;
        qd++;
        cp++;
        u1 += 2;
        u0 += 2;
        s += 2;
    }
    for (j = 3; j >= 0; j--) {
        func_L00_001FD1D8(q, c, 0);
        bb[2] += *(float *)&D_L16_00161ABC;
    }
}
