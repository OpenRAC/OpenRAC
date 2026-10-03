/* NON_MATCHING func_L05_00316378 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: SIZE ours 676 / retail 576, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 5 timed platform moby update (states 0 init, 1 wait for trigger, 2 move along path with two func_00214D8
 *   Left: one addu operand order in state 1 (`addu $v0,$v0,$v1` idx first in retail vs ours base first); written b
 */
extern int func_001F9908(int *arg0);
extern float func_001F9B88(float);
extern void func_0022ED80(int, int, int);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern int D_L05_00160098_m __asm__("D_L05_00160098") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern short D_L05_00161F40;
extern short D_L05_00161F44;
extern short D_L05_00161F48;
extern short D_L05_00161F4C;
extern short D_L05_00161F50;
extern short D_L05_00161F54;

// Update of a timed platform-style moby: states 0 spawn, 1 wait, 2 move along path.
void func_L05_00316378(char *m) {
    char *d = *(char **)(m + 0x78);
    switch ((unsigned char)m[0x20]) {
    case 0:
        *(float *)(d + 0xC) = *(float *)(m + 0x18) + 1.0f;
        m[0x20] = 1;
        *(float *)(m + 0x18) = *(float *)(m + 0x18) - 8.0f;
        break;
    case 1: {
        int off = *(int *)d << 8;
        char *b = (char *)D_L05_00160098_m;
        int v = *(unsigned char *)(off + b + 0xBC);
        if (v == 1) {
            m[0x20] = 2;
        } else if (v == 2) {
            float t = *(float *)(d + 0xC);
            m[0x20] = 3;
            *(float *)(m + 0x18) = t;
            *(int *)(m + 0x44) = 0;
        }
        break;
    }
    case 2:
        if (func_001F9908((int *)(d + 4))) {
            func_00214D88((float *)(m + 0x18), (float *)(d + 0x10), *(float *)(d + 0xC),
                          *(float *)&D_L05_00161F40 * D_0015EE70, *(float *)&D_L05_00161F44 * D_0015EE70,
                          *(float *)&D_L05_00161F48 * D_0015EE6C);
            if (*(short *)(m + 0xA6) == 0x355) {
                if (func_001F9B88(*(float *)(m + 0x18) - *(float *)(d + 0xC)) < 3.0f
                    && 3.0f < func_001F9B88(*(float *)(d + 0xC) - *(float *)(m + 0x18) + *(float *)(d + 0x10))) {
                    func_0022ED80(0, 0, (int)m);
                }
            }
        }
        if (func_001F9908((int *)(d + 8))) {
            func_L00_0025CE58((float *)(m + 0x44), (float *)(d + 0x14), 0.0f,
                              *(float *)&D_L05_00161F4C * 0.017453292f * D_0015EE70,
                              *(float *)&D_L05_00161F50 * 0.017453292f * D_0015EE70,
                              *(float *)&D_L05_00161F54 * 0.017453292f * D_0015EE6C);
            if (*(float *)(m + 0x44) == 0.0f) {
                m[0x20] = 3;
            }
        } else if (*(int *)(d + 8) == 1 && *(short *)(m + 0xA6) == 0x354) {
            func_0022ED80(0, 0, (int)m);
        }
        break;
    }
}
