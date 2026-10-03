/* NON_MATCHING func_L00_002346C0 -- src/overlays/shared/help_00232560.c
 * Best so far: BYTES 7/80 (91.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   AddAmmo(weapon, n): ammo[weapon] += n (int table at D_0013D50F+0x21), then clamps to the ushort max at D_L00_0
 *   Body (address math, lhu, subu) is exact; only the tail differs (4 words): retail has `blez -> external return;
 *   Tried nested if, early returns, return inside the inner if (p0-p3): identical bytes. Looks like a shared retur
 *   Also tried returning the overflow (`return over;` only on the clamp path, p4): same bytes. Same wall as func_L
 */
extern unsigned char D_0013D50F NOT_SDA;
extern char D_L00_001C43B0[];
// Adds ammo for a weapon, clamped to its maximum when it has one; returns the overflow when it clamped.
int func_L00_002346C0(int w, int n) {
    int *p = (int *)(&D_0013D50F + 0x21) + w;
    char *t = D_L00_001C43B0 + w * 0x18;
    int v = *p + n;
    *p = v;
    if (*(unsigned short *)(t + 0xE) != 0) {
        int m = *(unsigned short *)(t + 0xE);
        int over = v - m;
        if (over > 0) {
            *p = m;
            return over;
        }
    }
}
