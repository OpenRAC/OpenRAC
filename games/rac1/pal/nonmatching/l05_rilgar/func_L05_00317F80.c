/* NON_MATCHING func_L05_00317F80 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: SIZE ours 2316 / retail 2344, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   func_L05_00317F80 (2344 bytes, level 05 hoverboard_girl update): a state machine on moby[0x20] (states 0-3) th
 *   Where it differs: the frame. Retail saves $f20-$f23 and six GP registers; ours saves $f20-$f22 and one more GP
 *   Unblock: the saved-float set. A way to keep $f23 live across the calls that does not turn it into a propagated
 *   Runs used: 7 of 16 (p0-p5 plus two intermediate candidates; p1.diff.txt, p2.diff.txt, p3.diff.txt, p4.diff.txt
 */
extern char D_L05_00161F98[];
extern char *D_L05_001B1E78[];
extern char D_L05_00215F00[];
extern char D_L05_00215F20[];
extern char D_0013D355[];
extern char D_0013D50F[];
extern char D_0013E633[];
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern int D_L05_0015F6A8 MACRO_ADDR;
extern int D_L05_0015F6B0 MACRO_ADDR;
extern short D_L05_00161F90;
extern void func_L05_00317E58(char *arg);
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);
extern float func_00214358(void *, int, float);
extern int func_L00_00267618_676E8(void *) __asm__("func_L00_00267618");
extern int func_L00_002676E8(void *, void *);
extern void func_L02_0025D750(char *moby);
extern int func_L00_00267290(void *, void *);
extern void func_L01_00279398(float, void *);
extern void func_L00_00217718(void *, void *, int, int);
extern void func_L00_002664B0(int, int);
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_002140B0(int);
extern void func_L00_00264DB8(int, int);
extern int func_0022EE28(int, int, int);
extern int func_0020BFC8(int, int);
extern float func_001F9D48(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9CB8(void *a);
extern float func_001FA748(float, float);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA790(float, float);
extern float func_001F9CE8(void *);
extern void func_L05_00317CD8(char *, int);
extern void func_L00_00263950(char *, char *, int, float, float);

/* Hoverboard girl update: steers her along the track, runs her state machine and sets her animation. */
void func_L05_00317F80(char *moby) {
    char *data = *(char **)(moby + 0x78);
    char *pos = moby + 0x10;
    char *g = D_0013E633 + 0xE1D;
    char *h;
    char *ps;
    char buf[0x30];
    int moving = 1;
    int state;
    int m3;
    int w;
    int r;
    int v;
    float a, k, f20, f21, f22, f23, dx, dy;

    func_L05_00317E58(moby);
    state = *(unsigned char *)(moby + 0x20);
    if (state == 1) goto L180A8;
    if (state >= 2) {
        if (state == 3) goto L18328;
        goto L183A0;
    }
    if (*(unsigned char *)(moby + 0x53) == 0) goto L1801C;
    v = func_001F9850(0x14);
    func_00213DE0(moby, 0, v, 0);
    pos = moby + 0x10;
L1801C:
    a = func_00214358(pos, 0, 0.5f);
    *(float *)(moby + 0x18) = a;
    ps = D_0013D605 + 0xB3;
    if (*(int *)(ps + 0x10C) == 0 || *(int *)(ps + 0x11C) == 0) {
        r = func_L00_00267618_676E8(moby);
        *(short *)(D_L05_001B1E78[r] + 4) = 13;
    }
L1807C:
    *(char **)(data + 0x20) = D_L05_00161F98;
    *(unsigned char *)(moby + 0x20) = 1;
    func_L00_002676E8(moby, data);
    func_L02_0025D750(moby);
    m3 = *(unsigned char *)(moby + 0x53);
    goto L183C8;

L180A8:
    if (*(int *)(g + 0x2084) == 0x1D) goto L18160;
    r = func_L00_00267290(moby, data);
    if (r != 0) {
        func_L01_00279398(2.0f, moby);
        *(unsigned char *)(moby + 0x20) = 3;
    }
    if (*(short *)(data + 0x36) != 3) goto L18160;
    *(short *)(data + 0x4) = *(unsigned short *)(data + 0x36);
    *(int *)(data + 0x38) = D_L05_0015F6B0;
    *(short *)(data + 0x36) = 2;
    *(unsigned char *)(moby + 0xBC) = 0;
    func_L00_00217718(D_L05_00215F00, D_L05_00215F00 + 0x10, 0, 1);
    qcopy(g + 0x1D00, D_L05_00215F00 + 0x20);
    qcopy(g + 0x1D10, D_L05_00215F00 + 0x30);
    *(unsigned char *)(g + 0x20AC) = moving;
    func_L00_002664B0(2, 6);

L18160:
    w = *(unsigned char *)(moby + 0xBC);
    if (w == 1) {
        func_L00_00217718(D_L05_00215F20, D_L05_00215F20 + 0x10, 0, 1);
        func_L00_002664B0(0, 8);
        if (D_0013D355[0x13B] == 0) {
            unsigned short u = *(unsigned short *)(data + 0x36);
            *(int *)(data + 0x38) = D_L05_0015F6B0;
            *(short *)(data + 0x36) = 4;
            *(short *)(data + 0x4) = u;
            *(unsigned char *)(data + 0x8) = w;
        }
        *(unsigned char *)(moby + 0xBC) = 0;
    }
    w = *(unsigned char *)(moby + 0xBC);
    if (w == 2) {
        func_L00_002664B0(0, 8);
        func_L00_00217718(D_L05_00215F20, D_L05_00215F20 + 0x10, 0, 1);
        *(unsigned char *)(moby + 0xBC) = 0;
    }
    w = *(unsigned char *)(moby + 0xBC);
    if (w == 3) {
        func_L00_002664B0(0, 8);
        *(unsigned char *)(moby + 0xBC) = 0;
        func_L00_00217718(D_L05_00215F00, D_L05_00215F00 + 0x10, 0, 1);
        qcopy(D_0013E633 + 0x2B1D, D_L05_00215F00 + 0x20);
        qcopy(D_0013E633 + 0x2B1D + 0x10, D_L05_00215F00 + 0x30);
    }
    m3 = *(unsigned char *)(moby + 0x53);
    if (m3 == 0) goto L18274;
    if (m3 != 2) goto L182D8;
    goto L18274;

L18274:
    r = func_001F9908((int *)(data + 0x244));
    if (r == 0) goto L182D8;
    a = func_002140F8(1200.0f, 2400.0f);
    a = func_001F9878(a);
    *(int *)(data + 0x244) = func_001FA898_r(a);
    m3 = *(unsigned char *)(moby + 0x53);
    if (m3 == 1) goto L183C8;
    v = func_001F9850(10);
    func_00213DE0(moby, 1, v, 0);
    m3 = *(unsigned char *)(moby + 0x53);
    goto L183C8;

L182D8:
    if ((*(unsigned char *)(moby + 0x70) & 2) == 0) goto L183C4;
    r = 0;
    if (func_002140B0(2) == 0) r = 2;
    if (*(unsigned char *)(moby + 0x53) == r) goto L183C8;
    v = func_001F9850(10);
    func_00213DE0(moby, r, v, 0);
    m3 = *(unsigned char *)(moby + 0x53);
    goto L183C8;

L18328:
    *(unsigned char *)(moby + 0x20) = moving;
    if (*(short *)(data + 0x4) != 4) goto L183C4;
    D_0013D355[0x13B] = moving;
    v = func_001F9850(0x12C);
    func_L00_00264DB8(0x1395, v);
    if (D_0013D50F[0xA] != 0) {
        func_0022EE28(1, 0, 0);
        func_L00_00264DB8(0x53DB, -1);
    }
    func_0020BFC8(0, -1);
    goto L183C4;

L183A0:
    if (*(unsigned char *)(moby + 0xBC) == 0) goto L183C4;
    func_L00_00217718(D_L05_00215F20, D_L05_00215F20 + 0x10, 0, 1);
    *(unsigned char *)(moby + 0xBC) = 0;

L183C4:
    m3 = *(unsigned char *)(moby + 0x53);

L183C8:
    f22 = 0.02f;
    f23 = 0.3f;
    moving = 0;
    if (m3 == 0) goto L183F4;
    if (m3 != 2) goto L185C4;

L183F4:
    h = D_0013E633 + 0xE9D;
    a = func_001F9D48(pos, h);
    moving = 1;
    if (!(a < 8.0f)) goto L184BC;
    dx = *(float *)(g + 0xD0) - *(float *)(moby + 0x10);
    dy = *(float *)(g + 0xD4) - *(float *)(moby + 0x14);
    a = func_L00_001FF860(dx, dy);
    a = func_001FA850(*(float *)(moby + 0x48), a);
    if (!(a < 1.5707963f)) {
        v = *(int *)(data + 0x248);
        goto L184C0;
    }
    a = func_001F9CB8(h + 0x80);
    if (!(0.01f < a)) goto L184AC;
    *(int *)(data + 0x248) = func_001F9850(0x78);
    goto L184DC;
L184AC:
    func_001F9908((int *)(data + 0x248));
    goto L184DC;
L184BC:
    v = *(int *)(data + 0x248);
L184C0:
    if (v == 0) goto L184DC;
    *(int *)(data + 0x248) = 0;
    qcopy(data + 0x250, D_0013E633 + 0xEED);
L184DC:
    r = func_001F9908((int *)(data + 0x24C));
    if (r == 0) {
        v = *(int *)(data + 0x248);
        goto L18584;
    }
    f21 = 0.017453292f;
    a = func_002140F8(180.0f, 300.0f);
    a = func_001F9878(a);
    *(int *)(data + 0x24C) = func_001FA898_r(a);
    a = func_002140F8(-90.0f, 90.0f);
    func_001FA748(*(float *)(moby + 0x48), a * f21);
    f20 = func_002140F8(0.0f, 30.0f);
    k = f20 * f21;
    func_00215C00(data + 0x250, 6.0f, f20, k);
    func_001F9BD8(data + 0x250, data + 0x250, pos);
    v = *(int *)(data + 0x248);

L18584:
    if (v == 0) goto L185B8;
    qcopy(buf, D_0013E633 + 0xEED);
    f22 = 0.03f;
    f23 = 0.3f;
    goto L185C4;
L185B8:
    qcopy(buf, data + 0x250);

L185C4:
    if (!moving) goto L186DC;
    qcopy(buf + 0x10, pos);
    *(float *)(buf + 0x18) = *(float *)(buf + 0x18) + 1.0f;
    func_001F9BF0(buf + 0x20, buf, buf + 0x10);
    a = func_L00_001FF860(*(float *)(buf + 0x20), *(float *)(buf + 0x24));
    f20 = func_001FA790(a, *(float *)(moby + 0x48));
    k = func_001F9CE8(buf + 0x20);
    a = func_L00_001FF860(k, *(float *)(buf + 0x28));
    if (1.5707963f < f20) {
        dy = -a;
        f20 = 1.5707963f;
    } else {
        dy = -a;
        if (f20 < -1.5707963f) f20 = -1.5707963f;
    }
    if (0.5235988f < dy) dy = 0.5235988f;
    else if (dy < -0.5235988f) dy = -0.5235988f;
    *(float *)(data + 0xA4) = dy;
    *(float *)(data + 0xA8) = f20 * 0.6f;
    *(float *)(data + 0x128) = f20 * 0.4f;

L186DC:
    h = D_0013E633 + 0xE9D;
    a = func_001F9D48(pos, h);
    if (!(a < 15.0f)) goto L187F8;
    g = h - 0x80;
    dx = *(float *)(h) - *(float *)(moby + 0x10);
    dy = *(float *)(h + 4) - *(float *)(moby + 0x14);
    a = func_L00_001FF860(dx, dy);
    a = func_001FA850(*(float *)(moby + 0x48), a);
    if (!(a < 1.2228f)) goto L187F8;
    if (*(int *)(g + 0x2084) == 11 && *(int *)(g + 0x198) == 15) {
        v = *(int *)(data + 0x260) + 1;
        *(int *)(data + 0x260) = v;
        if (!(v < 21)) *(int *)(data + 0x260) = 20;
        *(int *)&D_L05_00161F90 = *(int *)(data + 0x260);
    }
    if (*(int *)(g + 0x2084) == 4) {
        v = func_001F9850(0x14);
        if (v < *(int *)(g + 0x198)) {
            if (D_L05_0015F6B0 % 10 == 0) {
                v = *(int *)(data + 0x260) - 1;
                *(int *)(data + 0x260) = v;
                if (v < 0) *(int *)(data + 0x260) = 0;
                *(int *)&D_L05_00161F90 = *(int *)(data + 0x260);
            }
        }
    }
L187F8:
    m3 = D_L05_0015F6A8;
    if (m3 == 2) goto L18874;
    func_L05_00317CD8(moby, *(int *)(moby + 0x78));
    if (D_0015EEB0[0] != 0) *(float *)(data + 0xB0) = 2.75f;
    k = D_0015EE64;
    func_L00_00263950(moby, data + 0x40, 0, f22 * k, f23 * k);
    k = D_0015EE64;
    func_L00_00263950(moby, data + 0xC0, 1, f22 * k, f23 * k);

L18874:
    return;
}
