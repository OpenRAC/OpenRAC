/* NON_MATCHING func_L02_002211C0 -- src/overlays/shared/help_0021A2E0.c
 * Best so far: SIZE ours 184 / retail 180, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L02_002211C0: calls func_L00_00211F80(0,1.0) then updates a float field (p+0x190, p = D_0013E633+0xE1D): 
 *   Logic, branch shapes and delay slots match (stores b then overwrites with a). Remaining 20 bytes: register all
 *   Wordings p4 p5 p6 p7 (local v/direct reads, array vs int symbol, early state load) all give the same bytes; lo
 */
extern void func_L00_00211F80(int, float);
extern float D_0015EE6C MACRO_ADDR;
extern int D_0013E633;
extern float D_L02_0017C438[];

/* Updates a speed field: scales it while in state 0x3F, else picks a table value when positive. */
void func_L02_002211C0(void)
{
    char *p;
    func_L00_00211F80(0, 1.0f);
    p = (char *)&D_0013E633 + 0xE1D;
    if (*(int *)(p + 0x2084) == 0x3F) {
        float a = D_0015EE6C + D_0015EE6C;
        float b = D_0015EE6C * 3.5f * *(float *)(p + 0x190);
        *(float *)(p + 0x190) = b;
        if (b < a) *(float *)(p + 0x190) = a;
    } else if (0.0f < *(float *)(p + 0x190)) {
        float r = D_L02_0017C438[3];
        if (*(float *)(p + 0x190) < r) r = D_L02_0017C438[2]; else r = D_L02_0017C438[6];
        *(float *)(p + 0x190) = r * D_0015EE6C;
    }
}
