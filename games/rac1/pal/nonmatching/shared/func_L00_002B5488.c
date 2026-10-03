/* NON_MATCHING func_L00_002B5488 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: BYTES 6/1284 (99.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002B5488 (UpdateMoby_167): 5-state switch on moby[0x20], with a pre-pass computing `flag` from game-s
 *   Only difference known: retail emits `daddu $19,$6,$0; movn $19,$0,$2` (flag=1 then clear if short at +0x41E se
 *   Needed per-use locals `char *padN = D_0013E633 + 0xE1D;` (separate pseudos) to get the lui/addiu rematerialisa
 *   Round fz6/x02: best p14.c = BYTES 6/1284 (same size). Fixed: flag via 'if (short==0) flag=1;' (gives movn-free
 */
extern char D_0013E633[] NOT_SDA;
extern char D_0013F450[] NOT_SDA;
extern int func_001F9850(int);
extern int func_L00_0020DB30(int);
extern void func_0020D678(int);
extern void func_001F49B0(void (*)(void), void *);
extern int func_001F9908(int *arg0);
extern void func_L00_002B4918(void);
extern void func_L00_002B4F40(char *);

/* update moby 167 */
void func_L00_002B5488(char *m) {
    float *d = *(float **)(m + 0x78);
    int *di = (int *)d;
    char *pad = D_0013E633 + 0xE1D;
    char *pad2;
    char *pad3;
    char *pad5;
    char *pad6;
    char *pad4;
    int flag = 0;
    int t;
    int t2;
    if (*(int *)(pad + 0x11A8) == 3) {
        if (*(unsigned char *)(m + 0x20) == 1 || *(unsigned char *)(m + 0x20) == 3) {
            t = *(int *)(pad + 0x2084);
            if ((t >= 0xB && t <= 0xE)
                || (t == 0x1C && (*(unsigned char **)(pad + 0x2080))[0x52] == (*(unsigned char **)(pad + 0x2080))[0x53]
                    && *(float *)(pad + 0xAA8) >= 6.0f && *(float *)(pad + 0xAA8) <= 16.0f)) {
                if (*(short *)(pad + 0x41E) == 0) flag = 1;
            }
            pad2 = D_0013E633 + 0xE1D;
            if (*(int *)(pad2 + 0x2084) == 0x10) {
                if (*(int *)(pad2 + 0x198) < func_001F9850(0x2C)) flag = 1;
            }
            if (*(int *)(pad2 + 0x2084) == 0x22) {
                if (*(int *)(pad2 + 0x198) < func_001F9850(0xF)
                    || (func_001F9850(0x21) < *(int *)(pad2 + 0x198) && *(short *)(pad2 + 0x30E) != 0)) {
                    flag = 1;
                }
            }
            pad4 = D_0013E633 + 0xE1D;
            t2 = *(int *)(pad4 + 0x2084);
            if (t2 == 8 || t2 == 0x81) flag = 1;
        }
    }
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        *(float *)(m + 0x18) = *(float *)(m + 0x18) + 0.5f;
        *(unsigned char *)(m + 0x20) = 1;
        *(unsigned char *)(m + 0x30) = 0xFF;
        *(unsigned short *)(m + 0x34) |= 1;
        break;
    case 1:
        if (func_L00_0020DB30(3) != 3) {
            func_0020D678((int)m);
            return;
        }
        if (flag) {
            *(unsigned char *)(m + 0x20) = 2;
            di[0] = func_001F9850(2);
            d[0x30 / 4] = 0.0125f / (float)func_001F9850(2);
            d[0x34 / 4] = 0.2f / (float)func_001F9850(2);
            d[0x38 / 4] = 0.05f / (float)func_001F9850(2);
            d[0x3C / 4] = 0.025f / (float)func_001F9850(2);
            d[0x20 / 4] = 0;
            d[0x24 / 4] = 0;
            d[0x28 / 4] = 0;
            d[0x2C / 4] = 0;
        }
        break;
    case 2:
        d[0x20 / 4] += d[0x30 / 4];
        d[0x24 / 4] += d[0x34 / 4];
        d[0x28 / 4] += d[0x38 / 4];
        d[0x2C / 4] += d[0x3C / 4];
        pad3 = D_0013E633 + 0xE1D;
        di[2] = *(int *)(pad3 + 0x1180);
        func_001F49B0(func_L00_002B4918, m);
        if (func_001F9908(di)) {
            float c0 = 0.0125f, c1 = 0.2f, c2 = 0.05f, c3 = 0.025f;
            *(unsigned char *)(m + 0x20) = 3;
            d[0x2C / 4] = c3;
            d[0x20 / 4] = c0;
            d[0x24 / 4] = c1;
            d[0x28 / 4] = c2;
            di[3] = 0;
        }
        break;
    case 3:
        pad5 = D_0013E633 + 0xE1D;
        di[2] = *(int *)(pad5 + 0x1180);
        func_001F49B0(func_L00_002B4918, m);
        if (flag == 0) {
            *(unsigned char *)(m + 0x20) = 4;
            di[0] = func_001F9850(8);
            d[0x30 / 4] = 0.0125f / (float)func_001F9850(8);
            d[0x34 / 4] = 0.2f / (float)func_001F9850(8);
            d[0x38 / 4] = 0.05f / (float)func_001F9850(8);
            d[0x3C / 4] = 0.025f / (float)func_001F9850(8);
        } else {
            d[0x20 / 4] = 0.0125f;
            func_L00_002B4F40(m);
            return;
        }
        break;
    case 4:
        if (func_001F9908(di)) {
            *(unsigned char *)(m + 0x20) = 1;
        } else {
            d[0x20 / 4] -= d[0x30 / 4];
            d[0x24 / 4] -= d[0x34 / 4];
            d[0x28 / 4] -= d[0x38 / 4];
            d[0x2C / 4] -= d[0x3C / 4];
            pad6 = D_0013E633 + 0xE1D;
        di[2] = *(int *)(pad6 + 0x1180);
            func_001F49B0(func_L00_002B4918, m);
        }
        break;
    }
}
