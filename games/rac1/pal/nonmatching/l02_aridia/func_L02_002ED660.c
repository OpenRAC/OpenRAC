/* NON_MATCHING func_L02_002ED660 -- src/overlays/l02_aridia/vendor_002E21F8.c
 * Best so far: BYTES 8/220 (96.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a class-0x4B8 moby (init via func_L00_00251328), copies pos/src flags, sets angles from vec and stores 
 *   Best p3 (int ff = 0xFF shared by the sb/sh stores, source store order 0x31,0x32,0x30): 8 of 220 bytes differ, 
 *   Store order permutations and chained assignment tried; none moves the a0 copy. Unblock: find the wording that 
 */
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L00_00251328(void *, int, int, int);
extern float func_L00_001FF860(float, float);
extern float func_001F9CE8(void *);
extern int func_001F9850(int);
extern short D_L02_00161FDC;
extern short D_L02_00161FE0;
extern short D_L02_00161FE4;

// Spawn a moby of class 0x4B8 at pos, aimed along vec, carrying vec and a random timer.
unsigned char *func_L02_002ED660(char *src, float *pos, float *vec) {
    unsigned char *m = (unsigned char *)func_0020D348_m(0x4B8);
    char *data;
    int ff = 0xFF;
    if (m) {
        m[0x31] = 1;
        *(short *)(m + 0x32) = ff;
        m[0x30] = ff;
        func_L00_00251328(m, *(int *)&D_L02_00161FDC, *(int *)&D_L02_00161FE0, *(int *)&D_L02_00161FE4);
        *(unsigned short *)(m + 0x34) = *(unsigned short *)(src + 0x34);
        qcopy(m + 0x10, pos);
        *(float *)(m + 0x48) = func_L00_001FF860(vec[0], vec[1]);
        *(float *)(m + 0x44) = func_L00_001FF860(func_001F9CE8(vec), vec[2]);
        m[0x20] = 1;
        data = *(char **)(m + 0x78);
        qcopy(data, vec);
        *(int *)(data + 0x10) = func_001F9850(0x3C);
    }
    return m;
}
