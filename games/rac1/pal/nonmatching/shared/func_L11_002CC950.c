/* NON_MATCHING func_L11_002CC950 -- src/overlays/shared/vendor_002C99E0.c
 * Best so far: BYTES 9/128 (93.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern float func_001F9D10(void *, void *);
extern int func_00116248(char *, const char *, ...);
extern int func_001F9850(int);
extern void func_0020D678(void *);
extern char D_0013E633[];
extern char D_0013D355[];
extern char D_L11_00179B98[];
extern char D_L11_0021ABF0[];
extern int D_L11_0015F720;

void func_L11_002CC950(char *moby)
{
    if (func_001F9D10(moby + 0x10, D_0013E633 + 0xE9D) < 2.0f) {
        D_0013D355[0x13C] = 1;
        func_00116248(D_L11_00179B98, D_L11_0021ABF0);
        func_0020D678((D_L11_0015F720 = func_001F9850(0xB4), moby));
    }
}
