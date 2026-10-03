/* NON_MATCHING func_L06_0022A868 -- src/overlays/shared/help_0021D6B8.c
 * Best so far: BYTES 28/956 (97.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - run1 p0: SIZE 968/956. long r survives as pseudo (not propagated) but int conversion emits dsll32/dsra32 at 
 *   - run2 p1: BYTES 28/956. int r + (short)r casts at 3 calls keeps 0xD24 in a pseudo (gcse cprop cannot replace 
 *   - run3 p2: BYTES 28/956 (same). local t for FastVecAdd args changes nothing.
 *   - run4 p3: BYTES 28/956 (identical). r=0xD24 before the FastVecAdd call changes nothing: scheduler places li r
 *   - run5 p4 (DIAGNOSTIC do/while(0), banned): SIZE 964/956: loop label alignment adds nops; li r/i stay after ca
 *   - run6 p5: BYTES 28/956 (identical). r declared before i: no change. Third same-diff change in a row -> stop.
 *   STOP. Best = p1.c (BYTES 28/956): L01 port + `int r = 0xD24` passed as `(short)r` at the 3 collision calls (gc
 *   Left: only the FastVecAdd block before the loop. Retail = lui | addiu $a0; move $a2,$s6; jal; move $a1,$a0 | l
 */
extern char D_L06_001746F0[];
extern float D_0015EE60 MACRO_ADDR;
extern void func_001F9BC0(float *);
extern void func_L00_00235040(void);
extern void func_L00_00234090(float *dst, float *src, float dz);
extern void func_L00_00233D50(float *dst, float *src, float h);
extern int func_L00_001F10E0(float, void *, int, void *);
extern int func_L00_001F1D20(float, float, void *, int, void *);
extern int func_L00_001F34F0(float, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern void func_L00_001FF4B0(void *, void *, float);

/* Checks that the hero capsule passes: sweeps the capsule toward the target, and clamps the remaining offset. */
int func_L06_0022A868(int a0) {
    char *g = (char *)D_0013E633 + 0xE1D;
    float A[4], B[4], C[4];
    int r;
    int i;
    float len;
    char *p, *h;
    if (*(int *)(g + 0x1CC) != 0) return 1;
    qcopy(A, g + 0x80);
    func_001F9BC0(B);
    if (*(short *)(g + 0x22DA) != 0) {
        func_L00_00235040();
        *(short *)(g + 0x22DA) = 0;
    }
    if (*(int *)(g + 0x208C) != 0x11) {
        unsigned char t = *(unsigned char *)(g + 0x20B3);
        if (t != 0 || *(short *)(g + 0x1F8) != 0) {
            if (t == 1 || *(short *)(g + 0x1F8) != 0) {
                func_L00_00234090(B, B, 0.6f);
            } else {
                func_L00_00233D50(B, B, -*(float *)(g + 0x224));
            }
        } else {
            B[2] = *(float *)(g + 0x224);
        }
    }
    func_001F9BD8((char *)D_0013E633 + 0xE9D, (char *)D_0013E633 + 0xE9D, B);
    r = 0xD24;
    for (i = 0; i < 8; i++) {
        g = (char *)D_0013E633 + 0xE1D;
        if (*(unsigned char *)(g + 0x20B3) != 0) {
            if (!func_L00_001F10E0(D_0015EE60 * 0.4f, g + 0x80, (short)r, *(void **)(g + 0x2080))) break;
        } else if (*(int *)(g + 0x208C) == 0xF) {
            if (!func_L00_001F10E0(D_0015EE60 * 0.45f, g + 0x80, (short)r, *(void **)(g + 0x2080))) break;
        } else {
            float d = *(float *)(g + 0x220) - *(float *)(g + 0x224);
            int n;
            if (d < 0.05f) d = 0.05f;
            n = func_L00_001F1D20(*(float *)(g + 0x234), d, g + 0x80, (short)r, *(void **)(g + 0x2080));
            n |= func_L00_001F34F0(*(float *)(g + 0x234), g + 0x80);
            if (n == 0) break;
        }
        p = D_L06_001746F0;
        qcopy((char *)D_0013E633 + 0xE9D, p);
        qcopy((char *)D_0013E633 + 0xE9D + 0x180, p + 0x10);
        qcopy((char *)D_0013E633 + 0xE9D + 0x190, p - 0x10);
        h = (char *)D_0013E633 + 0xE1D;
        *(char *)(h + 0x257) = 1;
        *(int *)(h + 0x23C) = *(int *)(p - 0x18);
    }
    func_001F9BF0((char *)D_0013E633 + 0xE9D, (char *)D_0013E633 + 0xE9D, B);
    func_001F9BF0(C, (char *)D_0013E633 + 0xE9D, A);
    len = func_001F9CB8(C);
    if (*(float *)((char *)D_0013E633 + 0xE9D + 0x1B4) * 1.5f < len) {
        if (a0 == 0xF) {
        if (C[0] > 512.0f) C[0] = 512.0f;
        else if (C[0] < -512.0f) C[0] = -512.0f;
        if (C[1] > 512.0f) C[1] = 512.0f;
        else if (C[1] < -512.0f) C[1] = -512.0f;
        if (C[2] > 512.0f) C[2] = 512.0f;
        else if (C[2] < -512.0f) C[2] = -512.0f;
        g = (char *)D_0013E633 + 0xE1D;
        func_L00_001FF4B0(C, C, *(float *)(g + 0x234));
        func_001F9BD8(g + 0x80, A, C);
        }
        return -1;
    }
    return 1;
}
