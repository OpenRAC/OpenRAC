/* NON_MATCHING func_L09_002F1198 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: SIZE ours 496 / retail 500, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Spawns moby 0x1A1 with random heading/speed, velocity from two vectors, scale 0x2C by (pi-h)*2+1.
 *   Only difference left (p1/p3, 496 vs 500 bytes): ours CSEs/hoists (pi-h) to before the second func_L00_001FF4B0
 */
extern char *func_0020D348(int);
extern float func_002140F8(float, float);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_00251E30(void *);
extern float D_0015EE6C MACRO_ADDR;

/* Spawns a debris moby with a random heading, speed and velocity derived from the given vectors. */
char *func_L09_002F1198(int unused, char *a, char *b, float lo, float hi) {
    char *moby = func_0020D348(0x1A1);
    if (moby != 0) {
        char *pos = moby + 0x10;
        char *d = *(char **)(moby + 0x78);
        float pi = 3.14159274f;
        float ang = func_002140F8(-3.14159274f, pi);
        float spd = func_002140F8(lo, hi);
        float h = func_001FA850(func_L00_001FF860(((float *)b)[0], ((float *)b)[1]), ang);
        float t[4];
        *(float *)(moby + 0x10) = func_001F9F90(ang) * spd;
        *(float *)(moby + 0x14) = func_001F9FA8(ang) * spd;
        *(int *)(moby + 0x18) = 0;
        func_L00_001FF4B0(d, pos, h * D_0015EE6C);
        func_L00_001FF4B0(t, b, (pi - h) * (pi - h) * 0.25f * D_0015EE6C);
        func_001F9BD8(d, d, t);
        *(int *)(d + 8) = 0;
        func_001F9BD8(pos, pos, a);
        func_001F9BD8(pos, pos, b);
        func_001F9C30(d, d, 0.35f);
        *(float *)(moby + 0x58) = *(float *)(moby + 0x58) * 0.5f;
        *(float *)(moby + 0x48) = func_L00_001FF860(((float *)d)[0], ((float *)d)[1]);
        ((unsigned char *)moby)[0x30] = 0xFF;
        *(short *)(moby + 0x32) = 0xFF;
        moby[0x31] = 1;
        h = pi - h;
        *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) * (h + h + 1.0f);
        func_L00_00251E30(moby);
    }
    return moby;
}
