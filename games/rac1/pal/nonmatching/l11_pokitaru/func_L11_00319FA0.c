/* NON_MATCHING func_L11_00319FA0 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: BYTES 20/480 (95.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Update for a moby (states 1: ease two values; 2: fly to target, burst via func_L00_0025F4A8). Best p6.c: 20 by
 *   Only the scheduling of `lw $a2,0x24(data)` vs the 0.333f constant load (lui/ori/mtc1) before func_L00_001F2BE8
 */
extern void func_L11_0031A498(char *moby);
extern int func_001F9908_i(void *) __asm__("func_001F9908");
extern int func_001F9850(int);
extern float func_00214D28(float *p, float target, float maxstep);
extern void func_0020D678(void *);
extern void func_L11_0031A630(char *moby);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern int func_L00_001F2BE8(void *, int, void *, void *, float);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern short D_L11_00162418;

// Update function for a moby that eases values, then flies to a target and bursts.
void func_L11_00319FA0(unsigned char *moby) {
    char *data = *(char **)(moby + 0x78);
    float v[4];
    float *vp;
    func_L11_0031A498((char *)moby);
    switch (moby[0x20]) {
    case 1:
        if (func_001F9908_i(data + 0x20)) {
            float *p = *(float **)(moby + 0x24);
            float n = (float)func_001F9850(0x14);
            float a = p[9];
            float b = (*(float **)(moby + 0x24))[9] / n;
            p = (float *)(data + 0x1C);
            func_00214D28((float *)(moby + 0x2C), a, b);
            func_00214D28(p, *(float *)&D_L11_00162418, *(float *)&D_L11_00162418 / (float)func_001F9850(0x14));
        }
        if (func_001F9908_i(data + 0x28)) {
            func_0020D678(moby);
        }
        break;
    case 2: {
        char *pos = (char *)moby + 0x10;
        func_L11_0031A630((char *)moby);
        vp = v;
        qcopy(vp, pos);
        func_001F9BD8(pos, pos, data);
        if (func_001F9908_i(data + 0x20) ||
            func_L00_001EFFF0(vp, (char *)moby + 0x10, 0, moby, 0) ||
            func_L00_001F2BE8((char *)moby + 0x10, 0, *(void **)(data + 0x24), 0, 0.333f)) {
            func_L00_0025F4A8(moby, data, (char *)moby + 0x10, 1.5f, 2.0f, 10, 3, 16, 4.0f, 2.0f, 9.0f, -1, 1.0f, 15.0f, 1, 1, -1, 0);
            func_0020D678(moby);
        }
        break;
    }
    }
}
