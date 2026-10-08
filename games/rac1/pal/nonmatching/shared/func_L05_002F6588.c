/* NON_MATCHING func_L05_002F6588 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: BYTES 11/496 (97.8% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L05_002F6588 (496 B)
 *   Best: c2.c BYTES 11/496. Left: (1) the two source addresses build lui $v1/addiu $v1,$v1 in retail but
 *   lui $v0/addiu $v1,$v0 in ours (inline ternary same; MACRO_ADDR versions hoist both, worse);
 *   (2) the BC/state test: retail's `beqz BC / bnel on` threading of ((on && BC) || (!on && !BC)) && m[0x20].
 *   Other spellings (e1-e3) are worse (73+ B). The second list base d+0x50 is a local set before loop 1.
 */
#include "common.h"

extern char D_0013E633[];
extern char D_L05_001672C0[];
extern char *D_L05_001B0CB0[];
extern float func_001F9D10(void *, void *);
extern int func_L00_0025A778(void *, void *, int);
extern int func_00215570(void *, int);

/* Switch moby: on while the hero (or the camera target) is near or inside one of its zones; on a change it flips its state and drives its linked lists. */
void func_L05_002F6588(char *m) {
    int *d = *(int **)(m + 0x78);
    int on = 0;
    float pos[4];
    char *src;
    int i;
    int *l2;
    ((unsigned char *)m)[0x30] = 0xFF;
    if (((signed char *)d)[0x7E] != 0) {
        src = D_L05_001672C0;
    } else {
        src = D_0013E633 + 0xE9D;
    }
    qcopy(pos, src);
    if (((float *)d)[1] > 0.0f) {
        if (func_001F9D10(m + 0x10, pos) < ((float *)d)[1]) on = 1;
    }
    if (on == 0 && d[0] != -1) {
        char *z = D_L05_001B0CB0[d[0]];
        on = func_L00_0025A778(pos, z + 0x10, *(int *)z) != 0;
    }
    if (on == 0) {
        for (i = 0; i < 6; i++) {
            if (d[2 + i] == -1) continue;
            if (func_00215570(pos, d[2 + i]) == 0) continue;
            on = 1;
            break;
        }
    }
    if (((on && ((unsigned char *)m)[0xBC]) || (!on && !((unsigned char *)m)[0xBC])) && ((unsigned char *)m)[0x20] != 0) return;
    if (on != 0) {
        m[0xBC] = 1;
    } else {
        m[0xBC] = 0;
    }
    l2 = d + 0x14;
    m[0x20] = ((unsigned char *)m)[0xBC] + 1;
    for (i = 0; i < 12; i++) {
        if (d[8 + i] != -1) func_L05_002F6518((int)m, (int)d, d[8 + i], on);
    }
    for (i = 0; i < 11; i++) {
        if (l2[i] != -1) func_L05_002F62E8(l2[i], (int)d, on);
    }
}
