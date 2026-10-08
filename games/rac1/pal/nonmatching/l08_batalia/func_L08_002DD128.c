/* NON_MATCHING func_L08_002DD128 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: BYTES 19/788 (97.6% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int Q_2dd128 __attribute__((mode(TI)));
extern int D_L08_0015F6A8 NOT_SDA;
extern int D_L08_00179C90[];
extern int D_L08_0015F674 MACRO_ADDR;
extern int D_0013A5E0_d[] __asm__("D_0013A5E0") NOT_SDA;
extern float D_0015EE6C_d __asm__("D_0015EE6C") MACRO_ADDR;
extern void func_0020D960(void *, int, void *);
extern float func_001FA748(float, float);
extern void func_L00_0025AFA8(void *, void *);
extern void func_001F9BC0(void *);
extern void func_L00_00262DF0_i(int, void *, float, void *) __asm__("func_L00_00262DF0");
extern float func_001F9D10(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_002617B0(void *, void *, void *, void *);
extern void func_L08_002DD4F8(char *);
extern void func_L08_002DDB68(char *moby);
extern int func_L08_002DD9C0(char *m);
extern void func_L08_002DD818(char *moby);
extern void func_L08_002DD440(char *moby);

/* Platform moby update: toggles visibility, spins, records its start pose, then rides its path while the
 * player stands on it; at the end of the path it can hand over to the cutscene state. */
void func_L08_002DD128(char *m) {
    char *d = *(char **)(m + 0x78);
    float pos[4];
    float rot[4];
    float vec[4];
    float tgt[4];
    float *rp;
    qcopy(pos, m + 0x10);
    rp = rot;
    qcopy(rp, m + 0x40);
    if (D_L08_0015F6A8 != 2 && *(int *)(d + 0x124) != 0) {
        m[0x31] = 1;
        *(unsigned short *)(m + 0x34) &= 0xFFFE;
    } else {
        m[0x31] = 0;
        *(unsigned short *)(m + 0x34) |= 1;
    }
    if (((unsigned char *)d)[0xA1] == 0) {
        func_0020D960(m, 0, d + 0xA0);
    }
    *(float *)(d + 0x110) = func_001FA748(*(float *)(d + 0x110), D_0015EE6C_d * 1.5707964f);
    func_L00_0025AFA8(d + 0xB0, d + 0x110);
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        func_001F9BC0(d + 0x110);
        if (*(int *)(d + 0x80) == -1) {
            qcopy(d + 0x60, m + 0x10);
            qcopy(d + 0x70, m + 0x40);
            m[0x20] = 4;
            *(int *)(d + 0x124) = 1;
        } else {
            *(int *)(d + 0x100) = -1;
            *(short *)(d + 0xE0) = -1;
            func_L08_002DD4F8(m);
            m[0x20] = 2;
            ((unsigned char *)m)[0x30] = 0xFF;
        }
        break;
    case 2: {
        char *q = D_0013E633 + 0xE1D;
        if (*(char **)(q + 0x2FC) == m) {
            if (D_L08_00179C90[0] == 0 && D_L08_00179C90[9] == -1) {
                qcopy(tgt, q + 0x80);
                func_L00_00262DF0_i(*(int *)(d + 0x8C), tgt, 0.5f, tgt);
                if (func_001F9D10(q + 0x80, tgt) < 0.001f) {
                    func_L08_002DDB68(m);
                    if ((D_0013A5E0_d[0x2604 / 4] & 0x10) && *(short *)(q + 0x30E) == 0 && D_L08_0015F674 == 10) {
                        m[0x20] = 3;
                    }
                }
            }
        } else {
            func_L08_002DD4F8(m);
        }
        break;
    }
    case 3: {
        char *q = D_0013E633 + 0xE1D;
        *(short *)(q + 0x1F2) = 5;
        *(short *)(q + 0x1F4) = 5;
        *(short *)(q + 0x22DA) = *(unsigned short *)(d + 0x8C);
        if (func_L08_002DD9C0(m)) {
            func_L08_002DD4F8(m);
            m[0x20] = 2;
        }
        break;
    }
    }
    func_L08_002DD818(m);
    if (*(int *)(d + 0x8C) != -1) {
        func_L08_002DD440(m);
    }
    func_001F9BF0(vec, m + 0x10, pos);
    func_L00_002617B0(d + 0x20, vec, rp, m + 0x40);
}
