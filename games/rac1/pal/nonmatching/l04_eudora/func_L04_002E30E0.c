/* NON_MATCHING func_L04_002E30E0 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: SIZE ours 572 / retail 568, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Four-sparkle ring updater (4 iterations: wrap float, timer, func_001FA8A8 colour, func_L00_00273E08 spawn). Be
 */
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9908(int *);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern float func_001F9B88(float);
extern int func_001FA8A8(int, int, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern unsigned char *func_L00_00273E08(float f, void *pos, int a1, unsigned char a2, int idx, int flag, int s, int mode);
extern float D_L04_00166FC0[];
extern int D_L04_0015F6A8 MACRO_ADDR;

/* Spins a ring of four sparkles around the moby, fading by their timers. */
void func_L04_002E30E0(char *moby) {
    float v0[4], v10[4];
    char *d;
    float *pp;
    int *e;
    int n, i;
    char *pos = moby + 0x10;
    d = *(char **)(moby + 0x78);
    *(float *)(moby + 0x18) += 0.25f;
    func_001F9BF0(v0, D_L04_00166FC0, pos);
    func_L00_001FF4B0(v0, v0, -0.3f);
    func_L00_001FF4B0(v10, v0, 0.1f);
    func_001F9BD8(v0, v0, pos);
    *(float *)(moby + 0x18) -= 0.25f;
    pp = (float *)(d + 0x10);
    e = (int *)(d + 0x30);
    i = 0;
    for (n = 3; n >= 0; n--) {
        float f1 = *pp + *(float *)(d + i + 0x20);
        float t;
        int s;
        *pp = f1;
        s = (int)(d + 0x30);
        if (255.0f <= f1) {
            *pp = f1 - 255.0f;
        } else if (f1 <= 0.0f) {
            *pp = f1 + 255.0f;
        }
        if (func_001F9908(e)) {
            *(int *)(s + i) = func_001F9850(255);
        }
        t = func_001FA888(func_001F9850(255) - *(int *)(s + i));
        t = t / (float)func_001F9850(255);
        s = func_001FA8A8(0x4040FFFF, 0x1040FFFF, func_001F9B88(0.5f - t));
        if (D_L04_0015F6A8 != 2) {
            func_L00_00273E08(*(float *)(d + i + 0x40), v0, s, func_001FA898_r(*pp), 0x35, 1, 2, 0);
        }
        func_001F9BD8(v0, v0, v10);
        e++;
        i += 4;
        pp++;
    }
}
