/* NON_MATCHING func_L13_0030D2A8 -- src/overlays/l13_gemlik/vendor_0030CAE0.c
 * Best so far: SIZE ours 108 / retail 112, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern int D_L13_0015F6A8 MACRO_ADDR;
extern int D_L13_0016CBE0 MACRO_ADDR;
extern void func_L00_00264870(int);

void func_L13_0030D2A8(unsigned char *moby)
{
    switch (moby[0x20]) {
    case 0:
        moby[0x30] = 0xFF;
        moby[0x20] = 1;
        break;
    case 1:
        if (D_L13_0015F6A8 == 2 && *(int *)((char *)D_L13_0016CBE0 + 0x30) == 2) {
            func_L00_00264870(*(int *)((char *)D_L13_0016CBE0 + 0x180));
        }
        break;
    }

}
