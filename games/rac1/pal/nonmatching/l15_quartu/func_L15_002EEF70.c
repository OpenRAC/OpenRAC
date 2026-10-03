/* NON_MATCHING func_L15_002EEF70 -- src/overlays/l15_quartu/vendor_002EDB50.c
 * Best so far: SIZE ours 184 / retail 188, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L15_002EEF70: UpdateMoby_1560; state 0 -> set state 1 and byte 0x30 = 0xFF; state 1 -> when D_L15_0015F6A
 *   A switch on the state byte reproduces the control flow (p2, 180 bytes vs 188). Only the last arm differs: reta
 *   Every wording of "i = st; if (s != 6) i = 0" and ternaries folds the same way; would need a source form where 
 */
typedef struct {
    char pad0[0x30];
    int state;
    char pad34[0x10];
    short count;
    char pad46[0x132];
    void *mobys[1];
} Level15State;

extern void func_L00_00264870(void *);
extern int D_L15_0015F6A8;
extern Level15State D_L15_0016CEE0;

// Moby update: starts in state 1, then reacts to the level event phase.
void func_L15_002EEF70(char *moby) {
    int st = (unsigned char)moby[0x20];
    switch (st) {
    case 0:
        moby[0x20] = 1;
        moby[0x30] = 0xFF;
        break;
    case 1:
        if (D_L15_0015F6A8 == 2) {
            int s = D_L15_0016CEE0.state;
            if ((unsigned)(s - 1) < 4 || s == 6) {
                int i;
                if (s == st) {
                    i = 4;
                } else if ((unsigned)(s - 2) < 3) {
                    i = 2;
                } else {
                    i = (s != 6) ? 0 : st;
                }
                func_L00_00264870(D_L15_0016CEE0.mobys[i]);
            }
        }
        break;
    }
}
