extern float func_001EC120(void *, float, float, float, float, float);
extern float D_L00_0015F044 NOT_SDA;
extern int D_L00_0015F060 NOT_SDA;
extern char *D_L00_001690C0 NOT_SDA;
extern float D_L00_00161D70 MACRO_ADDR;
extern float D_L00_00161DAC MACRO_ADDR;
extern float D_L00_00161DB0 MACRO_ADDR;
extern float D_L00_00161DB4 MACRO_ADDR;
extern float D_L00_00161DD4 MACRO_ADDR;
extern float D_L00_00161DD8 MACRO_ADDR;
extern float D_L00_00161DEC MACRO_ADDR;

/* Resets a camera-style block of the object at a0 (its state at +0x70): zeroes the spring state, eases
   the two blend values toward the template at D_L00_001690C0 with func_001EC120 (a damped step), and
   writes the default timing constants. */
void func_L00_002E72E8(char *a0) {
    char *m;
    char *t;
    char *p;
    char *g;
    char *q;
    float f13;
    float r;

    m = *(char **)(a0 + 0x70);
    q = m + 0x1A8;
    *(float *)(q + 0x14) = D_L00_0015F044;
    *(int *)(q + 0x1C) = 0;
    *(int *)(q + 0x20) = 0;
    *(int *)(q + 0x10) = 0;
    *(int *)(q + 0x24) = 0;
    m = *(char **)(a0 + 0x70);
    t = D_L00_001690C0;
    p = m + 0x130;
    g = m + 0x40;

    if (*(short *)(p + 0x3C) != 0) {
        f13 = *(float *)(p + 0x40);
    } else {
        f13 = *(float *)(t + 0x130 + 0x2C);
    }
    r = func_001EC120(m + 0x174, *(float *)(p + 0x2C), f13, *(float *)(p + 0x48), D_L00_00161DD4,
                      *(float *)(q + 0x1C));
    *(float *)(p + 0x2C) = r;

    if (*(short *)(p + 0x3E) != 0) {
        f13 = *(float *)(p + 0x4C);
    } else {
        f13 = *(float *)(t + 0x130 + 0x30);
    }
    r = func_001EC120(m + 0x180, *(float *)(p + 0x30), f13, *(float *)(p + 0x54), D_L00_00161DD8, 0.0f);
    *(float *)(p + 0x30) = r;

    if (*(short *)(g + 0xDA) != 0) {
        f13 = *(float *)(g + 0xB4);
    } else {
        f13 = *(float *)(t + 0x40 + 0xB0);
    }
    r = func_001EC120(g + 0xBC, *(float *)(g + 0xB0), f13, *(float *)(g + 0xB8), 0.2f, 0.0f);
    *(float *)(g + 0xB0) = r;

    *(short *)(p + 0x3C) = 0;
    *(short *)(p + 0x3E) = 0;
    *(int *)(p + 0x40) = 0;
    *(int *)(p + 0x4C) = 0;
    *(float *)(p + 0x58) = *(float *)(t + 0x130 + 0x2C);
    *(short *)(g + 0xDA) = 0;
    *(int *)(g + 0xB4) = 0;

    m = *(char **)(a0 + 0x70);
    *(short *)(m + 0x10) = 1;
    m = *(char **)(a0 + 0x70);
    *(short *)(m + 0x22) = 0;
    *(unsigned char *)(g + 0xD6) = 0;
    *(float *)(g + 0xE4) = D_L00_00161D70;
    *(float *)(g + 0xE8) = 0.2f;
    *(float *)(g + 0xDC) = D_L00_00161D70;
    *(float *)(g + 0xE0) = 0.2f;

    m = *(char **)(a0 + 0x70);
    *(float *)(m + 0x1D0 + 0x44) = D_L00_00161DAC;
    *(int *)(m + 0x1D0 + 0x48) = D_L00_0015F060;
    *(float *)(m + 0x1D0 + 0x3C) = D_L00_00161DB4;
    *(float *)(m + 0x1D0 + 0x40) = D_L00_00161DB0;

    m = *(char **)(a0 + 0x70);
    *(float *)(m + 0x220 + 0xC) = D_L00_00161DEC;
    *(short *)(m + 0x220 + 0x6) = 0;
}
