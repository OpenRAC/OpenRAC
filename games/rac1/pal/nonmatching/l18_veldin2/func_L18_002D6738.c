/* NON_MATCHING func_L18_002D6738 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 10/320 (96.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1
 *   Allocates a 0x234 moby, inits header bytes, 0xC0 call, scales f2c by gp float (read as *(float*)&short D), cop
 *   Best p3.c (16 differing bytes of 320, 10 runs used). Remaining: s4/s5 allocation swap (c vs e params) and stor
 */
extern char *func_0020D348(int);
extern void func_L00_00251328(void *, int, int, int);
extern float func_L00_001FF860(float, float);
extern float func_001F9CE8(void *);
extern short D_L18_001619F0;

/* spawns a moby at pos with a direction and speed */
char *func_L18_002D6738(float *pos, float *b, float *c, int d, int e, float f1, float f2) {
    unsigned char *m = (unsigned char *)func_0020D348(0x234);
    char *p;
    if (m) {
        m[0x20] = 0;
        m[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        m[0x31] = 1;
        func_L00_00251328(m, 0xC0, 0xC0, 0xC0);
        *(float *)(m + 0x2C) = *(float *)(m + 0x2C) * *(float *)&D_L18_001619F0;
        qcopy(m + 0x10, pos);
        p = *(char **)(m + 0x78);
        qcopy(p, b);
        qcopy(p + 0x10, c);
        *(int *)(p + 0x30) = d >> 1;
        *(float *)(p + 0x24) = f1;
        *(float *)(p + 0x28) = f2;
        *(int *)(p + 0x34) = d;
        *(float *)(p + 0x2C) = 0.0f;
        *(int *)(p + 0x38) = e;
        *(float *)(m + 0x48) = func_L00_001FF860(b[0], b[1]);
        *(float *)(m + 0x44) = -func_L00_001FF860(func_001F9CE8(b), b[2]);
        func_0022ED80(0, 0, (int)m);
    }
    return (char *)m;
}
