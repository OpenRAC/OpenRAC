/* NON_MATCHING func_L18_002D96B0 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: SIZE ours 1128 / retail 1104, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Floating platform update (moby class 587, 1104 bytes): calls func_001F9C30 to negate the moby offset into a st
 *   Best candidate p3.c: 1128 bytes (retail 1104). The jump table is right with a case label for every state (stat
 *   Unblock: a way to pin the 1.0f register load ahead of the multiplies (no new flags), or a regalloc dump for th
 */
extern void func_001F9C30(void *, void *, float);
extern float func_00214158(void);
extern int func_001F9908(int *);
extern float func_001FA888(int);
extern void func_L18_002D9CA8(char *moby);
extern void func_L18_002D9B00(char *moby);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern int func_0022ED80_2F0F78(int, int, int) __asm__("func_0022ED80");
extern float func_001F9D48(void *, void *);
extern void func_L18_002284E0(int, int);
extern void func_0020D678(void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern char D_0013E633[];
extern short D_L18_00162438;
extern short D_L18_00161AF0;
extern short D_L18_00161AF4;
extern short D_L18_00161AE0;
extern short D_L18_00161AE8;
extern short D_L18_00161AEC;
extern short D_L18_00161AD0;
extern short D_L18_00161AD4;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_L18_0015F718 MACRO_ADDR;

/* Floating platform update (moby class 587): eases its height and tilt towards the sway target by state. */
void func_L18_002D96B0(char *m) {
    char *data;
    char *p;
    float v0[4] __attribute__((aligned(16)));
    float v10[4] __attribute__((aligned(16)));
    float a;
    float b;
    float t;
    float u;
    float k;
    int r;
    float rf;
    float f;

    func_001F9C30(v0, m + 0x10, -1.0f);
    data = *(char **)(m + 0x78);
    qcopy(v10, m + 0x40);
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        f = *(float *)(m + 0x18);
        *(float *)(data + 0x60) = f;
        *(int *)(data + 0x70) = 0;
        *(int *)(data + 0x68) = 0;
        *(int *)(data + 0x6C) = 0;
        *(float *)(data + 0x64) = f;
        rf = func_00214158();
        *(int *)(data + 0x5C) = *(int *)(data + 0x5C) | 1;
        *(float *)(data + 0x74) = rf;
        if (*(int *)(data + 0x88) != 0) {
            m[0x20] = 2;
        } else {
            m[0x20] = 1;
        }
        break;
    case 3:
        if (func_001F9908((int *)(data + 0x7C)) == 0) {
            break;
        }
        a = func_001FA888(*(int *)&D_L18_00162438) - *(float *)&D_L18_00161AF0;
        b = *(float *)&D_L18_00161AF4 - *(float *)&D_L18_00161AF0;
        t = a / b;
        if (1.0f < t) {
            t = 1.0f;
        } else if (t < 0.0f) {
            t = 0.0f;
        }
        u = t * 0.333f;
        if (*(int *)(data + 0x84) != 0) {
            k = *(float *)&D_L18_00161AE0;
        } else {
            k = *(float *)&D_L18_00161AE8;
        }
        *(float *)(data + 0x6C) = (k * (1.0f - u)) * 0.017453292f * D_0015EE6C;
        m[0x20] = 4;
        break;
    case 4:
        if (*(unsigned char *)(m + 0x31) != 0 && func_001F9908((int *)(data + 0x8C)) != 0) {
            func_L18_002D9CA8(m);
            r = func_L00_00258BC8(0x5A, 0x78);
            *(int *)(data + 0x8C) = func_001F9850(r);
        }
        p = D_0013E633 + 0xE1D;
        if (*(char **)(p + 0x2FC) == m && *(short *)(p + 0x30E) == 0) {
            *(int *)(data + 0x8C) = 0;
            m[0x20] = 5;
            *(short *)(data + 0x92) = func_0022ED80_2F0F78(1, 0, (int)m);
        }
        if (*(int *)(data + 0x80) == 0) {
            break;
        }
        if (func_001F9D48(m + 0x10, D_0013E633 + 0xE9D) < *(float *)&D_L18_00161AEC) {
            m[0x20] = 5;
            *(short *)(data + 0x92) = func_0022ED80_2F0F78(1, 0, (int)m);
        }
        break;
    case 5:
        if (*(unsigned char *)(m + 0x31) != 0 && func_001F9908((int *)(data + 0x8C)) != 0) {
            func_L18_002D9CA8(m);
            r = func_L00_00258BC8(0xF, 0x1E);
            *(int *)(data + 0x8C) = func_001F9850(r);
        }
        a = func_001FA888(*(int *)&D_L18_00162438) - *(float *)&D_L18_00161AF0;
        b = *(float *)&D_L18_00161AF4 - *(float *)&D_L18_00161AF0;
        t = a / b;
        if (1.0f < t) {
            t = 1.0f;
        } else if (t < 0.0f) {
            t = 0.0f;
        }
        u = t * 0.333f;
        if (*(int *)(data + 0x80) != 0 || (*(int *)(data + 0x88) != 0 && *(int *)(data + 0x84) == 0)) {
            *(float *)(data + 0x70) = *(float *)(data + 0x70) - D_0015EE70 * 15.0f;
        } else if (*(int *)(data + 0x84) != 0) {
            *(float *)(data + 0x70) = *(float *)(data + 0x70) - *(float *)&D_L18_00161AD0 * (1.0f - u) * D_0015EE70;
        } else {
            *(float *)(data + 0x70) = *(float *)(data + 0x70) - *(float *)&D_L18_00161AD4 * (1.0f - u) * D_0015EE70;
        }
        *(float *)(data + 0x64) = *(float *)(data + 0x64) + *(float *)(data + 0x70);
        if (*(float *)(m + 0x18) < *(float *)(data + 0x60) - 10.0f) {
            p = D_0013E633 + 0xE1D;
            if (*(char **)(p + 0x2FC) == m && *(short *)(p + 0x30E) == 0) {
                if (*(float *)(p + 0x88) < D_L18_0015F718) {
                    if (*(int *)(p + 0x2084) != 0x77) {
                        func_L18_002284E0(0x77, 1);
                    }
                }
            }
            *(float *)(data + 0x70) = *(float *)(data + 0x70) - D_0015EE70 * 13.7f;
            if (*(float *)(m + 0x18) < 15.0f) {
                func_0020D678(m);
                return;
            }
        }
        break;
    case 1:
        break;
    case 2:
        break;
    default:
        break;
    }
    func_L18_002D9B00(m);
    func_001F9BD8(v0, v0, m + 0x10);
    func_L00_002617B0(data + 0x20, v0, v10, m + 0x40);
}
