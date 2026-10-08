/* NON_MATCHING func_L11_0030A748 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 388 / retail 384, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a moby (type 0x40A) from a source moby: sets state bytes, copies pos/vel with qcopy, flags the target's
 *   p0.c is one instruction off (392 vs 384 bytes): ours keeps moby+0x10 in its own saved register ($s6); retail c
 *   Tried a local, reassigning pos after either qcopy, int cast: the compiler always propagates the copy. Would ne
 *   u04 (Lombyte port, p5-p7): US C has the same control flow; p5/p6 (`{ char *s = pos; pos = moby + 0x10; qcopy(p
 */
#include "common.h"
extern char *func_0020D348(int);
extern char *func_L00_0025D390(void *);
extern void func_00215C00(void *, float, float, float);
extern void func_00213DE0(void *, int, int, int);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00251E30(void *);
/* Spawns a projectile moby from a source moby: copies position and velocity, tags the target, scales its speed. Adapted from Lombyte (MIT) for PAL: src/overlays/l11/unclassified_002cb668.c, FUN_L11_00309378. */
char *func_L11_0030A748(char *src, char *pos, char *target, char *vel, int t0, float f) {
    char *moby = func_0020D348(0x40A);
    char *data;
    if (moby) {
        data = *(char **)(moby + 0x78);
        moby[0x30] = 0xFF;
        *(short *)(moby + 0x32) = 0xFF;
        moby[0x31] = 1;
        moby[0x20] = 0;
        *(char **)(data + 0x20) = src;
        *(char **)(data + 0x24) = target;
        { char *s = pos; pos = moby + 0x10; qcopy(pos, s); }
        qcopy(moby + 0x40, vel);
        if (target) {
            char *t = func_L00_0025D390(target);
            if (t) *(unsigned short *)(t + 0x1E) |= 0x80;
        }
        *(int *)(data + 0x28) = t0;
        *(float *)(data + 0x2C) = f;
        *(int *)(data + 0x30) = 0;
        *(int *)(data + 0x34) = 0;
        func_00215C00(data, f, *(float *)(vel + 8), -*(float *)(vel + 4));
        if (*(unsigned char *)(moby + 0x53) != 1) func_00213DE0(moby, 1, 0, 10);
        func_001F9BD8(pos, pos, data);
        if (*(char **)(data + 0x24)) qcopy(data + 0x10, *(char **)(data + 0x24) + 0x10);
        *(float *)(moby + 0x2C) = *(float *)(*(char **)(moby + 0x24) + 0x24) *
            (*(float *)(src + 0x2C) / *(float *)(*(char **)(src + 0x24) + 0x24));
        func_L00_00251E30(moby);
    }
    return moby;
}
