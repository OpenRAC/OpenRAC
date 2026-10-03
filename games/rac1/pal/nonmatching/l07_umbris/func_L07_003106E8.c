/* NON_MATCHING func_L07_003106E8 -- src/overlays/l07_umbris/vendor_002CE470.c
 * Best so far: BYTES 8/452 (98.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Checks moby state bytes and a distance window, then sweeps a path and flips the moby state. Best is p5.c.
 *   Only difference: o (arg1) and a (arg4, t0) saved in s4/s5 swapped (o lands in s4, retail s5); six instructions
 *   Would need a way to change the allocator's priority between the two; rewording locals did not move it.
 */
extern float func_0020D830(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_0025A8C0(char *arg, int a, int b, void *src, float scale);
extern void func_L00_00250800(void *, int, void *);
extern int func_L00_001F2BE8_alt(void *, float, int, void *, void *) __asm__("func_L00_001F2BE8");
extern void func_001F9C30(void *, void *, float);
extern void func_00213DE0(void *, int, int, int);
extern int D_L07_00173F58;
extern float D_0015EE60 MACRO_ADDR;

/* Checks a moby's state and distance, then pushes it along a swept path. */
void func_L07_003106E8(char *m, char *o, int s1, int s2, int a, int b, int c, int d,
                       float lo, float hi, float f3, float f4, float f5) {
    float v[4];
    float w[12];
    float x[4];
    float dist = func_0020D830(m);
    if ((unsigned char)m[0x20] == s1 && (unsigned char)m[0x52] == s2 && lo < dist && dist < hi) {
        char *dir = o + 0xD0;
        func_L00_001FF4B0(v, dir, f3);
        v[2] = 1.0f;
        func_L00_0025A8C0((char *)w, (int)m, 1, v, f4);
        func_L00_00250800(m, 0, x);
        if (func_L00_001F2BE8_alt(x, f5, 0x21, m, w)) {
            if (D_L07_00173F58 == d) {
                func_001F9C30(dir, dir, -1.0f);
                *(float *)(o + 0xD8) = -(D_0015EE60 * 0.05f);
                if ((unsigned char)m[0x53] != a) func_00213DE0(m, a, b, 5);
                m[0x20] = c;
            }
        }
    }
}
