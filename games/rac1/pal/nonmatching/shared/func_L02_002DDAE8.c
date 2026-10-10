/* NON_MATCHING func_L02_002DDAE8 -- src/overlays/shared/vendor_002A5218.c
 * Best so far: SIZE ours 848 / retail 864, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Escalator/conveyor update (864 bytes): state byte at moby+0x20 picks one of four motion modes; each arm calls 
 */
extern void func_001F9BC0(void *);
extern int func_001F9850(int);
extern int func_001F9908(int *);
extern float func_001F9CB8(void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001F9B88(float);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L02_00161CA8;
extern short D_L02_00161CAC;
extern short D_L02_00161CA4;

// Escalator / conveyor update: picks a motion mode from the state byte at 0x20 and steps the moby's transform.
void func_L02_002DDAE8(char *moby) {
    char *d;
    char *a;
    int s;
    int r;
    float f20;
    float f21;
    float f22;
    float g;
    float h;

    d = *(char **)(moby + 0x78);
    if (d == 0) {
        return;
    }
    s = *(unsigned char *)(moby + 0x20);
    switch (s) {
    case 0: {
        a = d + 0x40;
        func_001F9BC0(a);
        *(int *)(d + 0xA0) = func_001F9850(*(int *)&D_L02_00161CA8);
        *(int *)(d + 0x9C) = *(int *)(d + 0x9C) | 4;
        if (*(int *)(d + 0xA4)) {
            moby[0x20] = 3;
        } else {
            moby[0x20] = 1;
        }
        break;
    }
    case 1: {
        a = d + 0x40;
        *(float *)(moby + 0x58) = 1.0f;
        r = func_001F9908((int *)(d + 0xA0));
        if (r) {
            f22 = 0.0f;
            f21 = func_001F9CB8(a) - *(float *)&D_L02_00161CAC * D_0015EE70;
            if (f21 <= f22) {
                *(int *)(d + 0xA0) = func_001F9850(*(int *)&D_L02_00161CA8);
                moby[0x20] = 2;
            } else {
                g = func_001F9F90(*(float *)(moby + 0x48));
                f20 = -f21;
                *(float *)(d + 0x40) = g * f20;
                *(float *)(d + 0x48) = f22;
                h = func_001F9FA8(*(float *)(moby + 0x48));
                *(float *)(d + 0x44) = h * f20;
            }
        } else {
            f21 = func_001F9CB8(d + 0x40);
            if (f21 < func_001F9B88(*(float *)&D_L02_00161CA4 * D_0015EE6C)) {
                f21 = f21 + *(float *)&D_L02_00161CAC * D_0015EE70;
            }
            g = func_001F9F90(*(float *)(moby + 0x48));
            f20 = -f21;
            *(float *)(d + 0x40) = g * f20;
            h = func_001F9FA8(*(float *)(moby + 0x48));
            *(int *)(d + 0x48) = 0;
            *(float *)(d + 0x44) = h * f20;
        }
        *(float *)(moby + 0x58) = f21 / (*(float *)&D_L02_00161CA4 * D_0015EE6C);
        break;
    }
    case 2: {
        a = d + 0x40;
        *(float *)(moby + 0x58) = -1.0f;
        r = func_001F9908((int *)(d + 0xA0));
        if (r) {
            f21 = 0.0f;
            f20 = func_001F9CB8(d + 0x40) - *(float *)&D_L02_00161CAC * D_0015EE70;
            if (f20 <= f21) {
                *(int *)(d + 0xA0) = func_001F9850(*(int *)&D_L02_00161CA8);
                moby[0x20] = 1;
            } else {
                *(float *)(d + 0x40) = func_001F9F90(*(float *)(moby + 0x48)) * f20;
                *(float *)(d + 0x44) = func_001F9FA8(*(float *)(moby + 0x48)) * f20;
                *(float *)(d + 0x48) = f21;
            }
        } else {
            f20 = func_001F9CB8(d + 0x40);
            if (f20 < func_001F9B88(*(float *)&D_L02_00161CA4 * D_0015EE6C)) {
                f20 = f20 + *(float *)&D_L02_00161CAC * D_0015EE70;
            }
            *(float *)(d + 0x40) = func_001F9F90(*(float *)(moby + 0x48)) * f20;
            *(float *)(d + 0x44) = func_001F9FA8(*(float *)(moby + 0x48)) * f20;
            *(int *)(d + 0x48) = 0;
        }
        *(float *)(moby + 0x58) = f20 / -(*(float *)&D_L02_00161CA4 * D_0015EE6C);
        break;
    }
    case 3: {
        a = d + 0x40;
        *(float *)(d + 0x40) = func_001F9F90(*(float *)(moby + 0x48)) * -(*(float *)&D_L02_00161CA4 * D_0015EE6C);
        *(int *)(d + 0x48) = 0;
        *(float *)(d + 0x44) = func_001F9FA8(*(float *)(moby + 0x48)) * -(*(float *)&D_L02_00161CA4 * D_0015EE6C);
        break;
    }
    default:
        a = d + 0x40;
        break;
    }
    func_L00_002617B0(d + 0x60, a, moby + 0x40, moby + 0x40);
}
