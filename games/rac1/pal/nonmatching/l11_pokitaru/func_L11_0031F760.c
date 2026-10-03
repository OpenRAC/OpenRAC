/* NON_MATCHING func_L11_0031F760 -- src/overlays/l11_pokitaru/vendor_0031EFC0.c
 * Best so far: BYTES 40/952 (95.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L11_0031F760 (UpdateMoby_1903, pokitaru): switch on moby[0x20]: case 0 inits three particle sets and zero
 *   Best is p7.c (BYTES 57/952, sizes equal): only the pre-header order of the two small init loops differs (retai
 *   Five wordings (index form, pointer form, const in a local, for-init pointers) give the same header: a schedule
 *   fz6/x04: best now p12.c (40/952, from p9): arrays 00162628/0016262C/00162638/0016263C declared MACRO_ADDR (too
 */
#include "common.h"

extern void func_L08_002F2428(void *, int, void *, float, float, float, float, int);
extern int func_L00_00200290(char *, float);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *);
extern void func_L08_002F2760(int);
extern void func_L08_002F2618(void *, void *, float, float, float);
extern void func_L08_002F2838(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F49B0(void *, void *);
extern void func_L11_0031EFC0(void);

extern char D_L11_001F9D08[];
extern char D_L11_001F4C38[];
extern char D_L11_001FF9F0[];
extern char D_L11_001F9D38[];
extern int D_L11_001FFA10[];
extern char D_L11_001FFA18[];
extern int D_L11_00162640[] MACRO_ADDR;
extern int D_L11_00162628[] MACRO_ADDR;
extern float D_L11_0016262C[] MACRO_ADDR;
extern int D_L11_00162638[] MACRO_ADDR;
extern float D_L11_0016263C[] MACRO_ADDR;
extern char D_L11_00167700[];
extern float D_L11_0016257C MACRO_ADDR;
extern float D_L11_00162580 MACRO_ADDR;
extern float D_L11_00162584 MACRO_ADDR;
extern int D_L11_00162620 MACRO_ADDR;
extern short D_L11_001625BC;
extern short D_L11_00162578;
extern short D_L11_00162600;
extern short D_L11_001625C0;

// Update for the pokitaru moby 1903: sets up the glow state, then draws it near the camera.
void func_L11_0031F760(unsigned char *moby) {
    int i;
    switch (moby[0x20]) {
    case 0:
        if (*(int *)&D_L11_001625BC == 0) {
            func_L08_002F2428(&D_L11_00162600, 4, &D_L11_001625C0, 0.7f, 1.0f, 0.9f, 0.5f, 1);
            func_L08_002F2428(D_L11_001F9D08, 11, D_L11_001F4C38, 0.5f, 0.5f, 0.5f, 1.0f, 1);
            func_L08_002F2428(D_L11_001FF9F0, 8, D_L11_001F9D38, 0.25f, 0.25f, 0.25f, 0.7f, 1);
            *(int *)&D_L11_001625BC = 1;
        }
        moby[0x20] = 1;
        for (i = 0; i < 3; i++) {
            D_L11_001FFA10[i * 2] = 0;
            D_L11_001FFA10[i * 2 + 1] = 0;
            D_L11_00162640[0] = 0;
            D_L11_00162640[1] = 0;
        }
        {
            float *q;
            int *p;
            for (q = D_L11_0016262C, p = D_L11_00162628, i = 0; i < 2; i++) {
                *p = 0;
                *q = (float)i * 0.4f;
                p += 2;
                q += 2;
            }
        }
        {
            int *p;
            float *q;
            for (q = D_L11_0016263C, p = D_L11_00162638, i = 0; i < 1; i++) {
                *p = 0;
                *q = (float)i * 0.4f;
                p += 2;
                q += 2;
            }
        }
        break;
    case 1: {
        char *p = D_L11_00167700;
        float v[4];
        float w[4];
        float d;
        if (*(float *)(p + 0x148) < 170.0f) {
            return;
        }
        v[0] = *(float *)&D_L11_00162578;
        v[1] = D_L11_0016257C;
        v[2] = D_L11_00162580;
        v[3] = D_L11_00162584;
        if (func_L00_00200290((char *)v, 1000.0f) < 0) {
            return;
        }
        if (180.0f < *(float *)(p + 0x148)) {
            w[0] = *(float *)&D_L11_00162578;
            w[1] = D_L11_0016257C;
            w[2] = D_L11_00162580;
            w[3] = 1.0f;
            func_001F9BF0(w, p + 0x140, w);
            d = func_001F9CB8(w);
            if (d <= 35.0f) {
                func_L08_002F2760(0);
                func_L08_002F2618(D_L11_001FFA18, D_L11_00162640, 0.52f, 0.62f, 0.06f);
                func_L08_002F2760(2);
                if (d < 15.0f) {
                    *(unsigned char *)&D_L11_00162620 = 255;
                } else if (d < 35.0f) {
                    *(unsigned char *)&D_L11_00162620 = func_001FA898_r((1.0f - (d - 15.0f) / 20.0f) * 255.0f);
                } else {
                    *(unsigned char *)&D_L11_00162620 = 0;
                }
            }
            func_L08_002F2838(0);
            func_L08_002F2838(0);
            func_L08_002F2838(1);
            func_001F49B0(func_L11_0031EFC0, moby);
        }
        break;
    }
    }
}
