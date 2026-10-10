/* NON_MATCHING func_L08_002D68D0 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: SIZE ours 892 / retail 900, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped at budget: best p4.c (892 bytes against 900; p9.c is the same size). Steers the moby toward a route en
 */
extern int D_L08_001B0FB0[];
extern char D_0013E633[];
extern short D_L08_00161930;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;

extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern float func_00214D28(float *, float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214358(void *, int, float);
extern float func_001F9D48(void *, void *);
extern float func_001F9CB8(void *a);
extern int func_L00_0028EB98(void *, int);
extern int func_L00_0028F210(int, int);
extern int func_001F9850(int);
extern void func_L00_0028EBF0(int);

/* Steers the moby along the current route entry and keeps its two tracked slots in step. */
void func_L08_002D68D0(char *moby, int idx) {
    char *data = *(char **)(moby + 0x78);
    char *route = (char *)D_L08_001B0FB0[idx];
    float vec[4] __attribute__((aligned(16)));
    float w[4];
    float f2;
    float t;
    int r;
    int r2;
    char *e;
    char *p;
    char *m10 = moby + 0x10;
    char *m48 = moby + 0x48;

    qcopy(vec, route + 0x10 + (*(int *)(data + 0x168) << 4));
    f2 = func_L00_001FF860(vec[0] - *(float *)m10, vec[1] - *(float *)(moby + 0x14));
    func_L00_0025CE58((float *)m48, (float *)(data + 0x158), f2,
                      D_0015EE70 * 4.71238899f, D_0015EE70 * 4.71238899f, D_0015EE6C * 3.14159274f);
    func_00214D28((float *)(data + 0x180), *(float *)(data + 0x16C), D_0015EE70 + D_0015EE70);
    w[0] = func_001F9F90(*(float *)m48) * *(float *)(data + 0x180);
    w[1] = func_001F9FA8(*(float *)m48) * *(float *)(data + 0x180);
    w[2] = 0.0f;
    func_001F9BD8(m10, m10, w);
    *(float *)(moby + 0x18) = func_00214358(m10, 0, 0.5f);
    if (func_001F9D48(m10, w) < 0.5f) {
        *(int *)(data + 0x168) = (*(int *)(data + 0x168) + 1) % *(int *)route;
    }
    f2 = *(float *)(data + 0x180) / (*(float *)&D_L08_00161930 * D_0015EE6C);
    if (2.0f < f2) {
        f2 = 2.0f;
    } else if (f2 < 0.0f) {
        f2 = 0.0f;
    }
    if ((p = *(char **)(data + 0x160)) != 0) {
        *(float *)(p + 0x58) = f2;
        qcopy(*(char **)(data + 0x160) + 0x40, m48);
        qcopy(*(char **)(data + 0x160) + 0x10, m10);
    }
    if ((p = *(char **)(data + 0x164)) != 0) {
        *(float *)(p + 0x58) = f2;
        qcopy(*(char **)(data + 0x164) + 0x40, m48);
        qcopy(*(char **)(data + 0x164) + 0x10, m10);
    }
    t = D_0015EE6C * 0.1f;
    if (t < func_001F9CB8(data + 0x40)) {
        r = func_L00_0028EB98(moby, *(int *)(data + 0x15C));
        if (r == 0) {
            *(int *)(data + 0x15C) = func_0022ED80(1, 4, (int)moby);
            func_L00_0028F210(*(int *)(data + 0x15C), 1);
        } else {
            e = D_0013E633 + 0x1D + *(int *)(data + 0x15C) * 0x70;
            if (*(int *)(e + 0x80) < 0x400) {
                r2 = func_001F9850(0x3C);
                func_L00_0028F210(*(int *)(data + 0x15C), *(int *)(e + 0x80) + 0x400 / r2);
            }
        }
    } else {
        r = func_L00_0028EB98(moby, *(int *)(data + 0x15C));
        if (r != 0) {
            e = D_0013E633 + 0x1D + *(int *)(data + 0x15C) * 0x70;
            if (*(int *)(e + 0x80) < 0x100) {
                func_L00_0028EBF0(*(int *)(data + 0x15C));
                *(int *)(data + 0x15C) = -1;
            } else {
                r2 = func_001F9850(0x3C);
                func_L00_0028F210(*(int *)(data + 0x15C), *(int *)(e + 0x80) - 0x400 / r2);
            }
        }
    }
}
