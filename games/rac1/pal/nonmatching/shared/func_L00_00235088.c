/* NON_MATCHING func_L00_00235088 -- src/overlays/shared/help_00232560.c
 * Best so far: SIZE ours 588 / retail 584, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00235088: per-frame pulse of three HUD objects (D_0013F450+0x11D0/0x11D4/0x1220): advances a phase (f
 *   Budget spent at p9.c (BYTES 58/584, structure right: one shared func_001FA8A8 call after the if/else in block 
 *   (2) retail has g in $s0, p in $s1 (mine swapped): the block-1 argument load is a fresh `addiu $2,$s2,lo` (temp
 */
extern char D_0013F450[];
extern float D_0015EE6C MACRO_ADDR;
extern int D_L00_0015F6A8 MACRO_ADDR;
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern int func_001FA8A8(int, int, float);
extern void func_0020D960(char *, int, void *);
extern void func_L00_001FFED8(void *, int, float);

/* Pulses the colours of the three HUD objects at D_0013F450 + 0x11D0/0x11D4/0x1220 with a sine of a running phase. */
void func_L00_00235088(void) {
    char *g = D_0013F450;
    char *p;
    int col;

    p = *(char **)(g + 0x11D0);
    if (p != 0) {
        if (D_L00_0015F6A8 == 2) {
            *(float *)(g + 0x22E8) = func_001FA748(*(float *)(g + 0x22E8), D_0015EE6C * 1.2118f);
        } else {
            *(float *)(g + 0x22E8) = func_001FA748(*(float *)(g + 0x22E8), D_0015EE6C * 2.0946f);
        }
        *(int *)(p + 0x90) = func_001FA8A8(0xD2D2D2, 0x285050, func_001F9FA8(*(float *)(g + 0x22E8)) * 0.5f + 0.5f);
    }
    g = D_0013F450;
    p = *(char **)(g + 0x11D4);
    if (p != 0) {
        float t;
        if (D_L00_0015F6A8 != 2) {
            int c;
            *(float *)(g + 0x22EC) = func_001FA748(*(float *)(g + 0x22EC), D_0015EE6C * 2.9669f);
            c = 0x1EE628;
            if (*(float *)(g + 0x22EC) > 0.0f) {
                if (*(float *)(g + 0x22EC) < 1.9198f) {
                    c = 0x1E1ED2;
                }
            }
            col = func_001FA8A8(*(int *)(p + 0x90), c, 0.07f);
        } else {
            t = func_001FA748(*(float *)(g + 0x22EC), D_0015EE6C * 1.5708f);
            *(float *)(g + 0x22EC) = t;
            col = func_001FA8A8(0x1E1ED2, 0x1E1E50, func_001F9FA8(t) * 0.5f + 0.5f);
        }
        *(int *)(p + 0x90) = col;
        g = D_0013F450;
    }
    p = *(char **)(g + 0x1220);
    if (p != 0) {
        if (D_L00_0015F6A8 == 2) {
            *(float *)(g + 0x22F0) = func_001FA748(*(float *)(g + 0x22EC), D_0015EE6C * 1.3961f);
        } else {
            *(float *)(g + 0x22F0) = func_001FA748(*(float *)(g + 0x22EC), D_0015EE6C * 1.9198f);
        }
        g = D_0013F450;
        *(int *)(p + 0x90) = func_001FA8A8(0xAAC8AA, 0x965050, func_001F9FA8(*(float *)(g + 0x22F0)) * 0.5f + 0.5f);
        if (p != 0) {
            if (*(unsigned char *)(g + 0x1D91) == 0) {
                func_0020D960(p, 0, g + 0x1D90);
            } else {
                func_L00_001FFED8(g + 0x1DA0, 0, *(float *)(g + 0x22F0));
            }
        }
    }
}
