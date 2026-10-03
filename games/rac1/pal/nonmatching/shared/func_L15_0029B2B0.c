/* NON_MATCHING func_L15_0029B2B0 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: BYTES 6/376 (98.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawn-gate/reset helper: if a global word == 0x65 clear d+0x156; if moby[0x21]==0xFF set d+0x156 = func_001F98
 *   Best p8.c: 6 diff words, all in one place: retail stores `sb $zero,0x15B` before `daddu $a0,moby` and `sh $v0,
 */
#include "common.h"
extern int func_001F9850(int);
extern void func_L15_0029B428(char *, char *);
extern char D_0013E633[];
extern int D_L15_00160058 MACRO_ADDR;

/* after the spawn gate, find the class 0x2C moby in the moby's list and reset its state */
void func_L15_0029B2B0(char *moby) {
    float a[4];
    float b[4];
    char *d = *(char **)(moby + 0x78);
    unsigned short *list;
    int idx;
    short r;
    char *m;

    if (*(int *)(D_0013E633 + 0x2EA1) == 0x65) {
        *(short *)(d + 0x156) = 0;
        return;
    }
    if ((unsigned char)moby[0x21] == 0xFF) {
        *(short *)(d + 0x156) = func_001F9850(0xB4);
        return;
    }
    list = D_L15_001AC140[(unsigned char)moby[0x21]];
    if (list == 0) return;
    qcopy(a, moby + 0x10);
    a[2] = a[2] + 1.0f;
    do {
        idx = (*list & 0x7FFF) << 8;
        m = (char *)(idx + D_L15_00160058);
        if (m[0x20] >= 0 && *(short *)(m + 0xA6) == 0x2C) {
            qcopy(b, m + 0x10);
            b[2] = b[2] + 1.0f;
            if (func_L00_001EFFF0(b, a, 2, 0, 0) == 0) {
                d = *(char **)(idx + D_L15_00160058 + 0x78);
                r = func_001F9850(0x12C);
                d[0x15B] = 0;
                *(short *)(d + 0x156) = r;
                func_L15_0029B428(moby, (char *)(D_L15_00160058 + idx));
            }
        }
    } while (*(short *)list++ >= 0);
}
