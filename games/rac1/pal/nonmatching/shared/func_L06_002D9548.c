/* NON_MATCHING func_L06_002D9548 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: BYTES 8/920 (99.1% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level-06 moby update (shared, levels 06 and 10): a fan of sparks over ten passes, sampling from the owner (a1)
 *   Left: the one difference is where the pointer copy lands in the else arm: retail puts `addiu $6,$6,0x10` befor
 *   Unblock: a way to keep the copy after the call without a new local; the rest of the function matches instructi
 */
extern int func_0022ED80(int, int, int);
extern unsigned char *func_L00_0026C630(void *pos, int spin, int col, float range, float f1, float f2, float f3, float scale);
extern void func_001F9BF0(void *, void *, void *);
extern float func_L00_00258C80(float lo, float hi);
extern float func_002140F8(float, float);
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_L00_001FF860(float, float);
extern char *func_L06_003020B8(void *, void *, void *);
extern int func_001F9850(int);
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];

/* Per-frame update of a level-06 moby: scatters a fan of sparks from the owner or the moby and sets its state. */
void func_L06_002D9548(void *a0, void *a1) {
    char *moby = a0;
    char *owner = a1;
    float v0[4];
    float A[4];
    float B[4];
    float C[4];
    int i;
    int j;
    char *w;
    char *p16;
    char *pm;
    char *rt2;
    float acc;
    float r4;
    float r8;
    float rt;
    float r9;
    float r11;
    float f21;
    float r13;
    float r15;
    char *q;
    unsigned short hw;

    func_0022ED80(0, 0, (int)moby);
    qcopy(v0, moby + 0x10);
    v0[2] = v0[2] + 0.3f;
    func_L00_0026C630(v0, 1, 0x40808080, 0.3f, 1.01f, 1.075f, 0.05f, 50000.0f);
    for (i = 4; i >= 0; i--) {
        func_L00_0026C630(v0, 1, 0x40808080, 0.3f, 1.01f, 1.07f, 0.05f, 50000.0f);
    }

    if (owner != 0) {
        qcopy(A, owner + 0x10);
        p16 = moby + 0x10;
    } else {
        pm = moby + 0x10;
        rt2 = *(char **)(D_0013E633 + 0x2E9D) + 0x10;
        func_001F9BF0(A, pm, rt2);
        p16 = pm;
    }
    w = D_0013E633 + 0xE1D;
    acc = 0.0f;

    for (j = 9; j >= 0; j--) {
        qcopy(B, p16);
        B[0] = B[0] + func_L00_00258C80(0.0f, 0.2f);
        B[1] = B[1] + func_L00_00258C80(0.0f, 0.2f);
        B[2] = B[2] + func_002140F8(0.15f, 0.55f);
        r4 = func_00214158();
        rt = func_002140F8(2.0f, 4.5f);
        f21 = rt * D_0015EE6C;
        C[0] = func_001F9F90(r4) * f21;
        C[1] = func_001F9FA8(r4) * f21;
        C[2] = acc;
        r8 = func_002140F8(D_0015EE6C * 0.7f, D_0015EE6C * 1.5f);
        if (owner != 0) {
            r9 = func_L00_001FF860(*(float *)(owner + 0x10), *(float *)(owner + 0x14));
            C[0] = C[0] + func_001F9F90(r9) * r8;
            r11 = func_L00_001FF860(*(float *)(owner + 0x10), *(float *)(owner + 0x14));
            C[1] = C[1] + func_001F9FA8(r11) * r8;
        } else {
            r13 = func_L00_001FF860(*(float *)(moby + 0x10) - *(float *)(w + 0x80),
                                    *(float *)(moby + 0x14) - *(float *)(w + 0x84));
            C[0] = C[0] + func_001F9F90(r13) * r8;
            C[1] = C[1] + func_001F9FA8(r13) * r8;
        }
        r15 = func_002140F8(2.0f, 7.0f);
        C[2] = r15 * D_0015EE6C;
        q = func_L06_003020B8(moby, B, C);
        if (q != 0) {
            *(float *)(q + 0x2C) = *(float *)(q + 0x2C) * 0.3f;
        }
    }

    hw = *(unsigned short *)(moby + 0x34);
    *(int *)(moby + 0x94) = 0;
    moby[0x20] = 2;
    *(unsigned short *)(moby + 0x34) = hw | 1;
    *(unsigned char *)(moby + 0xBC) = func_001F9850(0xF);
}
