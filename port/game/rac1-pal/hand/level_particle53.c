/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native PAL particle type 53 update, recovered from the complete 0x240
 * retail body and nonmatching/shared/func_L00_00273090.c. Not a PS2 match.
 * Byte accumulation wraps; RGB survives alpha updates. Out-of-bounds
 * removal precedes lifetime decrement, as in retail. */
extern float func_001FA888(int);
extern int func_001FA898(float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_002688A8(void *);
extern int func_001F9938(void *);

/* particle update: tween size/alpha over its life, drift, kill when out of range */
void func_L00_00273090(char *m) {
    char *v = m + 0x20;
    int a;
    float t;
    float f;
    unsigned c;
    unsigned char u;
    float *p;
    float vec[4];
    ((unsigned char *)m)[8] += ((unsigned char *)v)[0xA];
    a = *(short *)(v + 8);
    if (a - *(short *)(m + 0xA) < 6) {
        float s;
        t = (float)(a - *(short *)(m + 0xA)) / 5.0f;
        s = *(float *)(v + 4);
        *(float *)(m + 0xC) = s + ((*(float *)(m + 0x20) + s) * 0.5f - s) * t;
        u = v[0xB];
        f = (float)u + (float)((u >> 1) - u) * t;
    } else {
        float s;
        float q = func_001FA888(a - 6);
        s = *(float *)(m + 0x20);
        t = (float)*(short *)(m + 0xA) / q;
        *(float *)(m + 0xC) = s + ((s + *(float *)(v + 4)) * 0.5f - s) * t;
        u = v[0xB];
        f = (float)(u >> 1) * t + 0.0f;
    }
    c = func_001FA898(f);
    *(unsigned *)(m + 4) = (c << 24) | (*(unsigned *)(m + 4) & 0xFFFFFFu);
    p = (float *)(v + 0xC);
    vec[0] = p[0];
    vec[1] = p[1];

    vec[3] = 0;
    vec[2] = p[2];
    func_001F9BD8(m + 0x10, m + 0x10, vec);
    *(float *)(v + 0x14) -= *(float *)(v + 0x18);
    if (*(float *)(m + 0x10) < 2.0f || *(float *)(m + 0x10) > 1021.0f ||
        *(float *)(m + 0x14) < 2.0f || *(float *)(m + 0x14) > 1021.0f ||
        *(float *)(m + 0x18) < 2.0f || *(float *)(m + 0x18) > 1021.0f) {
        func_L00_002688A8(m);
    } else if (func_001F9938(m + 0xA)) {
        func_L00_002688A8(m);
    }
}
