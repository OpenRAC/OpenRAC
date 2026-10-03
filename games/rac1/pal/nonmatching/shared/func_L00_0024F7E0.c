/* NON_MATCHING func_L00_0024F7E0 -- src/overlays/shared/missionfunc_0024EEF0.c
 * Best so far: SIZE ours 136 / retail 140, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Walks the 0x100-byte slot table [D_L00_0016009C, D_L00_001600A0): if (p[0x20] >= 0xFE && !((u64)(u32)D_L00_001
 *   Best p4 (SIZE 136 vs 140; structure, end-pointer copy, movz/branch layout all as retail). Remaining: retail to
 *   Unblock: the source form that gives one symbol both gp and lui accesses without alias-splitting the load (like
 */
extern unsigned char *D_L00_0016009C;
extern unsigned char *D_L00_001600A0;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern short D_L00_0016007C;

/* walks the 0x100-byte slot table counting entries that qualify */
void func_L00_0024F7E0(void) {
    unsigned char *p;
    int found = 0;

    *(int *)&D_L00_0016007C = 0;
    for (p = D_L00_0016009C; p < D_L00_001600A0; p += 0x100) {
        if ((p[0x20] >= 0xFE && !((unsigned long)(unsigned int)D_L00_0015F6B0 < *(unsigned long *)(p + 0x38))) || found) {
            *(int *)&D_L00_0016007C = *(int *)&D_L00_0016007C + 1;
            if (p[0x20] == 0xFF) found = 1;
        }
    }
}
