/* NON_MATCHING func_L00_001EB890 -- src/overlays/shared/camera_001EB508.c
 * Best so far: SIZE ours 768 / retail 764, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_001EB890: camera switch: picks blend speeds (D_L00_00166D80 fields 0x270/0x273/0x288/0x294/0x2F4) by 
 *   Best: p8.c (same size 764, structure and constants match; 407 bytes differ only by register roles). Retail kee
 *   What would unblock: whatever makes the allocator rank q above cam (priority tie); reordering declarations and 
 */
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9A98(void *, void *, int);
extern char *D_L00_0015F050 MACRO_ADDR;
extern char D_L00_00169490[];
extern int D_L00_0016C16C;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;

/* Switches the active camera to the given one, configuring blend speeds by mode. */
void func_L00_001EB890(char *c) {
    char *q;
    char *g = (char *)&D_L00_00166D80;
    char *cam = *(char **)(g + 0x180);
    int t = 0;
    int mode;
    float *d;
    int x;
    float f;
    char *g2;

    x = *(int *)(D_L00_0015F050 + (*(short *)(c + 0x84) << 5) + 0x1C);
    if (x) {
        t = *(unsigned char *)(x + 0x1D);
    }
    mode = *(short *)(cam + 0x7E);
    if (mode == 4) {
        *(short *)(c + 0x8E) = 1;
        q = c + 0x30;
    } else if (mode == 2 || t == 1 || t == 5) {
        if (t == 1) {
            f = *(float *)(c + 0x78);
            D_L00_00166D80.c = 0;
            if (f > 0.0f) {
                D_L00_00166D80.f0 = f;
                D_L00_00166D80.f1 = f;
            } else {
                D_L00_00166D80.f0 = 0.018000f;
                D_L00_00166D80.f1 = 0.018000f;
            }
        } else if (t == 5) {
            if (*(float *)(c + 0x78) > 0.0f) {
                g[0x273] = 2;
                *(int *)(g + 0x2F4) = func_001FA898_r(*(float *)(c + 0x78));
            } else {
                g[0x273] = 2;
                *(int *)(g + 0x2F4) = 0x28;
            }
        }
        D_L00_00166D80.s = D_L00_00166D80.s ? 2 : 1;
        q = c + 0x30;
    } else if (mode == 3 || mode == 5 || t == 3 || t == 6) {
        qcopy(c + 0x30, cam + 0x30);
        qcopy(c, cam);
        qcopy(c + 0x10, cam + 0x10);
        qcopy(c + 0x20, cam + 0x20);
        c[0x7D] = 2;
        q = c + 0x30;
        if (*(short *)(cam + 0x7E) == 5 || t == 6) {
            f = *(float *)(c + 0x78);
            D_L00_00166D80.c = 0;
            if (f > 0.0f) {
                D_L00_00166D80.f0 = f;
                D_L00_00166D80.f1 = f;
            } else {
                D_L00_00166D80.f0 = 0.018000f;
                D_L00_00166D80.f1 = 0.018000f;
                if (D_0015EE84_m == 1) {
                    D_L00_00166D80.f0 = 0.010300f;
                    D_L00_00166D80.f1 = 0.010300f;
                }
            }
            if (D_L00_00166D80.s == 0) {
                D_L00_00166D80.s = 1;
            } else {
                D_L00_00166D80.s = 2;
            }
        }
    } else {
        *(short *)(c + 0x8E) = 1;
        q = c + 0x30;
    }
    g2 = (char *)&D_L00_00166D80;
    *(short *)(cam + 0x7E) = 0;
    cam[0x7D] = 0;
    *(short *)(cam + 0x8E) = 0;
    *(char **)(g2 + 0x184) = cam;
    func_001F9A98(D_L00_00169490, D_L00_00169490 - 0x280, 0x280);
    *(char **)(*(char **)(g2 + 0x184) + 0x70) = D_L00_00169490;
    *(char **)(g2 + 0x180) = c;
    *(char **)(c + 0x70) = D_L00_00169490 - 0x280;
    *(int *)(g2 + 0x398) = 0;
    func_001EC270(c);
    func_001EC038();
    if (D_L00_0016C16C == 0) {
        qcopy(g2 + 0x140, q);
    }
    d = (float *)(c + 0x64);
    d[0] = *(float *)(c + 0x30);
    d[1] = *(float *)(q + 4);
    d[2] = *(float *)(q + 8);
}
