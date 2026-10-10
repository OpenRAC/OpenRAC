/* NON_MATCHING func_L14_002EE260 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 1128 / retail 1152, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby update for class 681 on level 14: a 0x10001 (sparks) burst loop of three then five iterations, a 3-entity
 *   Differences left: the prologue saves and register picks differ (retail keeps pos in $s1/$s5 and the bounds con
 *   Unblock: the allocator tie around pos/data/m (regalloc.py on p4.c), and a check whether retail keeps the EFFF0
 */
typedef int u128 __attribute__((mode(TI)));

extern char D_L14_00174660[];
extern char D_L14_00174680[];
extern float D_0015EE6C MACRO_ADDR;
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FF500(void *, void *, float);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern float func_001F9D48(void *, void *);
extern float func_002140F8(float, float);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern unsigned char *func_L07_0029B070(char *pos, char *vec);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_L00_001FF610(void *, void *, void *);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern char *func_L00_0026EBC0(char *pos, char *vel, int c, int d, float f);
extern int func_0022ED80(int, int, int);
extern void func_0020D678(void *);

/* Moby update (class 681, level 14): drifts the effect, emits sparks, checks its bounds. */
void func_L14_002EE260(char *m)
{
    char *pos = m + 0x10;
    char *data = *(char **)(m + 0x78);
    u128 t0;
    u128 tA;
    unsigned char tB[0x30];
    float tC[4];
    float v60[4];
    float v70[4];
    int flag, d20, cnt, n, a, b;
    float r, dd;
    char *base;
    char *p30;
    char *p4;
    int a18;

    *(u128 *)&t0 = *(u128 *)pos;
    func_001F9BD8(pos, pos, data);
    *(u128 *)&tA = *(u128 *)pos;
    d20 = *(int *)(data + 0x20);
    *(u128 *)tC = *(u128 *)data;
    ((int *)tC)[2] = 0;
    func_L00_001FF500(tC, tC, 1.0f);
    cnt = 0;
    tC[2] = 1.0f;
    func_L00_0025A8C0(tB, m, 0x10001, 1.0f, tC);
    tB[0x19] = 1;
    tB[0x18] = 1;
    flag = *(int *)(data + 0x24) == 0;
    r = func_001F9D48(pos, data + 0x10);
    if (r > 30.0f) {
        if (((unsigned char *)m)[0x31]) {
            n = 2;
            do {
                *(u128 *)v70 = 0;
                v70[0] = func_002140F8(-1.0f, 1.0f);
                v70[1] = func_002140F8(-1.0f, 1.0f);
                v70[2] = func_002140F8(-1.0f, 1.0f);
                *(u128 *)v60 = *(u128 *)v70;
                func_L00_001FF4B0(v60, v60, func_001F9CB8(v70) * 0.1f);
                func_001F9BD8(v60, v70, v60);
                dd = D_0015EE6C;
                func_L00_001FF4B0(v60, v60, func_002140F8(dd + dd, dd * 4.0f));
                func_L07_0029B070(pos, v60);
            } while (--n >= 0);
        }
        func_0020D678(m);
        return;
    }

    base = D_L14_00174660;
    p30 = base - 0x20;
    while (cnt < 3 && func_L00_001EFFF0(&t0, &tA, flag, d20, 0) != 0) {
        p4 = *(char **)(p30 + 0x18);
        cnt++;
        if (p4 != 0 && *(short *)(p4 + 0xA6) == 0xFC) {
            *(u128 *)&t0 = *(u128 *)base;
            a18 = (int)p4;
            continue;
        }
        func_L00_001EFFF0(&t0, &tA, flag, a18, (int)tB);
        *(u128 *)pos = *(u128 *)base;
        if (((unsigned char *)m)[0x31]) {
            cnt = 4;
            do {
                *(u128 *)v70 = 0;
                v70[0] = func_002140F8(-1.0f, 1.0f);
                v70[1] = func_002140F8(-1.0f, 1.0f);
                v70[2] = func_002140F8(-1.0f, 1.0f);
                *(u128 *)v60 = *(u128 *)v70;
                func_L00_001FF610(v70, data, D_L14_00174680);
                func_L00_001FF4B0(v60, v60, func_001F9CB8(v70) * 0.5f);
                func_001F9BD8(v60, v70, v60);
                dd = D_0015EE6C;
                func_L00_001FF4B0(v60, v60, func_002140F8(dd * 3.0f, dd * 6.0f));
                a = func_001F9850(10);
                b = func_001F9850(15);
                func_L00_0026EBC0(pos, (char *)v60, 0x7F2F4F6F, func_L00_00258BC8(a, b), 30000.0f);
            } while (--cnt >= 0);
        }
        func_0022ED80(0, 0, (int)m);
        func_0020D678(m);
        return;
    }
    {
        float *pf = (float *)pos;
        if (pf[0] < 2.0f || pf[0] > 1020.0f || pf[1] < 2.0f || pf[1] > 1020.0f ||
            pf[2] < 2.0f || pf[2] > 1020.0f)
            func_0020D678(m);
    }
}
