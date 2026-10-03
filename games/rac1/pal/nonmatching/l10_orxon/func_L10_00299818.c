/* NON_MATCHING func_L10_00299818 -- src/overlays/l10_orxon/vendor_00296BD8.c
 * Best so far: BYTES 7/504 (98.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_18: spins moby, 4-state script (cases 0-3). p2.c matches except 7 instructions in case 2: retail se
 */
extern float func_001FA748(float, float);
extern void func_L00_002D80A0(char *);
extern void func_0020D678(void *);
extern void func_L10_00299AF0(char *);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern void func_L00_00299B68(int);
extern void func_L00_00264DB8(int, int);
extern void func_L00_002618D8(int, int);
extern void func_L00_00286128(void *, void *);
extern int func_0020BFC8(int, int);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L10_0015F6A8 MACRO_ADDR;
extern int D_L10_0015F720 MACRO_ADDR;
extern short D_L10_00160058_s __asm__("D_L10_00160058");
extern unsigned char D_0013D605[];
extern char D_0013E633[] NOT_SDA;

/* Per-frame update of a moby: spins it and runs its four-state script. */
void func_L10_00299818(char *moby) {
    char *d = *(char **)(moby + 0x78);
    *(float *)(moby + 0x48) = func_001FA748(*(float *)(moby + 0x48), D_0015EE6C * 1.5707964f);
    switch (((unsigned char *)moby)[0x20]) {
    case 0:
        func_L00_002D80A0(moby);
        if (D_0013D605[7] != 0) {
            func_0020D678(moby);
        } else {
            *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + 0.5f;
            moby[0x20] = 1;
            *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) * 0.667f;
        }
        break;
    case 1:
        func_L10_00299AF0(moby);
        d = D_0013E633 + 0xE9D;
        if (func_001F9D48(moby + 0x10, d) < 5.0f) {
            d -= 0x80;
            if (func_001F9B88(*(float *)(moby + 0x18) - *(float *)(d + 0x88)) < 2.0f) {
                if (*(int *)(d + 0x22A8) != 0) {
                    *(unsigned short *)(moby + 0x34) |= 0x41;
                    func_L00_00299B68(2);
                    moby[0x20] = 2;
                }
            }
        }
        break;
    case 2:
        if (D_L10_0015F6A8 != 2) {
            func_L00_00264DB8(0x271B, -1);
            D_L10_0015F720 = 0xB4;
            func_L00_002618D8(0x1C, 1);
            {
            int idx = *(int *)(d + 4);
            if (idx != -1) {
                char *base = *(char **)&D_L10_00160058_s;
                func_L00_00286128(base + idx * 256 + 0x10, base + idx * 256 + 0x40);
            }
            }
            func_0020BFC8(0, -1);
            moby[0x20] = 3;
        }
        break;
    case 3:
        func_0020D678(moby);
        break;
    }
}
