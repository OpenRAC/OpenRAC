/* NON_MATCHING func_L00_002C2A80 -- src/overlays/shared/vendor_002C12B0.c
 * Best so far: BYTES 8/432 (98.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Three-state update of a moby (switch on byte 0x20): 0 sets angles and height, 1 polls proximity every 10th fra
 *   p8.c is 8/432 bytes off: only the order of the two lwc1 in state 0 (retail loads the gp float D_L00_001618F8 i
 *   Writing the sum the other way round (D + gp) fixes that pair but flips the constant loads/stores above it; loc
 */
extern unsigned char D_0013E633[] NOT_SDA;
extern float func_00214358(void *, int, float);
extern float func_001FA748(float, float);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern void func_L00_002618D8(int, int);
extern float func_00214D28(float *, float, float);
extern void func_0020D678(void *);
extern float D_L00_00173F68;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern short D_L00_001618F8;
extern short D_L00_00161900;

/* Three-state update of a spinning pickup: set up angles, then spin until the player is close, then shrink and delete. */
void func_L00_002C2A80(void *arg) {
    char *m = arg;
    float f;
    int c;
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        func_00214358(m + 0x10, 0, 0.5f);
        *(float *)(m + 0x40) = 1.5707964f;
        *(float *)(m + 0x44) = 3.1415927f;
        *(float *)(m + 0x18) = *(float *)&D_L00_001618F8 + D_L00_00173F68;
        m[0x20] = 1;
        break;
    case 1: {
        char *g;
        f = func_001FA748(*(float *)(m + 0x48), *(float *)&D_L00_00161900);
        c = D_L00_0015F6B0;
        *(float *)(m + 0x48) = f;
        if (c % 10 != 0) return;
        g = (char *)D_0013E633 + 0xE1D;
        if (*(int *)(g + 0x22A8) == 0) return;
        if (func_001F9D48(m + 0x10, g + 0x80) < 1.0f) {
            if (func_001F9B88(*(float *)(m + 0x18) - *(float *)(g + 0x88)) < 2.0f) {
                func_L00_002618D8(0x1A, 1);
                m[0x20] = 2;
            }
        }
        break;
    }
    case 2:
        func_00214D28((float *)(m + 0x2C), 0.0f, *(float *)(*(char **)(m + 0x24) + 0x24) * 0.02f);
        if (*(float *)(m + 0x2C) == 0.0f) {
            func_0020D678(m);
        }
        break;
    }
}
