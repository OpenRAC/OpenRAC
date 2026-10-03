/* NON_MATCHING func_L06_0030B248 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: BYTES 16/416 (96.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby update, states 0/1/2: state 1 waits for func_L00_0025B478's result to have a positive float at +0x2C; sta
 *   p4/p7 differ in 16 bytes: the two CSE'd pointers moby+0x10/moby+0x40 get the right registers ($s2/$s3) only wh
 *   Unblock: a wording that gives p10 the lower saved register while p40 is computed after func_L01_00279790; the 
 */
extern char D_L06_0015F660[];
extern char *func_L00_0025B478(void *, int, int);
extern void func_0022ED80(int, int, void *);
extern void func_L01_00279790(void *);
extern void func_L00_00265050(void *, int, void *, void *, int, int, float, void *, void *, void *);
extern char *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L00_00251E30(void *);
extern void func_0020D678(void *);

/* Moby update: wait for a target, then spawn an effect moby and delete itself. */
void func_L06_0030B248(unsigned char *moby) {
    int found = 0;
    char *r = func_L00_0025B478(moby, 0x10000, 0);

    switch (moby[0x20]) {
    case 0:
        moby[0x20] = 1;
        break;
    case 1:
        if (r != 0 && *(float *)(r + 0x2C) > 0.0f) {
            found = 1;
        }
        if (found) {
            moby[0x20] = 2;
        }
        break;
    case 2: {
        float z = 0.0f;
        char *g;
        char *m;
        char *p10;
        char *p40;

        func_0022ED80(0, 0, moby);
        p40 = moby + 0x40;
        p10 = moby + 0x10;
        g = D_L06_0015F660;
        func_L01_00279790(moby);
        func_L00_00265050(moby, 0x67E, p10, p40, 0, 0, z, g, g, g);
        func_L00_00265050(moby, 0x67E, p10, p40, 0, 0, z, g, g, g);
        m = func_0020D348_m(0x67D);
        if (m != 0) {
            m[0x31] = 1;
            *(short *)(m + 0x32) = 0xFF;
            qcopy(m + 0x10, p10);
            qcopy(m + 0x40, p40);
            *(long *)(m + 0x38) = *(long *)(moby + 0x38);
            func_L00_00251E30(m);
        }
        func_0020D678(moby);
        break;
    }
    }
}
