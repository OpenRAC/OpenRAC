/* NON_MATCHING func_L00_002BF380 -- src/overlays/shared/vendor_002BA7C8.c
 * Best so far: SIZE ours 1504 / retail 1496, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002BF380: sibling of func_L00_002BCED0 (ribbon of quads along a polyline, two passes, two draw-packet
 *   Remaining difference is float register allocation only: retail puts the zero reloaded from 0xE0 in $f20, 1.0 i
 *   Would need the allocator-priority tie (pseudo numbering of the zero/one constants and sy/sz) or the loop-invar
 *   q27 s09: old best.c did not compile (typedefs RW/RT clash with the file's); renamed to RWq/RTq it gives SIZE 1
 */
extern int func_001F4868(int);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern void func_00234C98(int, long);
extern float D_L00_00166EC0[];
extern short D_L00_00161874;
extern short D_L00_00161850;
extern short D_L00_00161878;
extern short D_L00_0016185C;
extern short D_L00_00161844;
typedef float RWq[4] __attribute__((aligned(16)));
typedef struct {
    int c[4];
    float f[8];
    long z4, q0, q1, q2;
} RTq;

// Draws a ribbon of quads along a polyline, in two passes with edge colouring.
void func_L00_002BF380(char *m, int a, int b, int n, int col, float sx, float sy, float sz) {
    RWq q[4];
    RTq t;
    RWq q2[4];
    RTq u;
    RWq w120, w130, w140, w150, r160, r170, a180, b190, s1A0, s1B0;
    int i, col0, col1, n1;
    float x, y, len, len1, len2, l0, l1;
    t.q0 = func_001F4868(*(int *)&D_L00_00161874);
    col0 = *(int *)&D_L00_00161850 | (a << 24);
    t.q2 = 0x8000000048L;
    t.q1 = 0xFF9000000260L;
    t.z4 = 0;
    t.c[3] = col0;
    t.c[2] = col0;
    t.c[1] = col0;
    t.c[0] = col0;
    u.q0 = func_001F4868(*(int *)&D_L00_00161878);
    col1 = col | (b << 24);
    u.q2 = t.q2;
    u.q1 = t.q1;
    u.z4 = t.z4;
    u.c[3] = col1;
    u.c[2] = col1;
    u.c[1] = col1;
    u.c[0] = col1;
    u.f[0] = 0; u.f[1] = 0; u.f[2] = 0; u.f[3] = 1.0f; u.f[4] = 1.0f; u.f[5] = 0; u.f[6] = 1.0f; u.f[7] = 1.0f;
    x = *(float *)&D_L00_0016185C + 0.0f;
    y = *(float *)&D_L00_0016185C + 1.0f;
    t.f[0] = x; t.f[1] = 0; t.f[2] = x; t.f[3] = 1.0f; t.f[4] = y; t.f[5] = 0; t.f[6] = y; t.f[7] = 1.0f;
    func_001F9BF0(w140, m, D_L00_00166EC0);
    func_001F9BF0(w150, m + 0x10, m);
    func_001F9CA0(w120, w150, w140);
    len = func_001F9CB8(w120);
    if (len != 0) len = 1.0f / len;
    func_001F9C30(w120, w120, *(float *)&D_L00_00161844 * len);
    func_001F9BD8(q[0], m, w120);
    func_001F9BF0(q[1], m, w120);
    n1 = n - 1;
    for (i = 1; i < n1; i++) {
        func_001F9BF0(w140, m + i * 16, D_L00_00166EC0);
        func_001F9BF0(w150, m + i * 16 + 16, m + i * 16);
        func_001F9CA0(w120, w150, w140);
        len1 = func_001F9CB8(w120);
        if (len1 != 0) len1 = 1.0f / len1;
        func_001F9C30(w120, w120, *(float *)&D_L00_00161844 * len1);
        func_001F9BD8(q[2], m + i * 16, w120);
        func_001F9BF0(q[3], m + i * 16, w120);
        if (i == 1) {
            t.c[0] = *(int *)&D_L00_00161850;
            t.c[1] = *(int *)&D_L00_00161850;
        } else if (i == n1 - 1) {
            t.c[2] = *(int *)&D_L00_00161850;
            t.c[3] = *(int *)&D_L00_00161850;
        } else if (i == 2) {
            t.c[1] = col0;
            t.c[0] = col0;
        }
        func_L00_001FD1D8(q, 0, 0);
        qcopy(q[0], q[2]);
        qcopy(q[1], q[3]);
    }
    for (i = 0; i < n1; i++) {
        func_001F9BF0(a180, m + i * 16, D_L00_00166EC0);
        func_001F9BF0(b190, m + i * 16 + 16, D_L00_00166EC0);
        l0 = func_001F9CB8(a180);
        l1 = func_001F9CB8(b190);
        if (l1 < l0) {
            if (l1 == 0) qcopy(b190, a180);
            else func_001F9C30(b190, b190, l0 / l1);
        } else {
            if (l0 == 0) qcopy(a180, b190);
            else func_001F9C30(a180, a180, l1 / l0);
        }
        func_001F9BD8(r160, D_L00_00166EC0, a180);
        func_001F9BD8(r170, D_L00_00166EC0, b190);
        func_001F9BF0(w150, r170, r160);
        func_001F9C30(s1B0, w150, sz);
        func_001F9BF0(r160, r160, s1B0);
        func_001F9BD8(r170, r170, s1B0);
        func_001F9BF0(s1A0, D_L00_00166EC0, r160);
        func_001F9CA0(w120, s1A0, w150);
        len2 = func_001F9CB8(w120);
        if (len2 != 0) len2 = 1.0f / len2;
        func_001F9C30(w130, w120, sy * len2);
        func_001F9BD8(q2[0], r160, w130);
        func_001F9BF0(q2[1], r160, w130);
        func_001F9BD8(q2[2], r170, w130);
        func_001F9BF0(q2[3], r170, w130);
        if (i == 0) {
            u.c[0] = col;
            u.c[1] = col;
        } else if (i == n - 2) {
            u.c[2] = col;
            u.c[3] = col;
        } else if (i == 1) {
            u.c[1] = col1;
            u.c[0] = col1;
        }
        func_L00_001FD1D8(q2, 0, 0);
    }
    func_00234C98(0x47, 0x5360B);
}
