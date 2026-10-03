/* NON_MATCHING func_L05_00263490 -- src/overlays/shared/hud_00263490.c
 * Best so far: BYTES 13/108 (88.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hud init: sets moby+0x7C = func_001F9850(0xB4)+0x1E, zeroes shorts at 0x48/0x4A, clears the four words ending 
 *   Only difference (13 bytes, 4 words at 0x28-0x34): retail schedules `sh 0x4A; addiu $v1,3; lui; addiu` and ours
 *   Would unblock: a wording that keeps the loop-setup lui/addiu behind the last sh; not found in the budget.
 */
extern int func_001F9850(int);
extern void func_L00_00236610(char *);
extern int D_L05_0015FBB4[];

// Sets a timer and two fields, clears a four-word table to -1, then calls the hud setup.
void func_L05_00263490(char *moby)
{
    int i;
    *(int *)(moby + 0x7C) = func_001F9850(0xB4) + 0x1E;
    *(short *)(moby + 0x48) = 0;
    *(short *)(moby + 0x4A) = 0;
    for (i = 3; i >= 0; i--) {
        D_L05_0015FBB4[i - 3] = -1;
    }
    func_L00_00236610(moby);
}
