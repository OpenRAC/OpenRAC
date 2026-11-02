/* Near miss: rechecked 28 October 2026 at 380/380 bytes, 0 of 28 USA v1.01 places.
 *
 * Profile: -O2 -G8 -ffunction-sections (the body counts three small-data
 * words through $gp).
 * Last verdict: `python scratch/wslcmp.py src/game/002E7018-g8.c func_002EBC70`
 *   func_002EBC70: 0 of 28 places match   [boot: instruction at +0x34 differs]
 *
 * Remaining difference: only the loop's local allocation.  Retail keeps
 * previous->$v1, i->$a1 and the array cursor->$a0 around base->$a2; this cc1
 * swaps previous and i and puts the cursor in $a1.  The entry walk, the whole
 * call sequence, the two gp stores and the return value are identical.
 * A 28 Oct pointer-induction probe (cached count plus `char **cursor`) grew the
 * candidate to 364/380 bytes, 0/28; it reloaded the count in the loop and did
 * not recover retail's allocation. The indexed form remains the best source.
 */
/* One frame of the level transition loop: it re-links the queued entries to
   their predecessor, runs the frame's call sequence, and repeats while the
   shared mode word says "running".  The three small-data words it counts live
   in the program's own small-data area, so the unit is built with -G8. */
extern int D_001B1B5C;
extern char D_001B8840[];
extern int D_001B8BB0;
extern int D_001B8BB8;
extern int D_001B169C __attribute__((sda));
extern int D_001B1594 __attribute__((sda));
extern int D_001B1598 __attribute__((sda));
extern int D_001A8BB0 __attribute__((sda));

extern void func_002EB7E8(void);
extern void func_002FC280(void);
extern void func_002FC220(void);
extern void func_00284FC0(void);
extern void func_00285338(void);
extern void func_00284EA8(void);
extern void func_00298F00(void);
extern void func_00279D18(void);
extern void func_0029FB20(void);
extern void func_0027B858(void);
extern void func_002B9000(void);
extern int func_002EB1E0(void);
extern void func_00284F40(int);
extern void func_0027BD70(void);
extern void func_002FC3C0(int);
extern void func_001261F0(int);
extern void func_002EBAA8(void);

int func_002EBC70(int unused)
{
    char *prev;
    int i;
    int value;

    func_002EB7E8();
    prev = (char *)D_001B1B5C;
    for (i = 0; i < *(short *)(D_001B8840 + 0x44); i++) {
        char *entry = ((char **)(D_001B8840 + 0x18C))[i];

        if (entry != 0) {
            if (entry == prev)
                continue;
            *(int *)(entry + 0x28) = (int)prev;
            prev = entry;
        }
    }
    D_001B8BB0 = 2;
    do {
        func_002FC280();
        func_002FC220();
        func_00284FC0();
        func_00285338();
        func_00284EA8();
        func_00298F00();
        func_00279D18();
        func_0029FB20();
        func_0027B858();
        func_002B9000();
        D_001B169C = -1;
        value = func_002EB1E0();
        func_00284F40(0);
        func_0027BD70();
        D_001B1594 = 0x131;
        if (D_001B8BB8 != -2) {
            D_001B8BB0 = D_001B8BB8;
            D_001B8BB8 = -2;
        }
        func_002FC3C0(1);
        func_001261F0(0);
        D_001B1598 = D_001B1598 + 1;
    } while (D_001B8BB0 == 2);
    func_002EBAA8();
    D_001A8BB0 = 0;
    return value;
}

