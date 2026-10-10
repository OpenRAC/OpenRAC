/* NON_MATCHING func_L12_002EE180 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 1144 / retail 1152, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   What it does: rocket moby update (class 409). State 1 gets a target (func_L00_0025B478), sets the moby's aim v
 *   Stopped at the budget (10 runs). Left: the global D_L12_00174340 (lui %hi kept in $s1 with addiu %lo at each u
 */
extern char *func_L00_0025B478(void *, int, int);
extern void func_0020D678(void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_001FF240(void *, void *, void *);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern void func_L00_0026A7F8(void *, void *, int, int, int, int, int, int);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_001F10E0(void *, float, int, void *);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern void func_L00_0025AAC0(void *, void *);
extern int func_001F9908(int *arg0);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_00260108(void *, void *, int, float, float);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern int D_0015EE84_e __asm__("D_0015EE84") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L12_001672C0[];
extern char D_L12_00174340[];
extern char D_0013E633[];
extern float D_L12_0015F660[] MACRO_ADDR;

struct Pk {
    float f[4];
    int pad[2];
    unsigned char a;
    unsigned char b;
    unsigned short c;
};

/* Rocket moby update (class 409): state 1 sets up the effect, state 2 emits its packets. */
void func_L12_002EE180(char *moby) {
    float vq[4];
    float aq[4];
    float bq[4];
    float cq[4];
    struct Pk pk;
    struct Pk *pp;
    char *hs;
    char *dd;
    char *data;
    char *r2;
    char *hsv;
    int mode, r16, ra, rb, s;
    unsigned char st;
    float f20, f13;

    data = *(char **)(moby + 0x78);
    st = *(unsigned char *)(moby + 0x20);
    switch (st) {
    case 1:
        r2 = func_L00_0025B478(moby, 0x230000, 0);
        if (r2 != 0 && *(int *)(r2 + 0x20) != *(int *)(data + 0x10)) {
            moby[0x20] = 2;
            return;
        }
        mode = D_0015EE84_e;
        if (mode == 0xC && *(int *)(D_0013E633 + 0x2EA1) == 0x32 && *(char **)(data + 0x10) != 0) {
            char *r = *(char **)(data + 0x10);
            if ((signed char)r[0x20] < 0) {
                func_0020D678(moby);
                return;
            }
            if ((unsigned char)r[0x20] < 2) {
                moby[0x20] = 2;
                return;
            }
        }
        qcopy(vq, moby + 0x10);
        f20 = 0.2f;
        func_001F9BD8(moby + 0x10, moby + 0x10, data);
        f13 = D_0015EE6C * 6.2831855f;
        *(float *)(moby + 0x48) = *(float *)(data + 0x1C);
        *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40), f13);

        func_L00_001FF4B0(aq, data, f20);
        func_L00_001FF4B0(bq, data, -0.2f);
        func_L00_001FF240(cq, bq, moby + 0x10);
        ra = func_L00_00258BC8(0xF, 0x16);
        r16 = func_001F9850(ra);
        rb = func_L00_00258BC8(0x14, 0x23);
        func_L00_0026A7F8(bq, aq, 0x6F00AFFF, 0xFF, r16, 0x28, rb, 1);
        ra = func_L00_00258BC8(0x1E, 0x3C);
        r16 = func_001F9850(ra);
        rb = func_L00_00258BC8(0x32, 0x4B);
        func_L00_0026A7F8(bq, aq, 0x1FFFFFFF, 0x4F4F4F, r16, 0x28, rb, 0);

        if (!(func_L00_001EFFF0(moby + 0x10, vq, 0, moby, 0) == 0
              && func_L00_001F10E0(moby + 0x10, f20, 0, moby) == 0)) {
            hs = D_L12_00174340;
            hsv = *(char **)(hs + 0x18);
            if (hsv != 0 && hsv != *(char **)(data + 0x10)) {
                mode = D_0015EE84_e;
                s = 1;
                if (mode == 0xC && *(int *)(D_0013E633 + 0x2EA1) == 0x32) {
                    s = 3;
                    if (*(int *)&D_L12_00161FA0 >= 0x10) s = 2;
                }
                pp = &pk;
                func_L00_0025A8C0(pp, moby, 0x10003, (float)s, data);
                pp->f[0] = func_001F9F90(*(float *)(data + 0x1C));
                pp->f[1] = func_001F9FA8(*(float *)(data + 0x1C));
                pp->f[2] = 1.0f;
                pp->f[3] = 5625.9248046875f;
                pk.b = 1;
                pp->c = *(unsigned short *)(moby + 0xA6);
                pp->a = 1;
                func_L00_0025AAC0(*(char **)(D_L12_00174340 + 0x18), pp);
            }
            moby[0x20] = 2;
        }
        if (func_001F9908((int *)(data + 0x14))) {
            moby[0x20] = 2;
        }
        break;
    case 2:
        func_001F9BF0(vq, moby + 0x10, D_L12_001672C0);
        func_L00_001FF4B0(vq, vq, 0.4f);
        func_001F9BD8(moby + 0x10, moby + 0x10, vq);
        mode = D_0015EE84_e;
        if (mode == 0xC && *(int *)(D_0013E633 + 0x2EA1) == 0x32) {
            func_L00_0025F4A8(moby, D_L12_0015F660, 0, 0.0f, 0.0f, 3, 3, 5, 0.3f, 0.2f, 0.0f, -1, 0.15f, 0.0f, 0, 0, -1, 0);
            dd = D_L12_001672C0 - 0x140;
            *(float *)(dd + 0x160) = 0.1f;
            *(int *)(dd + 0x168) = func_001F9850(0x14);
        } else {
            func_L00_00260108(moby, moby + 0x10, 0, 0.25f, 13.0f);
        }
        func_0020D678(moby);
        break;
    default:
        return;
    }
}
