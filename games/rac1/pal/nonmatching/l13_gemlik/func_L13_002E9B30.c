/* NON_MATCHING func_L13_002E9B30 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 544 / retail 548, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L13_002E9B30: picks a slot from a table (idx + rand%n) not yet set in obj+0x13C, claims it, writes global
 *   Control flow decoded and right; best p2.c/p4.c (locals decl order k,v,i gives matching regs s1=i,s2=v,s3=n,s4=
 *   Unblock: a way to make g a per-use lui+addiu with the hi shared (sdata/MACRO_ADDR variants tried, no luck). Bu
 *   w11 (p6-p9): set path moved after the loop as if (idx<0x1D){loop} else-tail (retail's block order), g dropped 
 */
#include "common.h"
extern int func_001160D8(void);
extern int func_002140B0(int);
extern char D_0014171B[] MACRO_ADDR;
extern unsigned char D_L13_001D3828[];

// Picks the next free slot from a shuffled table and claims it, returning 1 on success.
int func_L13_002E9B30(int unused, char *obj, int idx, int n) {
    int k;
    int v;
    int i;
    char *g;
    i = func_001160D8() % n;
    v = D_L13_001D3828[idx + i];
    if (idx < 0x1D) {
        if (idx < 0xC) {
            if (((unsigned char *)obj)[0x148] != 0) return 0;
            if (*(short *)(obj + 0x144) > 0) return 0;
        } else {
            if (*(short *)(obj + 0x146) > 0) return 0;
            if (((unsigned char *)obj)[0x148] != 0) {
                if (idx >= 0x17) {
                    if (idx < 0x1B) return 0;
                }
            }
        }
    }
    g = D_0014171B + 0x100B5;
    if (*(int *)(g + 0x50) != 0 || *(int *)(g + 0x1C) != -1) {
        if (idx < 0x1D) return 0;
    }
    if (idx < 0x1D) {
        for (k = 0; k < n; k++) {
            if ((*(int *)(obj + 0x13C) >> v) & 1) {
                if (idx < 0xC && func_002140B0(4) == 0) {
                    *(int *)(obj + 0x13C) &= ~(1 << v);
                }
                i = (i + 1) % n;
                v = D_L13_001D3828[idx + i];
            } else {
                *(int *)(D_0014171B + 0x100D1) = v + 0xC350;
                obj[0x13A] = 1;
                *(int *)(obj + 0x13C) |= 1 << v;
                return 1;
            }
        }
        if (idx < 0xC) {
            *(int *)(obj + 0x13C) &= ~(1 << v);
        }
        return 0;
    }
    *(int *)(D_0014171B + 0x100D1) = v + 0xC350;
    obj[0x13A] = 1;
    return 1;
}
