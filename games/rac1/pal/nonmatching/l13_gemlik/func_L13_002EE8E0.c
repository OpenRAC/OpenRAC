/* NON_MATCHING func_L13_002EE8E0 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: BYTES 6/588 (99.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_404: state machine (0 init, 1 wait for flag, 2 fade out, 3 delete), then a spin of the field at +0x
 *   Best is p4.c (6 bytes differ, one instruction): retail loads the halfword into $v0 and puts the new value (+0x
 *   Unblock: a form of the wrap-around tail that splits the old and new value into two registers (budget spent on 
 */
extern void func_L00_00251328(void *, int, int, int);
extern int func_001E9730();
extern void func_L13_002EE590(void *, void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_0023F1D0(int);
extern void func_0020D678(void *);
extern void func_L00_0024FFE8(unsigned char *, int, int);
extern unsigned char D_0014171B[] NOT_SDA;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern char D_L13_001F52A0[];
extern float D_0015EE60 MACRO_ADDR;
extern char *D_L13_00160058_m __asm__("D_L13_00160058") MACRO_ADDR;

/* Updates a gem-lock pickup: waits for its flag, fades out, then deletes itself; always spins. */
void func_L13_002EE8E0(char *moby) {
    char *d = *(char **)(moby + 0x78);
    int v;
    int dx;
    switch (((unsigned char *)moby)[0x20]) {
    case 0:
        moby[0x20] = 1;
        moby[0x23] = 0x14;
        *(unsigned short *)(moby + 0x34) |= 0xA08;
        *(short *)(d + 0xE) = -1;
        *(float *)(d + 4) = 1.0f;
        func_L00_00251328(moby, 0x80, 0x80, 0x80);
        break;
    case 1:
        if (*(int *)d != -1) {
            char *p = D_L13_00160058_m + (*(int *)d << 8);
            if (p == 0 || (unsigned char)p[0x20] == 0xFE || (unsigned char)p[0x20] == 0xFD
                || (*(int *)(D_0014171B + 0xAB75 + (((short)*(unsigned short *)(p + 0xB2) >> 5) * 4 + (D_0015EE84_m << 8))) >> (*(unsigned short *)(p + 0xB2) & 0x1F)) & 1) {
                *(float *)(d + 4) = 1.0f;
                moby[0x20] = 2;
            }
        } else {
            *(float *)(d + 4) = 1.0f;
            moby[0x20] = 2;
            func_001E9730(D_L13_001F52A0, *(short *)(moby + 0xA6), *(short *)(moby + 0xB2));
        }
        func_L13_002EE590(moby, d);
        break;
    case 2:
        *(float *)(d + 4) = *(float *)(d + 4) - D_0015EE60 * 0.05f;
        if (*(float *)(d + 4) < 0.0f) {
            moby[0x20] = 3;
        } else {
            moby[0x23] = func_001FA898_r(*(float *)(d + 4) * 20.0f);
        }
        break;
    case 3:
        if (*(short *)(d + 0xE) != -1) {
            func_L00_0023F1D0(*(short *)(d + 0xE));
            *(short *)(d + 0xE) = -1;
        }
        func_0020D678(moby);
        return;
    }
    v = *(unsigned short *)(d + 0xC);
    dx = 0xC0;
    v += 0xC0;
    *(short *)(d + 0xC) = v;
    if ((short)v > 0x1000) {
        *(short *)(d + 0xC) = v - 0x1000;
        dx = -0xF40;
    } else if ((short)v < 0) {
        *(short *)(d + 0xC) = v + 0x1000;
        dx = 0x10C0;
    }
    func_L00_0024FFE8(*(unsigned char **)(moby + 0x24), dx, 0);
}
