/* NON_MATCHING func_L00_002E91D0 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 1620 / retail 1612, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped at the 10-run budget, not EXACT. Best p6.c (or p4.c, same size) is 1620 bytes against retail's 1612, 8
 *   Still differing: the prologue (retail's lui+lw read of D_0015EE84 at the top; ours goes through $gp, and the M
 *   Note: p0.c was edited in place (declaration fixes) after its first COMPILE failure, so the second run is on th
 */
extern char D_L00_00166F10[];
extern char D_0013E633[];
extern short D_0015EE84;
extern unsigned char D_0013A5E0[];
extern int D_L00_0015F058 SDATA(D_L00_0015F058);
extern float D_L00_00161DA0 SDATA(D_L00_00161DA0);
extern int *D_L00_00178000[];
extern int *D_L00_001E7910[];
extern char D_L00_00173F60_a[] __asm__("D_L00_00173F60");
extern u8 D_L00_00166EC0_E4838[] __asm__("D_L00_00166EC0");
extern void func_001F9C08(float, void *, void *, void *);
extern int func_L00_001F2BE8_2FB898(float, void *, int, void *, void *) __asm__("func_L00_001F2BE8");
extern int func_L00_0025A868(char *a);
void func_001F9BF0_E4838(void *, void *, void *) __asm__("func_001F9BF0");
extern float func_001F9C78(void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9FC0(float);
extern float func_001F9B88(float);
extern void func_001F9CA0(void *, void *, void *);
extern void func_002156E0(void *, void *, void *, float);
extern int func_L00_002E8EE0(char *m);
extern long func_L00_002E89D0(void);
extern int func_001F9850(int);
extern int func_001F9938(void *);
extern float func_001FA888(int);
extern float func_00214220(float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_L00_002E5740(VS *a);
extern int func_L00_0025F410(int);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern void func_L00_0025AAC0(void *, void *);
extern void func_L00_001ED600(void);

/* Per-frame update of the moby list: picks entries near the hero and steps their timers. */
void func_L00_002E91D0(int mi) {
    char *fp = D_L00_00166F10;
    char *d = *(char **)(mi + 0x70);
    char *a = d + 0x130;
    char *b = d + 0x1D0;
    char *pad = D_0013E633 + 0xE1D;
    char *q90 = d + 0x90;
    float v0[4];
    float v10[4];
    float v20[4];
    float v30[4];
    float v40[4];
    char blk[0x30];
    float f20, f21, af, tt, r2, q;
    int f84 = 0;
    int f8c = 0;
    int n88 = 0;
    int cnt, k, r, v;
    int **in;
    int **out;
    short t;
    char *s16, *s19;

    f21 = (*(float *)(a + 0x2C) - *(float *)(b + 0x30)) + 1.5f;
    if (*(int *)&D_0015EE84 == 0xF) {
        f8c = (*(unsigned char *)(pad + 0x20A4) == 2);
    }
    f20 = 0.5f;
    func_001F9C08(f20, v0, q90, fp - 0x50);
    cnt = func_L00_001F2BE8_2FB898(f21 * f20, v0, 1, *(void **)(pad + 0x2080), 0);
    if (cnt > 0) {
        in = D_L00_00178000;
        out = D_L00_001E7910;
        do {
            char *e = (char *)*in;
            if (*(int *)(e + 0x94) == 0) goto next;
            if (func_L00_0025A868(e) != 0 && f8c == 0) goto next;
            t = *(short *)(e + 0xA6);
            if (t == 0x72) goto proc;
            if (t == 0xB) goto proc;
            if (t == 0x2C) goto proc;
            if (t == 0x9A) goto proc;
            if (t == 0xFF) goto proc;
            if (t == 0x392) goto proc;
            if (t == 0x452) goto proc;
            if (t == 0x353) goto proc;
            *out = (int *)e;
            out++;
            n88++;
            *(int *)(e + 0x94) = 0;
            goto next;
        proc: {
            char *g = fp + 0x30;
            func_001F9BF0_E4838(v10, e + 0x10, q90);
            tt = func_001F9C78(g, v10);
            func_001F9C30(v20, g, tt);
            func_001F9BF0_E4838(v30, v10, v20);
            func_L00_001FF4B0(v30, v30, 1.0f);
            tt = func_001F9C78(v30, a);
            r2 = func_001F9B88(1.5707964f - func_001F9FC0(tt / (*(float *)(a + 0x2C) - *(float *)(b + 0x30))));
            if (r2 < 0.43633232f) {
                func_001F9CA0(v40, a, g);
                func_L00_001FF4B0(v40, v40, 1.0f);
                if (0.0f < func_001F9C78(v40, v10)) {
                    func_002156E0(a, a, fp + 0x20, -0.017453292f);
                } else {
                    func_002156E0(a, a, fp + 0x20, 0.017453292f);
                }
            }
            }
        next:
            in++;
        } while (--cnt != 0);
    }

    k = *(int *)(pad + 0x2084);
    if (k != 0x7F && !(*(int *)&D_0015EE84 == 0xD && k == 0x7B)) {
        f84 = func_L00_002E8EE0(mi) != 0;
    }
    if (func_L00_002E89D0()) f84 = 1;
    if (func_L00_002E87C8(mi)) f84 = 1;
    if (*(float *)(a + 0x2C) - *(float *)(b + 0x30) < D_L00_00161DA0) {
        *(float *)(b + 0x30) = *(float *)(a + 0x2C) - D_L00_00161DA0;
        *(short *)(b + 0x34) = func_001F9850(2000);
    }

    if (*(short *)(b + 0x3A) != 0) goto L_ptr;
    v = *(short *)(b + 0x34);
    if (f84 != 0) goto L_val;
    r = func_001F9850(2000);
    if (v < r && *(short *)(b + 0x34) > 0) goto L_ptr;
    if (*(float *)(D_0013A5E0 + 0x2460 + 0x100) < 0.3f) {
        if (*(float *)(D_0013A5E0 + 0x2460 + 0x104) < 0.3f) goto L_ptr;
    }
    goto L_val;

L_val:
    r = func_001F9850(2000);
    if (r < v) {
        func_001F9938(b + 0x34);
    } else {
        *(short *)(b + 0x34) = func_001F9850(2000);
    }
    af = *(float *)(a + 0x2C);
    goto L_9650;

L_ptr:
    if (func_001F9938(b + 0x34) != 0) *(short *)(b + 0x3A) = 0;
    if (*(short *)(b + 0x3A) != 0) {
        if (func_001F9938(b + 0x34) != 0) *(short *)(b + 0x3A) = 0;
    }
    q = func_001FA888(*(short *)(b + 0x34));
    r2 = func_001FA888(func_001F9850(2000));
    *(float *)(b + 0x30) = func_00214220(0.0f, *(float *)(b + 0x30), q / r2);
    af = *(float *)(a + 0x2C);

L_9650:
    func_L00_001FF4B0(a, a, af - *(float *)(b + 0x30));
    func_001F9BD8(v10, a, q90);
    k = *(int *)(pad + 0x2084);
    if (k == 0x7F) goto L_zero;
    if (*(int *)&D_0015EE84 == 0xD && k == 0x7B) goto L_zero;
    if (func_L00_001EFFF0(q90, v10, D_L00_0015F058, *(int *)(pad + 0x2080), 0) == 0) goto L_zero;
    s16 = D_L00_00173F60_a;
    if (func_L00_002E5740(s16) != 0) goto L_zero;
    s19 = s16 - 0x20;
    v = *(int *)(s19 + 0x18);
    if (v != 0 && func_L00_0025F410(v)) {
        func_001F9BF0_E4838(v20, (char *)v + 0x10, D_L00_00166EC0_E4838);
        v20[2] = 0;
        func_L00_001FF4B0(v20, v20, 1.0f);
        v20[3] = 5627.925f;
        func_L00_0025A8C0(blk, *(void **)(pad + 0x2080), 0x800000, 20.0f, v20);
        blk[0x18] = 3;
        blk[0x19] = 3;
        *(unsigned short *)(blk + 0x1A) = *(unsigned short *)(*(char **)(pad + 0x2080) + 0xA6);
        func_L00_0025AAC0((void *)v, blk);
    }
    *(int *)(b + 0x4C) += 1;
    r = func_001F9850(30);
    if (*(int *)(b + 0x4C) >= r) func_L00_001ED600();
    goto L_end;

L_zero:
    *(int *)(b + 0x4C) = 0;

L_end:
    if (n88 > 0) {
        int **p = D_L00_001E7910;
        k = n88;
        do {
            char *e = (char *)*p;
            p++;
            *(int *)(e + 0x94) = *(int *)(*(char **)(e + 0x24) + 0x10);
        } while (--k != 0);
    }
}
