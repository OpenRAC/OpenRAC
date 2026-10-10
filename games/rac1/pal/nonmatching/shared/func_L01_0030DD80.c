/* NON_MATCHING func_L01_0030DD80 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: SIZE ours 864 / retail 880, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped with p1.c as the best candidate: SIZE 864 vs retail 880 (16 bytes short). p2.c (named pi/3pi/2 locals)
 *   What it does: shared moby update for class 1633 (10 levels). Eases moby+0x2C toward a target, aims moby+0x44/0
 *   Where it differs (p1): the three constants k, d, max are formed in retail from D_0015EE70 and D_0015EE6C re-re
 *   Would unblock: a way to make the global constant reads sit between the calls the way retail places them. Runs 
 */
extern int func_001F9850(int);
extern float func_L00_00259148(float *vel, float cur, float target, float k, float d, float max);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D48(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF500(void *, void *, float);
extern int func_001F9938(void *);
extern void func_0020D678(void *);
extern int func_L00_001F3958(void);
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L01_001672C0_c[] __asm__("D_L01_001672C0");
extern char D_L01_00174360[];
extern char D_0013E633[];

// Shared moby update (class 1633): eases its position and angle toward the target and checks bounds.
void func_L01_0030DD80(char *moby) {
    char *data = *(char **)(moby + 0x78);
    char *p1 = *(char **)(moby + 0x24);
    short s;
    float v0[4];
    float w[4];
    char s20[0x24];
    int t1, t2, lt, r;
    float f22, f13, mx, k, d, r2, pi, tpi;

    if (*(float *)(moby + 0x2C) < *(float *)(p1 + 0x24)) {
        *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) + D_0015EE60 * 0.21f * *(float *)(p1 + 0x24);
    }
    s = *(short *)(data + 0x14);
    t1 = func_001F9850(0x23);
    t2 = func_001F9850(1);
    lt = s < (t1 - t2);
    if (lt) {
        f13 = *(float *)(data + 8);
        f22 = *(float *)(data + 0xC);
    } else {
        f13 = *(float *)(moby + 0x48);
        f22 = *(float *)(moby + 0x44);
    }
    pi = 3.14159274f;
    tpi = 4.71238899f;
    k = D_0015EE70 * tpi;
    d = D_0015EE70 * pi;
    mx = D_0015EE6C * pi;
    *(float *)(moby + 0x48) = func_L00_00259148((float *)data, *(float *)(moby + 0x48), f22, k, d, mx);
    r2 = func_L00_00259148((float *)(data + 4), *(float *)(moby + 0x44), f22, k, d, mx);
    *(float *)(moby + 0x44) = r2;
    func_00215C00(w, D_0015EE6C * 40.0f, *(float *)(moby + 0x48), -r2);
    qcopy(v0, moby + 0x10);
    func_001F9BD8(moby + 0x10, moby + 0x10, w);
    if (*(float *)(moby + 0x10) < 2.0f || 1021.0f < *(float *)(moby + 0x10) ||
        *(float *)(moby + 0x14) < 2.0f || 1021.0f < *(float *)(moby + 0x14) ||
        *(float *)(moby + 0x18) < 2.0f || 1021.0f < *(float *)(moby + 0x18) ||
        *(float *)(moby + 0x10) < 0.0f || *(float *)(moby + 0x14) < 0.0f ||
        *(float *)(moby + 0x18) < 0.0f) {
        func_0020D678(moby);
        return;
    }
    if (64.0f < func_001F9D48(moby + 0x10, D_L01_001672C0_c)) {
        func_0020D678(moby);
        return;
    }
    *(int *)(s20 + 0x14) = 0x10001;
    *(float *)(s20 + 0x1C) = 0.5f;
    *(char **)(s20 + 0x10) = moby;
    *(int *)(s20 + 0x20) = 1;
    func_001F9BF0(s20, moby + 0x10, D_0013E633 + 0xE9D);
    func_L00_001FF500(s20, s20, 1.0f);
    *(float *)(s20 + 0x8) = 1.0f;
    *(float *)(s20 + 0xC) = 5627.9248f;
    s20[0x18] = 1;
    s20[0x19] = 1;
    *(short *)(s20 + 0x1A) = *(unsigned short *)(moby + 0xA6);
    if (func_001F9938(data + 0x14) != 0) {
        func_0020D678(moby);
        return;
    }
    r = func_L00_001EFFF0(v0, moby + 0x10, 0, *(int *)(data + 0x10), (int)s20);
    if (r == 0) {
        return;
    }
    if (func_L00_001F3958() != 0) {
        qcopy(moby + 0x10, D_L01_00174360);
        func_0020D678(moby);
        return;
    }
    s = *(short *)(data + 0x14);
    if (func_001F9850(3) < s) {
        *(short *)(data + 0x14) = func_001F9850(3);
    }
}
