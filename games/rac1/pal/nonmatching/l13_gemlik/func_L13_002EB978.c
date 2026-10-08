/* NON_MATCHING func_L13_002EB978 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: BYTES 63/372 (83.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Builds a 2-vector aim for `other`, calls func_L13_002C2638 (spawn) with func_001F9850(0x12C) inside the argume
 *   Best p6.c: all logic and register use match (3 saved regs, tbl and b addresses held across the nested call) ex
 *   Needs a way to get the gp form and the lui form for the same symbol in one function; unknown.
 */
#include "common.h"
extern int func_001160D8(void);
extern void func_L00_00250800(void *, int, void *);
extern int func_001F9850(int);
extern int func_L13_002C2638(void *, void *, int, void *, int, float, float);
extern char D_0013E633[];
extern short D_0015EE6C_s __asm__("D_0015EE6C");
extern float D_0015EE6C MACRO_ADDR;

/* build the aim vectors for the other moby and spawn through func_L13_002C2638 */
int func_L13_002EB978(char *moby, char *other) {
    float a[4];
    float b[4];
    float k;
    int id;
    char *tbl;
    float *bp;
    int r = 0x10;
    int res;

    if (other[0xFC] & 1) r = 0x12;
    if ((unsigned char)moby[0xBC] != 6 || (func_001160D8() & 1)) {
        other[0xFC] = other[0xFC] ^ 1;
    }
    func_L00_00250800(moby, r, a);
    b[0] = 0.0f;
    if ((unsigned char)moby[0x20] != 5) {
        func_L13_002EB838(a, &b[1], &b[2], D_0015EE6C * 17.0f);
        k = *(float *)&D_0015EE6C_s;
    } else {
        func_L13_002EB838(a, &b[1], &b[2], -1.0f);
        k = D_0015EE6C;
    }
    k = k * 18.0f;
    b[2] = *(float *)(moby + 0x48);
    b[1] = 0.4363323f;
    k = k + *(float *)(other + 0xAC);
    tbl = D_0013E633 + 0xE1D;
    bp = b;
    id = func_001F9850(0x12C);
    res = func_L13_002C2638(moby, a, *(int *)(tbl + 0x15F0), bp, id, k, D_0015EE6C * 18.0f);
    if (res != 0) {
        func_0022ED80(2, 0, (int)moby);
    }
    return res;
}
