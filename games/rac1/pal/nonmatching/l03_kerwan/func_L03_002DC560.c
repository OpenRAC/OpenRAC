/* NON_MATCHING func_L03_002DC560 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: BYTES 9/228 (96.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L03_002DC560: spawns class 0x382 moby, initialises fields 0x20/0x30/0x31/0x32/0x34, copies a path's first
 *   Only diff (9 bytes): constant regs (0x80/0x40/1 land in a0/v1/a1 in ours vs v1/a1/a2 retail) and store order s
 *   Unblock: a source ordering that makes the constants live across the arg setup of func_L00_0025E210.
 */
#include "common.h"
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L00_0025E210(void *);
extern void func_001F9BC0(float *);
extern float func_L00_001FF860(float, float);
extern void func_L00_00251E30(void *);

// Spawns a path-following moby (class 0x382) at the path's start and aims it at its end.
char *func_L03_002DC560(char *path, float speed) {
    unsigned char *m = (unsigned char *)func_0020D348_m(0x382);
    if (m) {
        char *d;
        m[0x20] = 0;
        m[0x30] = 0x80;
        d = *(char **)(m + 0x78);
        m[0x31] = 1;
        *(short *)(m + 0x32) = 0x40;
        *(unsigned short *)(m + 0x34) |= 0x20;
        func_L00_0025E210(m);
        qcopy(m + 0x10, path + 0x10);
        func_001F9BC0((float *)(m + 0x40));
        {
            char *last = path + ((*(int *)path - 1) << 4);
            *(float *)(m + 0x48) = func_L00_001FF860(*(float *)(last + 0x10) - *(float *)(m + 0x10), *(float *)(last + 0x14) - *(float *)(m + 0x14));
        }
        *(float *)(d + 0x64) = speed;
        *(char **)(d + 0x60) = path;
        *(char **)(d + 0x8) = d + 0x20;
        func_L00_00251E30(m);
    }
    return (char *)m;
}
