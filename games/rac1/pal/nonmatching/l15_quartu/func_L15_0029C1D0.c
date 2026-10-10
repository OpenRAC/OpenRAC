/* NON_MATCHING func_L15_0029C1D0 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 1200 / retail 1184, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Level-15 moby class 67 update: a five-case state machine (jump table on moby[0x20]), with a func_00215570 test
 *   Runs: 7 of 10 spent (p2 at 1204 is the last try of the block-local form).
 */
extern char *D_L15_00160058_m __asm__("D_L15_00160058") MACRO_ADDR;
extern unsigned char D_0014171B[] NOT_SDA;
extern unsigned char D_0013E633[];
extern char D_L15_001BBB40_c[] __asm__("D_L15_001BBB40");
extern int D_0015EE84 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L15_001BADE0[];
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_00215570(void *arg0, int arg1);
extern int func_0022ED80(int, int, int);
extern void func_L00_0028EBF0(int);

// Update for moby class 67 on level 15: a state machine keyed on the moby's state byte.
void func_L15_0029C1D0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    switch ((unsigned char)moby[0x20]) {
    case 0:
        qcopy(data, moby + 0x10);
        moby[0x20] = 1;
        break;
    case 1: {
        int f18 = 0;
        int idx = *(int *)(data + 0x1C);
        int r;
        *(int *)(data + 0x18) = 0;
        if (idx != -1) {
            char *e = D_L15_00160058_m + (idx << 8);
            if (*(short *)(e + 0xA6) == 0x4B9 && e[0x20] == 4) {
                f18 = 1;
            } else if (*(short *)(e + 0xA6) == 0x596) {
                f18 = (e[0x20] == 4);
            }
        }
        if (*(int *)(data + 0x28) != 0) {
            int i = *(short *)(moby + 0xB2);
            if (*(unsigned char *)(D_L15_001BBB40_c + i + 0x454) != 0) {
                f18 = 1;
            } else if ((*(int *)(D_0014171B + 0xAB75 + ((i >> 5) << 2) + (D_0015EE84 << 8)) >> (i & 0x1F)) & 1) {
                f18 = 1;
            }
        }
        r = func_00215570(D_0013E633 + 0xE9D, *(int *)(data + 0x10));
        if (r != 0 || f18) moby[0x20] = 2;
        break;
    }
    case 2: {
        int st = *(int *)(data + 0x20);
        char *entry = D_0013E633 + 0x1D + st * 0x70;
        float f0, f3;
        if (!(*(char **)(entry + 0x88) == moby && entry[0x74] != 0)) {
            *(int *)(data + 0x20) = func_0022ED80(0, 4, (int)moby);
        }
        f3 = *(float *)(data + 0x24);
        f0 = *(float *)(data + 0x18) + f3 / 0.6f * D_0015EE6C;
        *(float *)(data + 0x18) = f0;
        if (f3 < f0) {
            int s = *(int *)(data + 0x20);
            if (s != -1) {
                char *en = D_0013E633 + 0x1D + s * 0x70;
                if (*(char **)(en + 0x88) == moby && en[0x74] != 0) func_L00_0028EBF0(s);
            }
            *(float *)(data + 0x18) = *(float *)(data + 0x24);
            *(int *)(data + 0x20) = -1;
            if (*(int *)(data + 0x1C) == -1 && *(int *)(data + 0x28) == 0) {
                moby[0x20] = 0;
            } else {
                int i5 = *(short *)(moby + 0xB2) >> 5;
                int b2 = *(unsigned short *)(moby + 0xB2);
                *(int *)(D_0014171B + 0xAB75 + (i5 << 2) + (D_0015EE84 << 8)) |= 1 << (b2 & 0x1F);
                *(int *)((char *)D_L15_001BADE0 + (i5 << 2)) |= 1 << (b2 & 0x1F);
                moby[0x20] = 5;
            }
        }
        {
            float v[4];
            float out[4];
            float m48 = *(float *)(moby + 0x48);
            v[0] = func_001F9F90(m48) * -*(float *)(data + 0x18);
            v[1] = func_001F9FA8(m48) * -*(float *)(data + 0x18);
            *(int *)&v[2] = 0;
            func_001F9BD8(out, data, v);
            qcopy(moby + 0x10, out);
        }
        break;
    }
    case 3:
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(data + 0x14)) != 0) break;
        if (func_00215570(D_L15_00167440, *(int *)(data + 0x14)) != 0) break;
        moby[0x20] = 4;
        break;
    case 4: {
        int st = *(int *)(data + 0x20);
        char *entry = D_0013E633 + 0x1D + st * 0x70;
        float f0, f1;
        if (!(*(char **)(entry + 0x88) == moby && entry[0x74] != 0)) {
            *(int *)(data + 0x20) = func_0022ED80(0, 4, (int)moby);
        }
        f1 = *(float *)(data + 0x24) / 0.6f * D_0015EE6C;
        f0 = *(float *)(data + 0x18) - f1;
        *(float *)(data + 0x18) = f0;
        if (f0 < 0.0f) {
            int s = *(int *)(data + 0x20);
            if (s != -1) {
                char *en = D_0013E633 + 0x1D + s * 0x70;
                if (*(char **)(en + 0x88) == moby && en[0x74] != 0) func_L00_0028EBF0(s);
            }
            *(float *)(data + 0x18) = 0.0f;
            *(int *)(data + 0x20) = -1;
            moby[0x20] = 1;
        }
        {
            float v[4];
            float out[4];
            float m48 = *(float *)(moby + 0x48);
            v[0] = func_001F9F90(m48) * -*(float *)(data + 0x18);
            v[1] = func_001F9FA8(m48) * -*(float *)(data + 0x18);
            *(int *)&v[2] = 0;
            func_001F9BD8(out, data, v);
            qcopy(moby + 0x10, out);
        }
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(data + 0x10)) != 0 ||
            func_00215570(D_L15_00167440, *(int *)(data + 0x10)) != 0) {
            moby[0x20] = 2;
        }
        break;
    }
    }
}
