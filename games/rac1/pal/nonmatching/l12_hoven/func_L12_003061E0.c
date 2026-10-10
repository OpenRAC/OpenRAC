/* NON_MATCHING func_L12_003061E0 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 1752 / retail 1708, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 12 moby update, class 0x4F5: aims at a nearby moby, steers and emits effects per state (0 setup, 1 steer
 *   Best by size: p3.c (1752 vs 1708, 44 over; goto-based exits match retail's dispatch shape). Block order (state
 *   Unblock: regalloc.py on p3.c for the saved registers and the state-1 block's ordering against retail's L3B0 la
 */
extern int func_L00_00260D30(char *, char *, float);
extern char *func_L00_0025B478(void *, int, int);
extern int func_L00_0025B4D0(void *, void *, void *, int, int *, float *, int, int);
extern float func_001F9D48(void *, void *);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern void func_L00_002584A8(void *, int, int);
extern int func_001F9850(int);
extern float func_001F9FA8(float);
extern void func_00213DE0(void *, int, int, int);
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214358(void *, int, float);
extern float func_L00_0025C918(float *, float *, float, float, float, float);
extern void func_L00_0025A8C0(void *, void *, int, float, void *);
extern int func_L00_001F2BE8(float, void *, int, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_0020D678(void *);
extern void func_L00_0025B040(unsigned char *, float);
extern int D_L12_00174358 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern char D_0013E633[];

/* Level 12 moby update, class 0x4F5: aims at a nearby moby, steers toward it and emits effects by state. */
void func_L12_003061E0(char *moby)
{
    char *data = *(char **)(moby + 0x78);
    char *r19;
    int cnt;
    int state;
    float sp20[4];
    float sp70[4];
    float sp80[4];
    float sb0[4];
    float c0[4];
    float f0;
    float f2;
    float f20;
    float f21;

    func_L00_00260D30(moby, (char *)sp20, 8.0f);
    r19 = func_L00_0025B478(moby, 0x230000, 0);
    func_L00_0025B4D0(moby, r19, data + 0x20, 0, &cnt, (float *)0, 0, 4);
    f0 = func_001F9D48(moby + 0x10, sp20);

    if (f0 < 0.25f) {
        goto A_;
    }
    if (cnt < 2) {
        goto B_;
    }
    if (r19 == 0) {
        goto B_;
    }
    if (*(short *)(*(char **)(r19 + 0x20) + 0xA6) == 0x4F5) {
        goto B_;
    }
A_:
    func_L00_0025F4A8(moby, D_L12_0015F660, 0, 0.0f, 0.0f, 3, 3, 5, 1.0f, 0.5f, 4.0f, 2, 0.7f, 7.0f, 0, 0, 0, -1);
    func_L00_002584A8(moby, 0, -1);
    func_0020D678(moby);
    return;

B_:
    {
        state = moby[0x20];
        if (state == 1) {
            int c;
            int m;
            float x;
            float g;
            float h;
            c = *(int *)(data + 0x84) + 1;
            *(int *)(data + 0x84) = c;
            m = func_001F9850(0x1E);
            if (*(int *)(data + 0x84) < m) {
                int m2 = func_001F9850(0x1E);
                x = ((float)*(int *)(data + 0x84) * 3.14159274f) / (float)m2;
                *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + func_001F9FA8(x) * 0.0700000003f;
            }
            if (func_001F9850(0xF) < *(int *)(data + 0x84)) {
                *(float *)(data + 0x70) = *(float *)(data + 0x70) + *(float *)&D_0015EE70 * 30.0f;
            }
            if (func_001F9850(0x1E) < *(int *)(data + 0x84)) {
                if (*(unsigned char *)(data + 0x53) == 0) {
                    func_00213DE0(moby, 1, 0, 2);
                }
                if (*(unsigned char *)(moby + 0x53) == state) {
                    if (*(unsigned char *)(moby + 0x70) & 2) {
                        func_00213DE0(moby, 2, 0, 2);
                    }
                }
            }
            {
                float t = *(float *)&D_0015EE6C * 5.75f;
                if (t < *(float *)(data + 0x70)) {
                    *(float *)(data + 0x70) = t;
                }
            }
            {
                char *p19 = moby + 0x10;
                float f1v = sp20[0];
                float f0v = sp20[1];
                float fa;
                fa = func_L00_001FF860(f1v - *(float *)(moby + 0x10), f0v - *(float *)(moby + 0x14));
                f21 = fa;
                f0 = func_001F9D48(moby + 0x10, sp20);
                f20 = f21;
                if (f0 < 7.0f) {
                    f20 = func_001FA748(f21, *(float *)(data + 0x80));
                }
                g = func_001F9F90(f20);
                h = g * *(float *)(data + 0x70);
                sp70[0] = h;
                g = func_001F9FA8(f20);
                *(int *)&sp70[2] = 0;
                h = g * *(float *)(data + 0x70);
                func_001F9BD8(p19, p19, sp70);
                sp70[1] = h;
            }
            if (func_001F9850(0x1E) < *(int *)(data + 0x84)) {
                f0 = *(float *)&D_0015EE6C;
                f2 = func_001FA748(*(float *)(data + 0x78), f0 * 5.23598766f);
                *(float *)(data + 0x78) = f2;
                *(float *)(moby + 0x18) = *(float *)(moby + 0x18) - *(float *)(data + 0x74);
                *(float *)(data + 0x74) = func_001F9FA8(*(float *)(data + 0x78)) * 0.100000001f;
                *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + *(float *)(data + 0x74);
            }
            if (func_001F9850(0x1E) < *(int *)(data + 0x84)) {
                f2 = func_00214358(moby + 0x10, 0, 0.5f);
                if (f2 < *(float *)(D_0013E633 + 0x10F5)) {
                    f2 = *(float *)(D_0013E633 + 0x10F5);
                }
                x = f2 - *(float *)(moby + 0x18);
                if (x < 0.5f && -3.0f < x) {
                    func_L00_0025C918((float *)(moby + 0x18), (float *)(data + 0x7C),
                                      f2 + 0.579999983f,
                                      *(float *)&D_0015EE64 * 0.0199999996f,
                                      *(float *)&D_0015EE64 * 0.300000012f,
                                      *(float *)&D_0015EE6C * 4.0f);
                }
            }
            if (func_001F9850(0x1E) < *(int *)(data + 0x84)) {
                f20 = 0.300000012f;
                sb0[0] = func_001F9F90(f21) * f20;
                sb0[1] = func_001F9FA8(f21) * f20;
                sb0[3] = 5627.9248f;
                sb0[2] = 1.20000005f;
                func_L00_0025A8C0(sp80, moby, 0x10001, 1.0f, sb0);
                qcopy(c0, moby + 0x10);
                c0[2] = c0[2] + 0.400000006f;
                if (func_L00_001F2BE8(0.400000006f, c0, 0x10, moby, sp80) != 0) {
                    int dd = D_L12_00174358;
                    if (dd != 0 && *(short *)(dd + 0xA6) != 0x4F5 && dd != *(int *)(moby + 0xB8)) {
                        moby[0x20] = 2;
                        func_L00_0025B040((unsigned char *)moby, 1.0f);
                        return;
                    }
                }
                qcopy(c0, moby + 0x10);
                c0[2] = c0[2] + 0.349999994f;
                if (func_L00_001F10E0(0.349999994f, c0, 0, moby) != 0) {
                    moby[0x20] = 2;
                }
            }
            func_L00_0025B040((unsigned char *)moby, 1.0f);
            return;
        }
        if (state == 0) {
            char *p;
            qcopy(data + 0x60, moby + 0x10);
            *(short *)(moby + 0x34) = *(unsigned short *)(moby + 0x34) | 0x1000;
            moby[0x20] = 1;
            *(float *)(data + 0x80) = 0.69813168f;
            p = *(char **)(moby + 0xB8);
            if (p != 0 && p[0xBC] != 0) {
                *(float *)(data + 0x80) = -*(float *)(data + 0x80);
            }
            *(int *)(data + 0x84) = 0;
            func_L00_0025B040((unsigned char *)moby, 1.0f);
            return;
        }
        if (state == 2) {
            func_L00_0025F4A8(moby, D_L12_0015F660, 0, 0.0f, 0.0f, 5, 0xF, 0x19, 2.0f, 1.0f, 4.0f, 2, 1.0f, 7.0f, 0, 9, 0, -1);
            func_0020D678(moby);
            return;
        }
        func_L00_0025B040((unsigned char *)moby, 1.0f);
        return;
    }
}
