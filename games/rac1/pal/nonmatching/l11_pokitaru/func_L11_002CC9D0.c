/* NON_MATCHING func_L11_002CC9D0 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: BYTES 27/380 (92.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L11_002CC9D0 (UpdateMoby_65): state 0 copies the moby into a child (class 0x41, flag 0x8000) once; state 
 *   Everything matches except 6 instructions in state 2: retail schedules `daddu $a1,data` after the k1 constant c
 *   Note state 0 store order: `sb state` before `swc1` is reached by writing the state store first.
 */
extern char *func_0020D348(int);
extern float func_001FA748(float, float);
extern float func_001FA790(float, float);
extern float func_L00_0025CE58(float *p, float *v, float a, float b, float c, float d);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

// Update for a moby that spawns a child copy once, then eases along a path and changes state.
void func_L11_002CC9D0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int state = (unsigned char)moby[0x20];
    switch (state) {
    case 0:
        moby[0x20] = 1;
        *(float *)(data + 4) = *(float *)(moby + 0x48);
        if ((*(unsigned short *)(moby + 0x34) & 0x8000) == 0) {
            char *c = func_0020D348(0x41);
            *(short *)(c + 0x32) = *(short *)(moby + 0x32);
            c[0x30] = moby[0x30];
            *(unsigned short *)(c + 0x34) = *(unsigned short *)(moby + 0x34) | 0x8000;
            qcopy(c + 0x10, moby + 0x10);
            qcopy(c + 0x40, moby + 0x40);
            *(char **)(data + 8) = c;
            *(float *)(*(char **)(c + 0x78) + 4) = *(float *)(moby + 0x48);
        }
        break;
    case 1:
        break;
    case 2: {
        float f;
        if ((*(unsigned short *)(moby + 0x34) & 0x8000) == 0)
            f = func_001FA748(*(float *)(data + 4), 0.20769417f);
        else
            f = func_001FA790(*(float *)(data + 4), 0.20769417f);
        if (func_L00_0025CE58((float *)(moby + 0x48), (float *)data, f, D_0015EE70 * 0.34906584f,
                              D_0015EE70 * 0.34906584f, D_0015EE6C * 0.7853982f) == 0.0f)
            moby[0x20] = 3;
        break;
    }
    }
}
