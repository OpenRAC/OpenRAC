/* NON_MATCHING func_L01_002C8740 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 556 / retail 552, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_156: aims a moby at the player (func_001F9D48 / func_L00_001FF860), then state 1 spawns effects/del
 *   Best p5.c (BYTES 365/552, all in the shifted tail): frame is 0xA0 vs retail 0xB0 (retail has an extra 0x10 of 
 */
extern float func_001F9D48(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern char *func_L01_00281738(int a, char *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_0025A8C0_x(void *, void *, int, void *, float) __asm__("func_L00_0025A8C0");
extern int func_L00_001F2BE8_x(void *, int, int, void *, float) __asm__("func_L00_001F2BE8");
extern int func_001E97C8(void *arg0);
extern void func_L00_0025AC00_x(int, float, void *, int, void *, void *) __asm__("func_L00_0025AC00");
extern void func_L01_002C84D8(void);
extern int func_001F9908(int *arg0);
extern int func_001F9850(int);
extern void func_0020D678(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char D_L01_001672C0[];
extern float D_L01_00174340[];
extern short D_L01_00161524;
extern short D_L01_0016152C;

/* updates a moby that flies toward the player and bursts */
void func_L01_002C8740(char *m) {
    float a[4];
    float b[4];
    char *d = *(char **)(m + 0x78);
    char *g;
    float dist;
    float *e = D_L01_00174340;
    float r;
    dist = func_001F9D48(D_L01_001672C0, m + 0x10);
    g = D_L01_001672C0 - 0x140;
    r = func_L00_001FF860(dist, *(float *)(g + 0x148) - *(float *)(m + 0x18));
    *(float *)(m + 0x44) = -r;
    *(float *)(m + 0x48) = func_L00_001FF860(*(float *)(g + 0x140) - *(float *)(m + 0x10), *(float *)(g + 0x144) - *(float *)(m + 0x14));
    *(float *)(m + 0x40) = func_001FA748(*(float *)(m + 0x40), *(float *)(d + 0xC));
    func_001F9BD8(m + 0x10, m + 0x10, d + 0x20);
    if ((unsigned char)m[0x20] == 1) {
        func_L01_00281738(*(int *)&D_L01_00161524, m);
        func_L00_001FF4B0(b, d + 0x20, 1.0f);
        func_L00_0025A8C0_x(a, m, 0, b, 0.0f);
        if (func_L00_001F2BE8_x(m + 0x10, 0x10, *(int *)(d + 0x18), a, *(float *)&D_L01_0016152C) != 0 && func_001E97C8(m) == 0) {
            if (((int *)e)[6] != 0)
                func_L00_0025AC00_x(((int *)e)[6], 1.0f, m, 0x10001, e + 8, b);
            func_L01_002C84D8();
            func_0020D678(m);
        } else if (func_001F9908((int *)d) != 0) {
            *(int *)d = func_001F9850(0x1E);
            m[0x20] = 2;
            if (*(int *)(d + 0x10) != 0) *(int *)(d + 0x10) = 0;
        }
    } else if ((unsigned char)m[0x20] >= 2) {
      if ((unsigned char)m[0x20] == 2) {
        if (func_001F9908((int *)d) != 0) {
            func_0020D678(m);
        } else {
            float t = (float)func_001F9850(0x1E);
            m[0x23] = func_001FA898_r((float)*(int *)d * (127.0f / t));
        }
      }
    }
}
