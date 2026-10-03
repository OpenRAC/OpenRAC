/* NON_MATCHING func_L01_0030E4C8 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 260 / retail 268, checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   BreakableCrateVariantUpdate: switch on state (0 -> 1; 1 -> 2 once the found object's float at +0x2C is > 0; 2 
 *   p2..p6 all give the same bytes (28/268 differ): the call setup is scheduled differently (retail does addiu/%lo
 *   Would unblock: a scheduler tie; likely the real prototype of func_L00_00265050 (unknown) changes the arg-setup
 */
extern int func_0022ED80(int, int, int);
extern void func_L01_00279790(void *);
extern void func_L01_00279E10(void *, int);

// Breakable crate variant update: state 0 starts, 1 waits for a positive value, 2 breaks into pieces.
void func_L01_0030E4C8(char *moby) {
    char *r = func_L00_0025B478(moby, 0x10000, 0);
    int hit = 0;
    switch (*(unsigned char *)(moby + 0x20)) {
    case 0:
        moby[0x20] = 1;
        break;
    case 1:
        if (r != 0 && *(float *)(r + 0x2C) > 0.0f) hit = 1;
        if (hit != 0) moby[0x20] = 2;
        break;
    case 2:
        func_0022ED80(0, 0, (int)moby);
        func_L01_00279790(moby);
        func_L00_00265050(moby, 0x717, moby + 0x10, moby + 0x40, 0, 0, 0.0f, D_L01_0015F660, D_L01_0015F660, D_L01_0015F660);
        func_L01_00279E10(moby, 0x718);
        func_0020D678(moby);
        break;
    }
}
