/* NON_MATCHING func_L08_002E9288 -- src/overlays/shared/vendor_002D3DF8.c
 * Best so far: SIZE ours 844 / retail 860, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   GS packet builder for moby o: copies two vectors, calls the level's float helpers in a loop of up to D_L08_001
 *   Left: retail keeps a 16-byte stack slot at sp+0x60 that nothing references (E at sp+0x70, F at sp+0x80, C at s
 *   Unblock: what the unreferenced sp+0x60 slot is (the same shape as func_L00_002C8DB8's gap at 0x40-0x6F).
 */
extern char *func_0020D348(int);
extern void func_L00_00251328(void *, int, int, int);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern void func_001FA218(void *, void *);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9F90(float);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern float func_001F9878(float);
extern int func_001FA898(float);
extern void func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern void func_L00_00251E30(void *);
extern int D_L08_0015F6B0 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern short D_L08_00161D68;
extern short D_L08_00161D70;
extern short D_L08_00161D80;
extern short D_L08_00161D84;
extern short D_L08_00161D88;
extern short D_L08_00161D8C;
extern short D_L08_00161D94;
extern short D_L08_00161D98;
extern short D_L08_00161D9C;
extern short D_L08_00161DA0;
extern short D_L08_00161DA4;
extern short D_L08_00161DA8;

// Effect builder: fills moby o's 0x10..0x50 records and its data packet; returns o.
char *func_L08_002E9288(void *a4, void *a5, void *a6, int a7) {
    float A[4] __attribute__((aligned(16)));
    float B[4] __attribute__((aligned(16)));
    float T[4] __attribute__((aligned(16)));
    float C[4] __attribute__((aligned(16)));
    float Dv[4] __attribute__((aligned(16)));
    float E[4] __attribute__((aligned(16)));
    float F[4] __attribute__((aligned(16)));
    char *o;
    int *dat;
    unsigned char *qq;
    int k;
    int r17, r16, r3;
    int n1, n2, r8;
    float a, rr, f20, f21;
    float *pC, *pE, *pF;

    qcopy(A, a5);
    qcopy(B, a6);
    o = func_0020D348(0x1CA);
    if (o == 0) {
        return o;
    }
    dat = *(int **)(o + 0x78);
    dat[5] = (int)a4;
    dat[4] = a7;
    func_L00_00251328(o, 0xA0, 0xA0, 0xA0);
    *(int *)(o + 0x40) = 0;
    *(short *)(o + 0x32) = 0xFF;
    *(unsigned char *)(o + 0x30) = 0xFF;
    *(unsigned char *)(o + 0x31) = 1;
    rr = func_001F9CE8(B);
    *(float *)(o + 0x44) = -func_L00_001FF860(rr, B[2]);
    *(float *)(o + 0x48) = func_L00_001FF860(B[0], B[1]);
    qcopy(o + 0x10, A);
    qcopy(dat, B);
    qq = (unsigned char *)(o + 0x10);
    if (D_L08_0015F6B0 & 1) {
        dat[7] = 1;
    } else {
        dat[7] = -1;
    }
    func_001FA218(T, o + 0x40);

    pC = C;
    pE = E;
    pF = F;
    k = 0;
    if ((float)k < *(float *)&D_L08_00161D70) {
        do {
            f21 = func_00214158();
            func_002140F8(*(float *)&D_L08_00161D94, *(float *)&D_L08_00161D94 + *(float *)&D_L08_00161D94);
            rr = func_001F9F90(f21);
            f20 = rr * D_0015EE6C;
            func_L00_001FF4B0(pE, Dv, f20 * rr);
            rr = func_001F9FA8(f21);
            func_L00_001FF4B0(pF, pC, f20 * rr);
            func_001F9BD8(pE, pE, pF);
            qcopy(pF, pE);
            pE[3] = *(float *)&D_L08_00161DA4;
            k++;
            pF[3] = *(float *)&D_L08_00161DA8;
            n1 = func_001F9850(10);
            n2 = func_001F9850(20);
            r8 = func_L00_00258BC8(n1, n2);
            func_L00_0026DA50(qq, pE, *(int *)&D_L08_00161D88, *(int *)&D_L08_00161D8C, r8, 1, 20000.0f);
            a = (float)*(int *)&D_L08_00161D98;
            r17 = func_001FA898(func_001F9878(func_002140F8(a, a * 1.2f)));
            a = (float)*(int *)&D_L08_00161D9C;
            r16 = func_001FA898(func_001F9878(func_002140F8(a, a * 1.2f)));
            a = (float)*(int *)&D_L08_00161DA0;
            r3 = func_001FA898(func_001F9878(func_002140F8(a, a * 1.5f)));
            func_00219780(qq, pE, pF, *(int *)&D_L08_00161D80, *(int *)&D_L08_00161D84, r17, r16, r3, -1);
        } while ((float)k < *(float *)&D_L08_00161D70);
    }
    if (D_0015EE84 == 0xC) {
        dat[6] = func_001F9850(0x78);
    } else {
        dat[6] = func_001FA898(func_001F9878(*(float *)&D_L08_00161D68));
    }
    func_L00_00251E30(o);
    return o;
}
