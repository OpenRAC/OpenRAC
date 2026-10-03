/* NON_MATCHING func_L00_00299148 -- src/overlays/shared/tieproc_00299108.c
 * Best so far: SIZE ours 256 / retail 260, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   IncrementTickCounter: bumps a still-pad tick counter (gp short D_L00_001610B0) when the pad fields are all zer
 *   Best p3.c: 18 of 260 bytes differ, only register allocation/schedule of the three global loads after the merge
 *   Note: MACRO_ADDR accesses in a branch delay slot turn into gp-relative by tools/check_macro_slots.py, so decla
 */
extern int func_001F9850(int);
extern unsigned char D_0013A5E0[];
extern int D_L00_0015F6B0 MACRO_ADDR;
extern short D_0015EFA4_g __asm__("D_0015EFA4");
extern int D_0015EFA4 MACRO_ADDR;
extern int D_L00_0015F678;
extern short D_0015EF28;
extern int D_0015EF24[2] MACRO_ADDR;
extern short D_L00_001610B0;

/* advance the frame counters and the still-pad tick counter */
void func_L00_00299148(void) {
    float *pad = (float *)(D_0013A5E0 + 0x2460);
    if (*(int *)((char *)pad + 0x1A0) != 0 || pad[0x108 / 4] != 0.0f || pad[0x10C / 4] != 0.0f
        || pad[0x100 / 4] != 0.0f || pad[0x104 / 4] != 0.0f) {
        *(int *)&D_L00_001610B0 = 0;
    } else {
        *(int *)&D_L00_001610B0 = *(int *)&D_L00_001610B0 + 1;
    }
    D_L00_0015F6B0 = D_L00_0015F6B0 + 1;
    *(int *)&D_0015EFA4_g = D_0015EFA4 + 1;
    if (D_L00_0015F678 != 0) {
        if (*(int *)&D_L00_001610B0 < (int)((float)func_001F9850(15) * 60.0f)) {
            D_0015EF24[1] = *(int *)&D_0015EF28 + 1;
        }
    }
}
