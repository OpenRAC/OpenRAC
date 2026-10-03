/* NON_MATCHING func_L05_0030D230 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: BYTES 23/448 (94.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Swaying moby update (state 0 init random angles, state 1 ease + rotate). Best p3.c: 23 bytes differ.
 *   Only the scheduling of the second func_001FA748 argument setup differs (store 0x60 and f1/f0 for D_0015EE6C lo
 */
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_00214158(void);
extern int func_001F9908(int *arg0);
extern void func_0022ED80(int, int, int);
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_002617B0(char *, void *, void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern short D_L05_00161D94;
extern short D_L05_00161D98;
extern short D_L05_00161D9C;
extern short D_L05_00161DA0;
extern short D_L05_00161DA4;
extern short D_L05_00161DA8;


// Update function for a swaying moby: picks a random angle, then eases and rotates it.
void func_L05_0030D230(unsigned char *moby) {
    char *data = *(char **)(moby + 0x78);
    float v[4];
    float w[4];
    float d[4];
    char *p;
    char *q;
    float *r;
    float k;
    switch (moby[0x20]) {
    case 0:
        *(int *)(data + 0x78) = func_001FA898_r(func_001F9878(func_002140F8((float)*(int *)&D_L05_00161DA4, (float)*(int *)&D_L05_00161DA8)));
        *(float *)(data + 0x60) = func_00214158();
        *(float *)(data + 0x6C) = func_00214158();
        moby[0x20] = 1;
        break;
    case 1:
        if (func_001F9908((int *)(data + 0x78))) {
            func_0022ED80(0, 0, (int)moby);
            *(int *)(data + 0x78) = func_001FA898_r(func_001F9878(func_002140F8((float)*(int *)&D_L05_00161DA4, (float)*(int *)&D_L05_00161DA8)));
        }
        p = (char *)moby + 0x10;
        qcopy(v, p);
        r = w;
        q = (char *)moby + 0x40;
        qcopy(r, q);
        *(float *)(data + 0x60) = func_001FA748(*(float *)(data + 0x60), *(float *)&D_L05_00161D94 * 0.017453292f * D_0015EE6C);
        k = *(float *)&D_L05_00161D98 * 0.017453292f * D_0015EE6C;
        *(float *)(data + 0x6C) = func_001FA748(*(float *)(data + 0x6C), k);
        *(float *)(moby + 0x44) = *(float *)&D_L05_00161D9C * 0.017453292f * func_001F9FA8(*(float *)(data + 0x60));
        *(float *)(moby + 0x40) = *(float *)&D_L05_00161DA0 * 0.017453292f * func_001F9FA8(*(float *)(data + 0x6C));
        func_001F9BF0(d, p, v);
        func_L00_002617B0(data + 0x20, d, r, q);
        break;
    }
}
