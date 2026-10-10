/* NON_MATCHING func_L06_00304B88 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: SIZE ours 1072 / retail 1056, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Two passes: a counted loop of 4 helper calls per pass (func_002140B0, func_001F9850, func_L00_00258BC8, then f
 *   Best p2.c: 1072 of 1056 bytes (16 over). Control flow and calls match; the allocator differs: retail keeps the
 */
extern void func_L00_00260108(void *, void *, int, float, float);
extern int func_002140B0(int);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026B890(void *, void *, int, int, float, int, int, int, int, float);
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern void func_001F9BD8(void *, void *, void *);
extern float func_002140F8(float, float);
extern void func_001F9C30(void *, void *, float);
extern int func_001FA8A8(int, int, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_00258DB0(float *, float, float);
extern float D_0015EE6C MACRO_ADDR;
extern char D_L06_00201CC8[];
extern char D_L06_00201CE0[];
extern short D_L06_001621A4;
extern short D_L06_001621A8;
extern short D_L06_001621AC;
extern short D_L06_001621B0;
extern short D_L06_001621B4;
extern short D_L06_001621B8;
extern short D_L06_001621BC;
extern short D_L06_001621C0;
extern short D_L06_001621C4;
extern short D_L06_001621C8;
extern short D_L06_001621CC;
extern short D_L06_001621D0;
extern short D_L06_001621D4;
extern short D_L06_001621D8;
extern short D_L06_001621DC;
extern short D_L06_001621E0;
extern short D_L06_001621E4;
extern short D_L06_001621E8;

typedef struct { char b[24]; } __attribute__((packed)) P24;

// Two passes over sampled tables: a per-count loop that calls the moby effect helper, then a loop of smoothed vector samples fed to func_00219780
void func_L06_00304B88(void *a0, void *a1, void *a2, float f, int n) {
    int A[6];
    int B[6];
    float v4[4];
    float v1[4];
    float v2[4];
    float v3[4];
    int i, c, m;
    int *p18;
    int *p17;
    float g;
    int r1, r2, r16, r5, r19, r9;
    int g1, g2, x1, x2, x3;
    float q3, q5, q7, q8;
    float t1, t2;

    func_L00_00260108(a0, a1, -1, 2.0f, 13.0f);
    *(P24 *)A = *(P24 *)D_L06_00201CC8;
    *(P24 *)B = *(P24 *)D_L06_00201CE0;

    for (c = n; c > 0; c--) {
        g = f * 400000.0f;
        r1 = func_002140B0(6);
        p18 = &A[r1];
        r2 = func_002140B0(6);
        p17 = &B[r2];
        r16 = func_001F9850(0xF);
        r5 = func_001F9850(0x14);
        r19 = func_L00_00258BC8(r16, r5);
        r16 = func_001F9850(0x19);
        r5 = func_001F9850(0x1E);
        r9 = func_L00_00258BC8(r16, r5);
        func_L00_0026B890(a1, a2, *p18, *p17, g, r19, r9, 0, 0, D_0015EE6C * 10.0f * f);
    }

    m = *(int *)&D_L06_001621E0 * n;
    if (m > 0) {
        for (i = 0; i < *(int *)&D_L06_001621E0 * n; i++) {
            func_L00_00258DB0(v4, 0.0f, 1.0f);
            func_001F9BD8(v4, v4, a1);
            func_L00_00258DB0(v1, 0.0f, 0.9f);
            func_001F9BD8(v1, v1, a2);
            func_001F9C30(v2, v1, func_002140F8(*(float *)&D_L06_001621A4, *(float *)&D_L06_001621A8) * D_0015EE6C);
            func_L00_00258DB0(v1, 0.0f, 0.9f);
            func_001F9BD8(v1, v1, a2);
            func_001F9C30(v3, v1, func_002140F8(*(float *)&D_L06_001621B4, *(float *)&D_L06_001621B8) * D_0015EE6C);
            t1 = func_002140F8(*(float *)&D_L06_001621AC, *(float *)&D_L06_001621B0) * f;
            v2[3] = t1;
            t2 = func_002140F8(*(float *)&D_L06_001621BC, *(float *)&D_L06_001621C0) * f;
            v3[3] = t2;
            q3 = func_002140F8(0.0f, 1.0f);
            g1 = func_001FA8A8(*(int *)&D_L06_001621C4, *(int *)&D_L06_001621C8, q3);
            q5 = func_002140F8(0.0f, 1.0f);
            g2 = func_001FA8A8(*(int *)&D_L06_001621CC, *(int *)&D_L06_001621D0, q5);
            q7 = func_002140F8((float)*(int *)&D_L06_001621D4, (float)(*(int *)&D_L06_001621D4 * 2));
            x1 = func_001FA898_r(func_001F9878(q7));
            q8 = func_002140F8((float)*(int *)&D_L06_001621D8, (float)*(int *)&D_L06_001621D8 * 1.2f);
            x2 = func_001FA898_r(func_001F9878(q8));
            q3 = func_002140F8((float)*(int *)&D_L06_001621DC * 0.8f, (float)*(int *)&D_L06_001621DC);
            x3 = func_001FA898_r(func_001F9878(q3));
            func_00219780(v4, v2, v3, g1, g2, x1, x2, x3, *(int *)&D_L06_001621E4 + (*(int *)&D_L06_001621E8 << 16));
        }
    }
}
