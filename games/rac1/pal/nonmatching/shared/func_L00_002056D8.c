/* NON_MATCHING func_L00_002056D8 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 2/76 (97.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   HeroTakeDamage(dmg): if (dmg) { amt = dmg >= 2 ? 1 : dmg; hp -= amt (clamped at 0) at D_0013E633+0xE1D+0x22A8;
 *   Best is 2 instructions off (p3-p6, all same bytes): retail does slti into the dmg copy register ($5) and movn 
 *   Unblock: a source form that keeps the parameter in $a0 separate from the copied value; not found.
 */
extern unsigned char D_0013E633[] NOT_SDA;
extern void func_L00_00235790(void);

/* hero takes damage: subtract from the health counter, clamp at zero, refresh the HUD */
void func_L00_002056D8(int dmg) {
    if (dmg != 0) {
        char *g = (char *)D_0013E633 + 0xE1D;
        int amt = dmg >= 2 ? 1 : dmg;
        *(int *)(g + 0x22A8) -= amt;
        if (*(int *)(g + 0x22A8) < 0) *(int *)(g + 0x22A8) = 0;
        func_L00_00235790();
    }
}
