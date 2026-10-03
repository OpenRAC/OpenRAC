/* NON_MATCHING func_L00_002C82E8 -- src/overlays/shared/vendor_002C12B0.c
 * Best so far: BYTES 7/228 (96.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby 0xD1, links parent/float into its data, copies 3 qwords of transform, finalizes. Differences: reta
 */
extern char *func_0020D348(int);
extern void func_L00_00251328(void *, int, int, int);
extern void func_L00_00250800(void *, int, void *);
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);
extern void func_L00_00251E30(void *);
typedef int u128 __attribute__((mode(TI)));

// Spawns moby 0xD1 linked to a parent, copies its transform and finalizes it.
unsigned char *func_L00_002C82E8(char *parent, float f) {
    unsigned char *m = (unsigned char *)func_0020D348(0xD1);
    if (m != 0) {
        char *d = *(char **)(m + 0x78);
        *(float *)(d + 4) = f;
        *(char **)d = parent;
        m[0x31] = 1;
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        func_L00_00251328(m, 0x80, 0x80, 0x80);
        func_L00_00250800(parent, 0, m + 0x10);
        *(u128 *)(m + 0xC0) = *(u128 *)(parent + 0xC0);
        *(u128 *)(m + 0xD0) = *(u128 *)(parent + 0xD0);
        *(u128 *)(m + 0xE0) = *(u128 *)(parent + 0xE0);
        if (m[0x53] != 0) {
            func_00213DE0(m, 0, 0, func_001F9850(10));
        }
        func_L00_00251E30(m);
        m[0x20] = 0;
        *(unsigned short *)(m + 0x34) |= 4;
    }
    return m;
}
