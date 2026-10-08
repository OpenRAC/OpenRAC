/* NON_MATCHING func_L14_002B5750 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 364 / retail 368, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L14_002B5750(idx): walks a -1-terminated moby-id list (D_L14_001AC2C0[idx]); returns 0 if any member is n
 *   Remaining diff (9 instrs): loop temporaries swapped v0/v1 (sll $v1 in loop 1, lhu $v1 in loop 2) and in the ta
 *   q28/t06: p13.c (364 vs retail 368; the earlier best.c did not compile: `short *D_L14_001AC2C0[]` clashes with 
 *   Left: (1) the second loop's `D_L14_00160098_m` MACRO_ADDR load comes out as `$gp` (lw -0x6C68($gp)) instead of
 */
extern void func_00213D28(void *, int, int);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern short *D_L14_001AC2C0_s[] __asm__("D_L14_001AC2C0");
extern short D_L14_00160098;
extern char *D_L14_00160098_m __asm__("D_L14_00160098") MACRO_ADDR;
extern short D_L14_001615D4;
extern short D_L14_001615D8;

// If every moby in list idx is idle or done, start them all (set state 5, timing data) and return 1.
int func_L14_002B5750(int idx) {
    short *p = D_L14_001AC2C0_s[idx];
    char *base = *(char **)&D_L14_00160098;
    if (p == 0)
        return 0;
    do {
        char *m = base + ((*p & 0x7FFF) << 8);
        if (*(unsigned char *)(*(char **)(m + 0x78) + 0x8B) == 0) {
            if ((unsigned char)m[0x20] != 5 && (unsigned char)m[0x20] != 1)
                return 0;
        }
    } while (*p++ >= 0);
    p = D_L14_001AC2C0_s[idx];
    do {
        char *m = D_L14_00160098_m + ((*p & 0x7FFF) << 8);
        if ((unsigned char)m[0x20] == 3) {
            char *data;
            int v;
            data = *(char **)(m + 0x78);
            func_00213D28(m, 4, 0);
            m[0x20] = 5;
            v = func_001F9850(*(int *)&D_L14_001615D4);
            *(short *)(data + 0x74) = v;
            *(float *)(data + 0x78) = 1.0f / func_001FA888((short)v);
            *(float *)(m + 0x58) = 1.0f / ((float)*(short *)(data + 0x74) * *(float *)&D_L14_001615D8);
            *(float *)(data + 0x8C) = *(float *)(m + 0x48);
            *(short *)(data + 0x88) = 0;
            *(char *)(data + 0x8B) = 0;
        }
    } while (*p++ >= 0);
    return 1;
}
