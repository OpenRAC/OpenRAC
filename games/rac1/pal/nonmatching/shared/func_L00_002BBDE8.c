/* NON_MATCHING func_L00_002BBDE8 -- src/overlays/shared/vendor_002BA7C8.c
 * Best so far: SIZE ours 1500 / retail 1488, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002BBDE8 draws a ribbon of n quads along a polyline (two RP structs: 4 vertices + GS tag block, FastV
 *   Best candidate p5.c is SIZE-exact; only diffs: float register numbering in the first part (retail: 1.0 in $f21
 *   store scheduling order that follows from it, and `mov.s $f1,$f0` before the zero compare in part 1/loop 1 (ret
 *   Looks like an FP-constant priority tie in global alloc; would need a source form that gives the zero pseudo a 
 *   q28/t08: candidate types must not be named RW/RT/RP (src.c's later function defines RW/RT: redefinition error)
 */
extern short D_L00_00161708;
extern short D_L00_001616C4;
extern short D_L00_0016170C;
extern short D_L00_001616C8;
extern short D_L00_001616BC;
extern short D_L00_001616CC;
extern int func_001F4868(int);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern void func_00234C98(int, long);
extern float D_L00_00166EC0[];
typedef float RibW[4] __attribute__((aligned(16)));
typedef struct {
    int c[4];
    float z0, z1, z2;
    float f0, f1;
    float z3;
    float f2, f3;
    long z4, q0, q1, q2;
} RibT;
typedef struct {
    RibW q[4];
    RibT t;
} RibP;

/* draws a ribbon strip along n points: a quad per segment with scaled ends */
void func_L00_002BBDE8(void *mv, int ca, int cb, int n, float unused, float s2, float s1) {
    char *m = mv;
    RibP p, u;
    RibW C, E, A, B, P2, Q, V1, V2, T2, S;
    float len1, len2, l;
    int i, col, col2;
    char *mp, *mq;
    p.t.q0 = func_001F4868(*(int *)&D_L00_00161708);
    col = *(int *)&D_L00_001616C4 | (ca << 24);
    p.t.q2 = 0x8000000048L;
    p.t.q1 = 0xFF9000000260L;
    p.t.z4 = 0;
    p.t.c[3] = col;
    p.t.c[2] = col;
    p.t.c[1] = col;
    p.t.c[0] = col;
    u.t.q0 = func_001F4868(*(int *)&D_L00_0016170C);
    u.t.z0 = 0;
    u.t.z1 = 0;
    u.t.z2 = 0;
    u.t.f0 = 1.0f;
    u.t.f1 = 1.0f;
    u.t.z3 = 0;
    u.t.f2 = 1.0f;
    u.t.f3 = 1.0f;
    col2 = *(int *)&D_L00_001616C8 | (cb << 24);
    u.t.q1 = p.t.q1;
    u.t.q2 = p.t.q2;
    u.t.z4 = p.t.z4;
    u.t.c[3] = col2;
    u.t.c[2] = col2;
    u.t.c[1] = col2;
    u.t.c[0] = col2;
    p.t.z0 = *(float *)&D_L00_001616BC + u.t.z0;
    p.t.z1 = u.t.z0;
    p.t.z2 = p.t.z0;
    p.t.f0 = 1.0f;
    p.t.f1 = *(float *)&D_L00_001616BC + 1.0f;
    p.t.z3 = u.t.z0;
    p.t.f2 = p.t.f1;
    p.t.f3 = 1.0f;
    func_001F9BF0(A, m, D_L00_00166EC0);
    func_001F9BF0(B, m + 0x10, m);
    func_001F9CA0(C, B, A);
    l = func_001F9CB8(C);
    l = (l == 0.0f) ? l : 1.0f / l;
    func_001F9C30(C, C, *(float *)&D_L00_001616CC * l);
    func_001F9BD8(p.q[0], m, C);
    func_001F9BF0(p.q[1], m, C);
    for (i = 1; i < n - 1; i++) {
        char *a = m + i * 0x10;
        func_001F9BF0(A, a, D_L00_00166EC0);
        func_001F9BF0(B, a + 0x10, a);
        func_001F9CA0(C, B, A);
        l = func_001F9CB8(C);
        l = (l == 0.0f) ? l : 1.0f / l;
        func_001F9C30(C, C, *(float *)&D_L00_001616CC * l);
        func_001F9BD8(p.q[2], a, C);
        func_001F9BF0(p.q[3], a, C);
        if (i == 1) {
            p.t.c[0] = *(int *)&D_L00_001616C4;
            p.t.c[1] = *(int *)&D_L00_001616C4;
        } else if (i == n - 2) {
            p.t.c[2] = *(int *)&D_L00_001616C4;
            p.t.c[3] = *(int *)&D_L00_001616C4;
        } else if (i == 2) {
            p.t.c[1] = col;
            p.t.c[0] = col;
        }
        func_L00_001FD1D8(&p, 0, 0);
        qcopy(p.q[0], p.q[2]);
        qcopy(p.q[1], p.q[3]);
    }
    for (i = 0; i < n - 1; i++) {
        mp = m + i * 0x10;
        func_001F9BF0(V1, mp, D_L00_00166EC0);
        func_001F9BF0(V2, mp + 0x10, D_L00_00166EC0);
        len1 = func_001F9CB8(V1);
        len2 = func_001F9CB8(V2);
        if (len2 < len1) {
            if (len2 == 0.0f) qcopy(V2, V1);
            else func_001F9C30(V2, V2, len1 / len2);
        } else {
            if (len1 == 0.0f) qcopy(V1, V2);
            else func_001F9C30(V1, V1, len2 / len1);
        }
        func_001F9BD8(P2, D_L00_00166EC0, V1);
        func_001F9BD8(Q, D_L00_00166EC0, V2);
        func_001F9BF0(B, Q, P2);
        func_001F9C30(S, B, s1);
        func_001F9BF0(P2, P2, S);
        func_001F9BD8(Q, Q, S);
        func_001F9BF0(T2, D_L00_00166EC0, P2);
        func_001F9CA0(C, T2, B);
        l = func_001F9CB8(C);
        l = (l == 0.0f) ? l : 1.0f / l;
        func_001F9C30(E, C, s2 * l);
        func_001F9BD8(u.q[0], P2, E);
        func_001F9BF0(u.q[1], P2, E);
        func_001F9BD8(u.q[2], Q, E);
        func_001F9BF0(u.q[3], Q, E);
        if (i == 0) {
            u.t.c[0] = *(int *)&D_L00_001616C8;
            u.t.c[1] = *(int *)&D_L00_001616C8;
        } else if (i == n - 2) {
            u.t.c[2] = *(int *)&D_L00_001616C8;
            u.t.c[3] = *(int *)&D_L00_001616C8;
        } else if (i == 1) {
            u.t.c[1] = col2;
            u.t.c[0] = col2;
        }
        func_L00_001FD1D8(&u, 0, 0);
    }
    func_00234C98(0x47, 0x5360B);
}
