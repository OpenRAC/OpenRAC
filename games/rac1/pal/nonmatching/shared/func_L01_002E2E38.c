/* NON_MATCHING func_L01_002E2E38 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 524 / retail 520, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ItemOfferUpdate: state switch (0 wait/check, 1 animate+take cost, 2 delete if taken, 4 memcard call). Best p4.
 *   Remaining: case 0 branch polarity/shared store of state 3 (b8..e0), and register assignment in case 1 (flag po
 */
extern void func_0020D678(void *);
extern int func_L00_002676E8(void *, void *);
extern int func_L00_00267290(void *, void *);
extern void func_L01_002AFD00(void *);
extern int func_0020BFC8(int, int);
extern int D_L01_001DEDE0[];
extern char D_L01_001C47B0[];
extern unsigned char D_0013E15A[];
extern int D_0015EE98 MACRO_ADDR;
extern int D_L01_0015F6A8 MACRO_ADDR;
extern short D_0015EF20;

// Item offer moby update: waits for the offer, then plays out its states.
void func_L01_002E2E38(char *m) {
    char *d = *(char **)(m + 0x78);
    if (d != 0 && *(int *)(d + 0x40) != -1 && ((*(int *)(d + 0x40) ^ 1) & 1)) {
        if (*(int *)&D_0015EF20 == 0) {
            func_0020D678(m);
            return;
        }
    }
    switch ((unsigned char)m[0x20]) {
    case 0:
        if (*(short *)(m + 0xA6) != 0x130) {
            m[0x20] = 2;
        } else if (D_0013E15A[0x4C6 + D_L01_001DEDE0[*(int *)(d + 0x40)]] == 0) {
            if (func_L00_002676E8(m, d) == -1) {
                m[0x20] = 3;
            } else {
                m[0x20] = 1;
            }
        } else {
            m[0x20] = 3;
        }
        break;
    case 1:
        *(float *)(d + 0xC) = 1.9f;
        func_L00_00267290(m, d);
        if (*(short *)(d + 4) == 2) {
            D_0013E15A[0x4C6 + D_L01_001DEDE0[*(int *)(d + 0x40)]] = 1;
            m[0x20] = 4;
            D_0015EE98 = D_0015EE98 - *(unsigned short *)(D_L01_001C47B0 + D_L01_001DEDE0[*(int *)(d + 0x40)] * 0x18 + 0x14);
            func_L01_002AFD00(m);
        }
        break;
    case 2:
        if (D_0013E15A[0x4C6 + D_L01_001DEDE0[*(int *)(d + 0x40)]] != 0) {
            func_0020D678(m);
        }
        break;
    case 3:
        break;
    case 4:
        if (D_L01_0015F6A8 == 0) {
            func_0020BFC8(0, -1);
            m[0x20] = 3;
        }
        break;
    }
}
