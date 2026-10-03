/* NON_MATCHING func_L00_002B8208 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: SIZE ours 1156 / retail 1160, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002B8208: UpdateMoby_175 state machine (states 1 and 2) for a stat-screen moby: pad input, counters i
 *   Best candidate p6.c (SIZE 1156/1160; first diff at +0x84 is only the state-byte reload: retail loads lbu $3,0x
 *   Wordings tried for that join: `st == 1` on int/unsigned char, switch, `(st & 0xFF) == 1`; none reproduce the s
 */
extern int func_L00_0028EB98(void *, int);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);
extern int func_0022ED80(int, int, int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_002B7EC0(void *, float);
extern void func_L00_0028EBF0(int);
extern void func_L00_002B8690(void *);
extern int D_L00_001615A4;
extern int D_L00_0016159C;
extern int D_0015EFA4 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern char D_0013E633[] NOT_SDA;
extern char D_0013A5E0[] NOT_SDA;
extern char D_0014171B[] NOT_SDA;

// per-frame update of the stat-screen moby: handles its state machine and input
void func_L00_002B8208(unsigned char *moby) {
    int *d;
    float *df;
    unsigned char *pad;
    unsigned char *q;
    unsigned char *base;
    int st;
    int r;
    int a;
    int b;
    int m;
    int i;
    int e;
    if (moby == 0) return;
    d = *(int **)(moby + 0x78);
    if (d == 0) return;
    df = (float *)d;
    if (0.0f == df[2]) df[2] = 30.0f;
    if (moby[0x53] == 2) moby[0x20] = 3;
    D_L00_001615A4 = 1;
    if (moby[0x20] == 0) {
        d[1] = -1;
        moby[0x20]++;
        D_L00_0016159C = 1;
    }
    st = moby[0x20];
    if (st == 1) {
        if (moby[0x70] & 2) {
            moby[0x20] = st + 1;
            d[3] = 0;
        }
    } else if (st == 2) {
        base = (unsigned char *)D_0013E633 + 0xE1D;
        if (moby[0x53] == 3 && base[0x20AC] == 0) {
            D_L00_001615A4 = 0;
            d[6]++;
        } else {
            d[6] = 0;
        }
        r = func_L00_0028EB98(moby, d[1]);
        if (r == 0) {
            pad = (unsigned char *)D_0013A5E0 + 0x2460;
            base = (unsigned char *)D_0013E633 + 0xE1D;
            m = *(int *)(pad + 0x1A4) & *(int *)(base + 0x10A0);
            if ((m != 0 || ((*(int *)(pad + 0x1A0) & *(int *)(base + 0x10A0)) != 0 && d[3] >= 0x11)) && base[0x20AC] == 0) {
                if (m != 0) {
                    d[4] = func_001F9850(func_L00_00258BC8(0xF, 0x2D));
                }
                q = (unsigned char *)D_0014171B + 0x65;
                if (!(0xFFFE < *(unsigned short *)(q + 0x70))) {
                    *(unsigned short *)(q + 0x70) = *(unsigned short *)(q + 0x70) + 1;
                }
                if (func_001F9850(D_0015EFA4) / 600 > *(unsigned short *)(q + 0x72)) {
                    *(unsigned short *)(q + 0x72) = func_001F9850(D_0015EFA4) / 600;
                }
                *(int *)(q + 0x74) = *(int *)(q + 0x74) | (1 << D_0015EE84_m) | 0x80000000;
                if (moby[0x53] != 3) {
                    func_00213DE0(moby, 3, 0, func_001F9850(3));
                }
                d[1] = func_0022ED80(d[0], 1, (int)moby);
                i = d[0] + 1;
                d[0] = i;
                i = i << 5;
                a = func_001FA898_r(*(float *)(*(char **)(*(char **)(moby + 0x24) + 0x28) + i) * 10.0f);
                b = func_001FA898_r(*(float *)(*(char **)(*(char **)(moby + 0x24) + 0x28) + i + 4) * 10.0f);
                if (!(d[0] < *(unsigned char *)(*(char **)(moby + 0x24) + 0xD) && a == 0x1E && b == a)) {
                    d[0] = 0;
                }
                func_L00_002B7EC0(moby, df[2]);
                d[3] = 0;
            } else if (d[1] >= 0) {
                goto done;
            } else {
                pad = (unsigned char *)D_0013A5E0 + 0x2460;
                base = (unsigned char *)D_0013E633 + 0xE1D;
                if ((*(int *)(pad + 0x1A0) & *(int *)(base + 0x10A0)) != 0 && base[0x20AC] == 0) {
                    d[3]++;
                } else {
                    d[3] = 0;
                }
                if (moby[0x53] != 1) {
                    func_00213DE0(moby, 1, 0, func_001F9850(5));
                }
            }
        } else {
            d[3]++;
            base = (unsigned char *)D_0013E633 + 0xE1D;
            if ((*(int *)(D_0013A5E0 + 0x2600) & *(int *)(base + 0x10A0)) != 0 || !(func_001F9850(0x1E) < d[6])) {
                if (base[0x20AC] == 0) {
                    if ((D_L00_0015F6B0 & 3) == 0) {
                        func_L00_002B7EC0(moby, df[2]);
                    }
                    goto fin;
                }
            }
done:
            if (moby[0x53] != 1) {
                func_00213DE0(moby, 1, 0, func_001F9850(5));
            }
            e = d[1];
            if (e != -1) {
                unsigned char *ent = (unsigned char *)D_0013E633 + 0x1D + e * 0x70;
                if (*(char **)(ent + 0x88) == moby && ent[0x74] != 0) {
                    func_L00_0028EBF0(e);
                }
            }
            d[3] = 0;
            d[1] = -1;
        }
    }
fin:
    base = (unsigned char *)D_0013E633 + 0xE1D;
    if (*(int *)(base + 0x10B4) != 3) func_L00_002B8690(moby);
}
