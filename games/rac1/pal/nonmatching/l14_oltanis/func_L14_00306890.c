/* NON_MATCHING func_L14_00306890 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: BYTES 19/504 (96.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1352 (3-state switch, flag bitsets indexed by (short)moby[0xB2]). Best p1.c: 19 instruction diffs, 
 *   (retail does `addu $v0,$v0,$v1` base first; ours index first, and D_0015EE84<<8 vs (idx>>5)<<2 land in swapped
 */
extern char D_0014171B[];
extern unsigned char D_L14_001BBCC0[];
extern int D_L14_001BAF60[];
extern int D_0015EE84 MACRO_ADDR;
extern unsigned char *D_L14_00160098 MACRO_ADDR;
extern void func_0020D678(void *);
extern void func_L14_00306A88(void *);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern void func_L10_002F6E10(int);
extern void func_001F9908(int *arg0);
extern void func_L14_00306B08(void *);

/* UpdateMoby_1352: three-state object tracking a bitset of collected flags */
void func_L14_00306890(unsigned char *moby) {
    int *data = *(int **)(moby + 0x78);
    unsigned short u;
    short s;
    switch (moby[0x20]) {
    case 0:
        if (data[3] < 0 || data[0] < 0 || data[4] < 0) {
            func_0020D678(moby);
            return;
        }
        u = *(unsigned short *)(moby + 0xB2);
        s = u;
        if (D_L14_001BBCC0[0x454 - (-s)] != 0) {
            moby[0x20] = 3;
            return;
        }
        if ((((int *)(D_0014171B + 0xAB75 + D_0015EE84 * 256))[s >> 5] >> (u & 0x1F)) & 1) {
            moby[0x20] = 3;
            return;
        }
        moby[0x20] = 1;
        func_L14_00306A88(moby);
        break;
    case 1:
        if (D_L14_00160098[data[3] * 256 + 0x20] == 2) {
            u = *(unsigned short *)(moby + 0xB2);
            s = u;
            ((int *)(D_0014171B + 0xAB75 + D_0015EE84 * 256))[s >> 5] |= 1 << (u & 0x1F);
            u = *(unsigned short *)(moby + 0xB2);
            s = u;
            D_L14_001BAF60[s >> 5] |= 1 << (u & 0x1F);
            moby[0x20] = 2;
            data[1] = func_001F9850(0x78);
            *(float *)(data + 2) = 1.0f / func_001FA888(data[1]);
            func_L10_002F6E10(data[4]);
        }
        break;
    case 2:
        func_001F9908(data + 1);
        func_L14_00306B08(moby);
        if (data[1] == 0) moby[0x20] = 3;
        break;
    }
}
