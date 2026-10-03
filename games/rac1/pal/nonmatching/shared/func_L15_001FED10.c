/* NON_MATCHING func_L15_001FED10 -- src/overlays/shared/variants.c
 * Best so far: SIZE ours 152 / retail 156, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Checks the byte at D_0013E633+0xE1D+0x20A4 (2 or 3 -> 0), then flag word +0x22A8==1 -> 0x54, arg==0 / bytes +0
 *   Logic is right; size is 152 vs retail 156 (best wording: separate `if (s==2)` / `if (s==3)`). Retail keeps the
 *   Unblock: some form that makes the base address be rematerialised from its hi part after the state compares (al
 */
extern char D_0013E633[];
extern char D_L15_0017A140[];
extern int func_L00_0020DB30(int);

// Returns a value from the table entry for the current slot when the gameplay flags allow it, 0x54 in one mode, else 0.
int func_L15_001FED10(int arg)
{
    char *b = D_0013E633 + 0xE1D;
    int s = *(unsigned char *)(b + 0x20A4);
    if (s == 2) {
        return 0;
    }
    if (s == 3) {
        return 0;
    }
    b = D_0013E633 + 0xE1D;
    if (*(int *)(b + 0x22A8) == 1) {
        return 0x54;
    }
    if (arg == 0) {
        return 0;
    }
    if (*(unsigned char *)(b + 0x20A8) == 0) {
        return 0;
    }
    if (*(unsigned char *)(b + 0x20AA) == 0) {
        return 0;
    }
    if (*(short *)(b + 0x22C8) != 0) {
        return 0;
    }
    return *(int *)(D_L15_0017A140 + func_L00_0020DB30(0) * 0x4C + 0x24);
}
