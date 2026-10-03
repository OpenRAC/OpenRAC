/* NON_MATCHING func_L05_002D8268 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: SIZE ours 292 / retail 288, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L05_002D8268: per-frame update of a moby's pulsing colour (loop of 3 calls, angle step, colour from func_
 *   Size and all instructions right (p10, 23 bytes differ): only the scheduling of the D_L05_0015F6B0 load (local 
 */
extern void func_L00_00250800(void *, int, void *);
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern int func_001FA8A8(int, int, float);
extern void func_001F49B0(void (*)(void), void *);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L05_0015F6B0;

// Updates a moby's pulsing colour and its draw hook.
void func_L05_002D8268(char *moby) {
    if ((unsigned char)moby[0x21] != 0xFF && ((unsigned char *)moby)[0x31] != 0) {
        char *data = *(char **)(moby + 0x78);
        char *p = data + 0x330;
        int i;
        int col;
        int v;
        int hi;
        for (i = 0; i < 3; i++) {
            func_L00_00250800(moby, i, p);
            p += 0x10;
        }
        *(float *)(data + 0x368) = func_001FA748(*(float *)(data + 0x368), D_0015EE6C * 1.9198622f);
        col = func_001FA8A8(0x1E1EB4, 0x1EB41E, func_001F9FA8(*(float *)(data + 0x368)) * 0.5f + 0.5f);
        *(int *)(moby + 0x90) = col;
        hi = (col & 0xFF0000) | 0x30000000;
        *(int *)(data + 0x360) = hi | (col & 0xFF00) | (col & 0xFF);
        v = D_L05_0015F6B0;
        if (v != 0) {
            int *q = *(int **)(data + 0x364);
            if (v != *q) {
                *q = v;
                func_001F49B0(func_L05_002D8190, moby);
            }
        }
    }
}
