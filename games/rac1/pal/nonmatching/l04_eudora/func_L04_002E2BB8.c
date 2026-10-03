/* NON_MATCHING func_L04_002E2BB8 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: BYTES 31/564 (94.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby (collectable): state 0 stores y, state 1 waits for pickup (bitset D_0014C290[level][i>>5], D_L04_00
 *   Best p4.c/p1.c (BYTES 31/564): only register allocation differs: retail addu $v0,$v0,$v1 (base first) for the 
 */
extern int func_L00_0028EF68(int i, int a1, int v, int k);
extern int func_001F9850(int);
extern float func_L04_002E2B48(char *);
extern int func_001F9908_i(int *arg0) __asm__("func_001F9908");
extern void func_0022ED80(int, int, int);
extern int D_0015EE84 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_0014C290[][64] NOT_SDA;
extern unsigned char D_L04_001BB6C0[];
extern int D_L04_001BA960[];
extern short D_L04_00160058;

/* updates a collectable moby: arms, waits for pickup, then marks it taken */
void func_L04_002E2BB8(char *m) {
    char *d = *(char **)(m + 0x78);
    int i;
    unsigned short u;
    switch ((unsigned char)m[0x20]) {
    case 0:
        *(float *)(d + 4) = *(float *)(m + 0x18);
        m[0x20] = 1;
        break;
    case 1:
        u = *(unsigned short *)(m + 0xB2);
        if (D_L04_001BB6C0[(short)u + 0x454] == 0 && ((D_0014C290[D_0015EE84][(short)u >> 5] >> (u & 0x1F)) & 1) == 0) {
            int o = *(int *)d;
            char *q;
            if (o == -1) break;
            q = *(char **)&D_L04_00160058 + o * 256;
            if (!((*(short *)(q + 0xA6) == 0x267 && (unsigned char)q[0x20] == 4) ||
                  (*(short *)(q + 0xA6) == 0x4A6 && (unsigned char)q[0x20] == 2)))
                break;
        }
        if (*(short *)(m + 0xA6) == 0x44D || *(short *)(m + 0xA6) == 0x5FC)
            func_L00_0028EF68(0, 0, (int)m, 0x44D);
        m[0x20] = 2;
        *(int *)(d + 8) = func_001F9850(0x3C);
        break;
    case 2:
        *(float *)(m + 0x18) = *(float *)(m + 0x18) + func_L04_002E2B48(m) * D_0015EE6C;
        if (func_001F9908_i((int *)(d + 8)) != 0) {
            u = *(unsigned short *)(m + 0xB2);
            D_0014C290[D_0015EE84][(short)u >> 5] |= 1 << (u & 0x1F);
            u = *(unsigned short *)(m + 0xB2);
            D_L04_001BA960[(short)u >> 5] |= 1 << (u & 0x1F);
            if (*(short *)(m + 0xA6) == 0x44D) func_0022ED80(1, 0, (int)m);
            m[0x20] = 3;
        }
        break;
    }
}
