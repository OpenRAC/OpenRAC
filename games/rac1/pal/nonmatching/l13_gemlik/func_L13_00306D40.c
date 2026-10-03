/* NON_MATCHING func_L13_00306D40 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: BYTES 7/224 (96.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby 0x4D1 from a parent (copies transform quadwords, inits data). Best p5.c: 7 bytes differ, only the 
 */
#include "common.h"
typedef int u128_306D40 __attribute__((mode(TI)));
extern char *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_00251E30(void *);

// Spawns moby 0x4D1 copying position/transform from the parent and initialising its data.
char *func_L13_00306D40(char *parent, short arg) {
    char *m = func_0020D348_m(0x4D1);
    char *d;
    char *pos;
    if (m != 0) {
        d = *(char **)(m + 0x78);
        m[0x20] = 1;
        pos = m + 0x10;
        m[0x31] = 1;
        *(unsigned char *)(m + 0x30) = 0xFF;
        *(short *)(m + 0x32) = 0x7F;
        *(char **)(d + 0x20) = parent;
        *(short *)(d + 0x26) = arg;
        *(short *)(d + 0x24) = 1;
        func_L00_00250800(parent, arg, pos);
        qcopy(m + 0x40, parent + 0x40);
        *(u128_306D40 *)(m + 0xC0) = *(u128_306D40 *)(parent + 0xC0);
        *(u128_306D40 *)(m + 0xD0) = *(u128_306D40 *)(parent + 0xD0);
        *(u128_306D40 *)(m + 0xE0) = *(u128_306D40 *)(parent + 0xE0);
        d += 0x10; qcopy(d, pos);
        *(int *)(m + 0x94) = 0;
        func_L00_00251E30(m);
    }
    return m;
}
