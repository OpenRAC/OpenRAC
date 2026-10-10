/* NON_MATCHING func_L11_0030F188 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: BYTES 8/864 (99.1% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Pokitaru moby: two clamped angles (f22/f23) from the moby's 0x6C float, then a 4x6 loop over state pointer rin
 *   Left: at the top of the outer loop retail sets the inner counter (j = 0, daddu $s5) before the i<2 flag (slti 
 */
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_001FA790(float, float);
extern void func_001FA1F8(void *, void *);
extern void func_L00_00251E30(void *);
extern short D_L11_00161FD0;
extern short D_L11_00161FD4;
extern short D_L11_00161FD8;

// Pokitaru moby: aims its 24 attached parts in four rings of six, clamping each angle.
void func_L11_0030F188(char *moby) {
    char *state;
    char **pp;
    float v[12];
    float f2, f20, f21, f22, f23;
    int i, j, lo, odd, next;

    state = *(char **)(moby + 0x78);
    func_L00_001FF4B0(v, moby + 0xC0, *(float *)&D_L11_00161FD0);
    func_L00_001FF4B0(v + 4, moby + 0xD0, *(float *)&D_L11_00161FD4);

    f2 = *(float *)(state + 0x6C);
    f23 = 90.0f - f2 * 180.0f;
    if (f23 < -90.0f) f23 = -90.0f;
    f22 = 90.0f - (f2 - 0.5f) * 180.0f;
    if (90.0f < f22) f22 = 90.0f;
    else if (f22 < 0.0f) f22 = 0.0f;

    i = 0;
    while (i < 4) {
        pp = (char **)(state + i * 0x18);
        j = 0;
        lo = i < 2;
        odd = i & 1;
        next = i + 1;
        for (; j < 6; j++) {
            f21 = (i < 2) ? f23 : f22;
            if (i < 2) {
                func_001F9BD8(*pp + 0x10, moby + 0x10, v);
            } else {
                func_001F9BF0(*pp + 0x10, moby + 0x10, v);
            }
            f20 = ((float)j - 2.5f) * 0.2617994f;
            if (odd) {
                float r1 = func_001FA748(*(float *)(moby + 0x48), 0.0f);
                *(float *)(*pp + 0x48) = r1;
                f20 = func_001FA748(f20, f21 * 0.017453292f);
                if (f20 < -0.7853982f) f20 += 1.5707964f;
            } else {
                float r1 = func_001FA748(*(float *)(moby + 0x48), 3.1415927f);
                *(float *)(*pp + 0x48) = r1;
                f20 = func_001FA790(f20, f21 * 0.017453292f);
                if (0.7853982f < f20) f20 -= 1.5707964f;
            }
            if (0.7853982f < f20) f20 = 0.7853982f;
            else if (f20 < -0.7853982f) f20 = -0.7853982f;
            *(float *)(*pp + 0x48) = func_001FA748(*(float *)(*pp + 0x48), f20);
            func_001FA1F8(*pp + 0xC0, *pp + 0x40);
            func_L00_001FF4B0(v + 8, *pp + 0xD0, -*(float *)&D_L11_00161FD8);
            if (odd) {
                func_001F9BD8(v + 8, v + 8, v + 4);
            } else {
                func_001F9BF0(v + 8, v + 8, v + 4);
            }
            func_001F9BD8(*pp + 0x10, *pp + 0x10, v + 8);
            func_L00_00251E30(*pp);
            pp++;
        }
        i = next;
    }
}
