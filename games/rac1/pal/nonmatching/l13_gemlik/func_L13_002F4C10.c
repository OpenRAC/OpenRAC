/* NON_MATCHING func_L13_002F4C10 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: BYTES 4/472 (99.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_558: if a linked moby is gone/in a state, bumps counters in a stats struct, else calls func_0021557
 *   p4.c is 4 bytes off: retail stores 0xFF (we emit -1: use ((unsigned char *)moby)[0x30] = 0xFF), and retail use
 *   p5.c has both fixes but the budget was spent before it could be run; try p5.c first.
 */
extern int func_001F9850(int);
extern int func_00215570(void *, int);
extern int func_L00_00203F20(int a, int b);
extern int D_L13_00179790[] NOT_SDA;
extern int D_L13_0015F6A8 MACRO_ADDR;
extern int D_0015EFA4 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern char *D_L13_00160058_m __asm__("D_L13_00160058") MACRO_ADDR;
extern char D_0013E633[] NOT_SDA;
extern unsigned char D_0013D50F[] NOT_SDA;
extern unsigned char D_0014171B[] NOT_SDA;

/* Per-frame update: awards a stat/flag when its linked moby is gone, then sets a flag byte from a test. */
void func_L13_002F4C10(char *moby) {
    char *data;
    moby[0x30] = 0xFF;
    data = *(char **)(moby + 0x78);
    if (D_L13_00179790[0] == 0 && D_L13_00179790[9] == -1) {
        unsigned int v = *(unsigned int *)(D_0013E633 + 0x2EA9);
        if ((v < 2 || v == 9) ? D_L13_0015F6A8 == 0 : 0) {
            int idx = *(int *)(data + 4);
            char *m;
            if (idx != -1) {
                m = D_L13_00160058_m + (idx << 8);
                if (m != 0 && *(short *)(m + 0xA6) == 0xAA) {
                    int s = (unsigned char)m[0x20];
                    if (s == 0xFE) goto join;
                    if (s != 0xFD) goto direct;
                }
            }
        join:
            if (D_0013D50F[0xB9 + 0xB] != 0 || D_0013D50F[0xB9 + 0xD] != 0) {
                unsigned char *q = D_0014171B + 0x34D;
                unsigned int h = *(unsigned short *)(q + 0x378);
                if (h <= 0xFFFE) {
                    *(unsigned short *)(q + 0x378) = h + 1;
                }
                if (func_001F9850(D_0015EFA4) / 600 > *(unsigned short *)(q + 0x37A)) {
                    *(unsigned short *)(q + 0x37A) = func_001F9850(D_0015EFA4) / 600;
                }
                *(unsigned int *)(q + 0x37C) = *(unsigned int *)(q + 0x37C) | (1 << D_0015EE84) | 0x80000000;
            } else {
            direct:
                if (func_00215570(D_0013E633 + 0xE9D, *(int *)data) != 0) {
                    if (*(int *)(D_0014171B + 0x6C9) >= 0) {
                        func_L00_00203F20(0x32C8, 0x6F);
                    }
                }
            }
        }
    }
    if (func_00215570(D_0013E633 + 0xE9D, *(int *)(data + 8)) != 0) {
        (D_0013E633 + 0xE9D)[0x224B] = 0;
    } else {
        (D_0013E633 + 0xE9D)[0x224B] = 1;
    }
}
