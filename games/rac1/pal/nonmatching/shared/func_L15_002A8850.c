/* NON_MATCHING func_L15_002A8850 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: BYTES 8/596 (98.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a quad (four corner vectors from two cross vectors, colour words, UV table copy, 64-bit tag words) and 
 *   Best p2.c/p3.c: everything matches except the order of the four colour-word stores and the sd $zero at 0x90 re
 *   Keys: locals declared B,A,Q,F,E,C,D (first declared = lowest address), 64-bit fields are `long` (long long bec
 *   Unblock: another source order of the colour stores / initialiser form; budget spent.
 */
typedef int u128_2A8850 __attribute__((mode(TI)));
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_001FF240(void *, void *, void *);
extern int func_001F4868(int);
extern void func_L00_001FD1D8(void *, void *, int);
extern float D_L15_00167440[];
extern float D_L15_001E2AF0[];

typedef struct {
    float v[4][4];
    int col[4];
    float uv[4][2];
    long a, b, c, d;
} Quad;

/* Builds a screen-space quad from the moby's offset and draws it. */
void func_L15_002A8850(char *moby) {
    float B[4];
    float A[4];
    Quad q;
    float F[4];
    float E[4];
    float C[4];
    float D[4];
    char *d = *(char **)(moby + 0x78);
    int i;
    func_001F9BF0(A, d + 0x10, moby + 0x10);
    *(u128_2A8850 *)B = *(u128_2A8850 *)A;
    func_L00_001FF4B0(B, B, 0.5f);
    *(u128_2A8850 *)C = *(u128_2A8850 *)(moby + 0x10);
    func_L00_001FF240(D, C, B);
    func_001F9BF0(E, C, d + 0x10);
    func_001F9BF0(A, C, D_L15_00167440);
    func_001F9BF0(F, d + 0x10, D_L15_00167440);
    func_001F9CA0(A, A, E);
    func_L00_001FF4B0(A, A, 0.1f);
    func_001F9CA0(F, F, E);
    func_L00_001FF4B0(F, F, 0.1f);
    q.b = func_001F4868(0xE);
    q.c = 0xFF9000000260L;
    q.d = 0x8000000048L;
    q.col[0] = 0x80243278;
    q.col[1] = 0x80243278;
    q.col[2] = 0x80243278;
    q.col[3] = 0x80243278;
    q.a = 0;
    for (i = 0; i < 4; i++) {
        q.uv[i][0] = D_L15_001E2AF0[i * 2];
        q.uv[i][1] = D_L15_001E2AF0[i * 2 + 1];
    }
    func_001F9BF0(D, C, A);
    *(u128_2A8850 *)q.v[0] = *(u128_2A8850 *)D;
    func_001F9BD8(D, C, A);
    *(u128_2A8850 *)q.v[1] = *(u128_2A8850 *)D;
    func_001F9BF0(D, d + 0x10, F);
    *(u128_2A8850 *)q.v[2] = *(u128_2A8850 *)D;
    func_001F9BD8(D, d + 0x10, F);
    *(u128_2A8850 *)q.v[3] = *(u128_2A8850 *)D;
    func_L00_001FD1D8(&q, 0, 0);
}
