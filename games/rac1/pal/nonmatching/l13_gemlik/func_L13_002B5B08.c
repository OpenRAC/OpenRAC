/* NON_MATCHING func_L13_002B5B08 -- src/overlays/l13_gemlik/vendor_002B2020.c
 * Best so far: BYTES 14/252 (94.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a class 0x24 child moby that copies its parent's class byte, position (qcopy) and scaled size, then cal
 *   Best p4.c: BYTES 14/252. Remaining difference is constant-1 allocation: retail keeps two separate 1 constants 
 *   Would need a source form that stops CSE of the two 1s (unknown); qcopy is volatile asm so its position is fixe
 */
extern char *func_0020D348(int);
extern void func_L00_00251E30(void *);
extern void func_L00_0025E210(void *);

/* Spawns a class 0x24 child moby that copies its parent's placement and scale. */
char *func_L13_002B5B08(char *parent) {
    char *m = func_0020D348(0x24);
    if (m) {
        char *d = *(char **)(m + 0x78);
        *(char **)(d + 0x70) = parent;
        *(char **)d = d + 0x20;
        *(char **)(d + 0xC) = d + 0x60;
        m[0x30] = parent[0x30];
        *(unsigned short *)(m + 0x32) = *(unsigned short *)(*(char **)(d + 0x70) + 0x32);
        m[0x31] = 1;
        *(unsigned short *)(m + 0x34) |= 0x20;
        m[0x20] = 0;
        m[0xBC] = 0;
        qcopy(m + 0x10, *(char **)(d + 0x70) + 0x10);
        *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * *(float *)(parent + 0x2C) / *(float *)(*(char **)(parent + 0x24) + 0x24);
        *(float *)(d + 0x20) = 1.0f;
        d[0x29] = 1;
        d[0x2A] = 3;
        *(short *)(d + 0x24) = 1;
        d[0x28] = 0;
        *(float *)(d + 0x30) = 0.7f;
        func_L00_00251E30(m);
        func_L00_0025E210(m);
    }
    return m;
}
