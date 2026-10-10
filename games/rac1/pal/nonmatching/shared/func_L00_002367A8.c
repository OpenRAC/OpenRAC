/* NON_MATCHING func_L00_002367A8 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 124 / retail 128, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the HUD bank (D_L00_0017DD50, stride 0x90) whose +0x64 equals id; if it exists and +0x68 == 0, sets +0x7
 *   Everything matches (loop, mult, movz, move $2,$0) except the tail: retail ends `jr $31` with the final `sw` in
 *   Tried void, `return 0` in the if block, early-return forms (p0-p2): gcc always merges the returns. Same shape 
 */
// Raises a HUD bank's counter to at least val, for the bank with the given id when it is idle.
void func_L00_002367A8(int id, int val) {
    int i;
    for (i = 0; i < 13; i++) {
        if (D_L00_0017DD50[i].unk64 == id) break;
    }
    if (i < 13 && D_L00_0017DD50[i].unk68 == 0) {
        if (D_L00_0017DD50[i].unk7C < val) D_L00_0017DD50[i].unk7C = val;
    }
}
