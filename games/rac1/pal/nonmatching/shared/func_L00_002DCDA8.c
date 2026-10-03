/* NON_MATCHING func_L00_002DCDA8 -- src/overlays/shared/vendor_002D9438.c
 * Best so far: BYTES 24/548 (95.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002DCDA8: moby animation state machine; looks up its record (func_L00_002DCD40), bails on dead/NULL, 
 *   Best is p3.c (size 548 = retail, BYTES 24/548): block layout, jump table and all branches agree; only the sche
 *   Retail orders `lq; sq` first (using $v0) then the state-index chain (addiu/sll/sra/sltiu in $v1); ours hoists 
 *   Unblock: a wording that keeps the lq/sq ahead of the switch-index computation (maybe the state is read from a 
 */
typedef int V16 __attribute__((mode(TI)));
extern char *func_L00_002DCD40(char *);
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);
extern float func_001F9D48(void *, void *);
extern int func_001E9730();
extern char D_L00_001EA3B0[];

/* Steps a moby's animation state machine (state at +0x68 of its record): picks the animation of the next state and returns the state to be in; 0 when there is no record or the moby is dead. */
int func_L00_002DCDA8(char *m, char *q, char *a2) {
    char *r = func_L00_002DCD40(m);
    char *p;

    if (r != 0) {
        if (m != 0 && *(unsigned char *)(m + 0x20) != 0xFE && *(unsigned char *)(m + 0x20) != 0xFD &&
            *(short *)(r + 0x68) != 8) {
            *(V16 *)r = *(V16 *)q;
            *(char **)(r + 0x60) = a2;
            *(float *)(r + 0xC) = 1.0f;
            switch (*(short *)(r + 0x68)) {
            case 1:
                if (*(float *)(r + 0x6C) >= 1.0f) {
                    p = *(char **)(r + 0x70);
                    if (*(unsigned char *)(m + 0x53) != *(unsigned char *)(p + 2)) {
                        func_00213DE0(m, *(unsigned char *)(p + 2), 0, func_001F9850(10));
                    }
                    *(int *)(r + 0x6C) = 0;
                    *(short *)(r + 0x68) = 2;
                    return 1;
                } else {
                    p = *(char **)(r + 0x70);
                    if (*(unsigned char *)(m + 0x53) != *(unsigned char *)(p + 1)) {
                        func_00213DE0(m, *(unsigned char *)(p + 1), 0, func_001F9850(10));
                    }
                    return 1;
                }
            case 2:
                if (*(float *)(r + 0x6C) >= 1.0f) {
                    return 2;
                }
                break;
            case 3:
                return 2;
            case 4:
                return 3;
            case 5:
            case 6:
            case 7:
                p = *(char **)(r + 0x70);
                if (*(unsigned char *)(m + 0x53) != *(unsigned char *)(p + 3)) {
                    func_00213DE0(m, *(unsigned char *)(p + 3), 0, func_001F9850(10));
                }
                *(short *)(r + 0x68) = 3;
                *(int *)(r + 0x6C) = 0;
                *(float *)(m + 0x58) = 1.0f;
                *(float *)(r + 0x64) = func_001F9D48(m + 0x10, a2 + 0x10);
                return 2;
            default:
                p = *(char **)(r + 0x70);
                if (*(unsigned char *)(m + 0x53) != *(unsigned char *)(p + 1)) {
                    func_00213DE0(m, *(unsigned char *)(p + 1), 0, func_001F9850(10));
                }
                *(int *)(r + 0x6C) = 0;
                *(short *)(r + 0x68) = 1;
                break;
            }
            return 1;
        }
    } else {
        func_001E9730(D_L00_001EA3B0);
    }
    return 0;
}
