/* NON_MATCHING func_L01_002BA150 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 552 / retail 556, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a DMA packet (matrix via func_001FA1C0/9C30/FA540, HUD floats from D_L01_0016CF40) at D_L01_00161240, t
 *   p4 is near: 552 vs 556 bytes. Left: the 0xFFFFFFFF constant is built with lui/ori in retail (we emit addiu -1)
 *   Wants another spelling of the -1 stores (maybe unsigned/long constant) and of the end-pointer local.
 */
typedef int u128_2BA150 __attribute__((mode(TI)));
extern void func_001FA1C0(float *, float);
extern void func_001F9C30(void *, void *, float);
extern void func_00234B48(void *, int);
extern void func_001FA540(void *, void *, void *);
extern void func_001FB848(void);
extern char D_L01_001672C0[];
extern int D_L01_0015F6C4 MACRO_ADDR;
extern unsigned short D_0010E800 NOT_SDA;
extern char D_0010E810[];
extern char *D_L01_00161240_p __asm__("D_L01_00161240") MACRO_ADDR;
extern char D_L01_0016CF40[];
extern char D_L01_001CAF00[];
extern short D_L01_001612B4;
extern short D_L01_001612A8;
extern short D_L01_001612AC;
extern short D_L01_001612B0;

// Builds a full-screen sprite draw packet into the DMA buffer from a transform and the HUD data.
void func_L01_002BA150(void)
{
    float mat[16];
    char *p;
    char *b;
    char *end;
    int sev = 7;

    func_001FA1C0(mat, 1024.0f);
    func_001F9C30(mat + 12, D_L01_001672C0, -1024.0f);
    mat[15] = 1.0f;
    if (D_L01_0015F6C4 != sev) {
        func_00234B48(D_0010E810, D_0010E800);
        D_L01_0015F6C4 = sev;
    }
    *(int *)(D_L01_00161240_p + 0) = 0x10000000;
    *(int *)(D_L01_00161240_p + 4) = 0;
    *(int *)(D_L01_00161240_p + 8) = 0x11000000;
    *(int *)(D_L01_00161240_p + 0xC) = 0x1000404;
    p = D_L01_00161240_p;
    *(int *)(p + 0x10) = 0;
    *(int *)(p + 0x14) = 0;
    *(int *)(p + 0x18) = 0;
    *(int *)(p + 0x1C) = 0x6C0C43A4;
    func_001FA540(p + 0x20, D_L01_001672C0 - 0x100, mat);
    func_001FA540(p + 0x60, D_L01_001672C0 - 0x80, mat);
    b = D_L01_0016CF40;
    end = p + 0xF0;
    *(int *)(p + 0xA8) = 0x412;
    *(int *)(p + 0xA0) = 0x8000;
    *(int *)(p + 0xA4) = 0x303EC000;
    *(float *)(p + 0xAC) = *(float *)(b + 0x210);
    qcopy(p + 0xB0, b + 0x190);
    qcopy(p + 0xC0, b + 0x1A0);
    *(float *)(p + 0xD0) = *(float *)(b + 0x22C);
    *(int *)(p + 0xE4) = 0x20001D2;
    *(float *)(p + 0xD4) = *(float *)(b + 0x228);
    *(int *)(p + 0xE8) = 0x15000000;
    *(int *)(p + 0xE0) = 0x3000000;
    *(int *)(p + 0xD8) = 0;
    *(int *)(p + 0xDC) = 0;
    *(int *)(p + 0xEC) = 0;
    *(int *)D_L01_00161240_p |= ((end - D_L01_00161240_p) >> 4) - 1;
    D_L01_00161240_p = end;
    func_001FB848();
    *(int *)(D_L01_00161240_p + 0) = 0x30000007;
    *(int *)(D_L01_00161240_p + 4) = (int)D_L01_001CAF00;
    *(int *)(D_L01_00161240_p + 8) = 0;
    *(int *)(D_L01_00161240_p + 0xC) = 0x50000007;
    *(int *)&D_L01_001612B4 = 0xFFFFFFFF;
    *(int *)&D_L01_001612A8 = 0xFFFFFFFF;
    *(int *)&D_L01_001612AC = 0xFFFFFFFF;
    D_L01_00161240_p = D_L01_00161240_p + 0x10;
    *(int *)&D_L01_001612B0 = 0xFFFFFFFF;
}
