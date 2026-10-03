/* NON_MATCHING func_L11_00310770 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 372 / retail 364, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L11_00310770 (UpdateMoby_1180): 3-state moby update (0 init, 1 wait on func_L11_00310738 then state 2/3, 
 *   Everything matches except case 2's float scheduling: retail loads EE70, k1(f0), EE6C(f1), k2 after `mov.s f12,
 *   Unblock: some source shape that changes pseudo-creation order of the two scaled floats; likely a scheduler tie
 */
extern int func_L11_00310738(char *moby);
extern void func_0022ED80(int, int, int);
extern float func_001FA748(float, float);
extern float func_L00_0025CE58(float *p, float *v, float a, float b, float c, float d);
extern int D_L11_00160058_m __asm__("D_L11_00160058") MACRO_ADDR;
extern unsigned char D_0013D4EB NOT_SDA;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L11_0015F6B0;

// Update for a moby that waits, then animates toward its target and changes state.
void func_L11_00310770(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int state = (unsigned char)moby[0x20];
    switch (state) {
    case 0:
        ((unsigned char *)moby)[0x30] = 0xFF;
        moby[0x20] = 1;
        *(float *)(data + 8) = *(float *)(moby + 0x48);
        break;
    case 1:
        if (func_L11_00310738((char *)(D_L11_00160058_m + (*(int *)data << 8)))) {
            if (*(int *)&D_L11_0015F6B0 > 10) {
                D_0013D4EB = state;
                moby[0x20] = 2;
                func_0022ED80(0, 0, (int)moby);
            } else {
                moby[0x20] = 3;
                *(float *)(moby + 0x48) = func_001FA748(*(float *)(data + 8), 1.0471976f);
            }
        }
        break;
    case 2:
        if (func_L00_0025CE58((float *)(moby + 0x48), (float *)(data + 4),
                              func_001FA748(*(float *)(data + 8), 1.0471976f),
                              D_0015EE70 * 0.34906584f, D_0015EE70 * 0.34906584f,
                              D_0015EE6C * 0.7853982f) == 0.0f)
            moby[0x20] = 3;
        break;
    }
}
