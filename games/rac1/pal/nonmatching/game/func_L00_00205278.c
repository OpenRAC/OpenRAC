/* Hero init (called once on the way into the first level). Looks for the first entry in the
   0x100-byte table between D_L00_00160098 and D_L00_0016009C with type 0 at +0xA6; for it,
   sets the hero object, stores a float at +0x18 and runs the object's setup, then copies
   16-byte blocks into the table at D_0013E15A + 0x36. Clears a byte in each 0xB0-byte entry of
   D_L00_0017A780 and calls func_001F9BC0 on six of its fields for each of 31 entries. Finally
   stores -1 in eight words at 0x2214..0x2234 and sets the timing fields at 0x2288..0x22B0. */
extern unsigned char D_0013F450[] NOT_SDA;
extern unsigned char D_0014171B[] NOT_SDA;
extern int D_L00_0016009C;
extern int D_L00_00160098;
extern int D_L00_00160600;
extern char D_L00_0017A780[];
extern char D_0013E15A[];
extern int D_0015EEA0;
extern float func_00214358(void *, int, float);
extern void func_L00_00212E70(void);
extern void func_L00_00208E98(void);
extern void func_001F9BC0_hi(void *) __asm__("func_001F9BC0");
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_001F99B0(void *, int, int);

void func_L00_00205278(void) {
    char *g = D_0013F450;
    int a = D_L00_0016009C;
    int b = D_L00_00160098;
    char *t = D_0013E15A + 0x36;
    char *u;
    int it;
    int k;
    int q;
    int n;

    func_001F99B0(g, 0, 0x2310);

    if ((unsigned int)b < (unsigned int)a) {
        int pa = b;
        while ((unsigned int)pa < (unsigned int)a) {
            char *p = (char *)pa;
            if (*(short *)(p + 0xA6) == 0) {
                float r;
                *(char **)(g + 0x2080) = p;
                r = func_00214358(p + 0x10, 0, 0.5f);
                if (0.0f < r) {
                    *(float *)(p + 0x18) = r;
                }
                *(unsigned short *)(p + 0x34) |= 2;
                *(char **)(g + 0xA88) = p;
                *(float *)(g + 0x98) = *(float *)(p + 0x48);
                for (n = 0; n < 16; n++) { (g + 0x80)[n] = (p + 0x10)[n]; }
                func_L00_00212E70();
                func_L00_00208E98();
                if (D_L00_00160600 == 0) {
                    for (n = 0; n < 16; n++) { (t)[n] = (g + 0x80)[n]; }
                    for (n = 0; n < 16; n++) { (t + 0x10)[n] = (g + 0x90)[n]; }
                }
                break;
            }
            pa += 0x100;
        }
    }

    q = *(int *)(D_0014171B + 0x45);
    if (q == 0) {
        *(int *)(D_0014171B + 0x45) = 10;
    }

    u = D_L00_0017A780;
    for (it = 0; it <= 30; it++) {
        u[1 + 0xB0 * it] = 0;
        func_001F9BC0_hi(u + 0x60 + 0xB0 * it);
        func_001F9BC0_hi(u + 0x40 + 0xB0 * it);
        func_001F9BC0_hi(u + 0x50 + 0xB0 * it);
        func_001F9BC0_hi(u + 0x90 + 0xB0 * it);
        func_001F9BC0_hi(u + 0x70 + 0xB0 * it);
        func_001F9BC0_hi(u + 0x80 + 0xB0 * it);
    }

    for (k = 0; k < 8; k++) {
        *(int *)(g + 0x2234 - 4 * k) = -1;
    }

    {
        int v = D_0015EEA0;
        int r1;
        int r2;
        int r3;
        *(int *)(g + 0x22A8) = v;
        *(float *)(g + 0x2288) = 2.125f;
        *(float *)(g + 0x228C) = 1.25f;
        *(int *)(g + 0x22AC) = 4;
        *(unsigned short *)(g + 0x22B0) = (unsigned short)v;
        r1 = func_001F9850(0xB4);
        r2 = func_001F9850(0x12C);
        r3 = func_L00_00258BC8(r1, r2);
        *(int *)(g + 0x1010) = r3;
        *(float *)(g + 0xD20) = 0.97f;
        *(float *)(g + 0x1014) = 0.007f;
        *(float *)(g + 0x1018) = 0.3f;
    }
}
