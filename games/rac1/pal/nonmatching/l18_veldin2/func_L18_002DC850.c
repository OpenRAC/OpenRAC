/* NON_MATCHING func_L18_002DC850 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 8/1236 (99.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1
 *   UpdateMoby_624 (veldin2 particle/blob): state byte at moby+0x20 switch (0 spawn particles via 00219780 in a lo
 *   Mattered: pos (moby+0x10) as block-scoped local in case 0 (function-scope hoists it); `cc` declared before `po
 *   Remaining: retail puts `sw pos,0x54($sp)` in the delay slot of the loop-entry blez (after daddu s7,0); ours em
 */
extern void func_001F9C30(void *, void *, float);
extern void func_L00_00258DB0(float *, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BC0(void *);
extern float func_002140F8(float, float);
extern int func_001FA8A8(int, int, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern float func_00214358(void *, int, float);
extern int func_001F9850(int);
extern void func_0020D678(void *);
extern void func_L00_0025B040(unsigned char *m, float s);
extern int func_001F9908(int *);
extern int func_L00_0028EF68(int i, int a1, int v, int k);
extern void func_L18_002DD270(char *moby);
extern void func_L18_002DCE10(void);
extern void func_001F49B0(void (*)(void), void *);
extern int D_L18_0015F6A8 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern char D_0013E633[];
extern short D_L18_00161C8C;
extern short D_L18_00161C90;
extern short D_L18_00161C94;
extern short D_L18_00161C98;
extern short D_L18_00161C9C;
extern short D_L18_00161CA0;
extern short D_L18_00161CA4;
extern short D_L18_00161CA8;
extern short D_L18_00161CAC;
extern short D_L18_00161CB0;
extern short D_L18_00161CB4;
extern short D_L18_00161CB8;
extern short D_L18_00161CBC;
extern short D_L18_00161CC0;
extern short D_L18_00161CC4;
extern short D_L18_00161CC8;
extern short D_L18_00161CCC;
extern short D_L18_00161CD0;
extern short D_L18_00161CD4;

void func_L18_002DC850(char *moby) {
    float v[4];
    float a[4];
    float c[4];
    float w[4];
    float *d = *(float **)(moby + 0x78);
    int i;
    int i0, i1, i2, i3, i4;

    if (*(int *)(D_0013E633 + 0x2EA1) == 0x72 || D_L18_0015F6A8 == 2) {
        func_0020D678(moby);
        return;
    }
    switch (((unsigned char *)moby)[0x20]) {
    case 0: {
        float *cc;
        char *pos;
        i = 0;
        pos = moby + 0x10;
        for (; i < *(int *)&D_L18_00161CD0; i++) {
            cc = c;
            func_001F9C30(v, d, *(float *)&D_L18_00161C98);
            func_L00_00258DB0(cc, 0.0f, *(float *)&D_L18_00161C9C * D_0015EE6C);
            func_001F9BD8(v, cc, v);
            func_L00_00258DB0(w, 0.0f, 0.5f);
            func_001F9BD8(w, w, pos);
            func_001F9BC0(a);
            v[3] = func_002140F8(*(float *)&D_L18_00161CB0, *(float *)&D_L18_00161CB4);
            a[3] = func_002140F8(*(float *)&D_L18_00161CB8, *(float *)&D_L18_00161CBC);
            i0 = func_001FA8A8(*(int *)&D_L18_00161CC0, *(int *)&D_L18_00161CC4, func_002140F8(0.0f, 1.0f));
            i1 = func_001FA8A8(*(int *)&D_L18_00161CC8, *(int *)&D_L18_00161CCC, func_002140F8(0.0f, 1.0f));
            i2 = func_001FA898_r(func_001F9878((float)*(int *)&D_L18_00161CA0 * func_002140F8(0.0f, 1.0f) + 1.0f));
            i3 = func_001FA898_r(func_001F9878((float)*(int *)&D_L18_00161CA4 * (func_002140F8(-*(float *)&D_L18_00161CAC, *(float *)&D_L18_00161CAC) + 1.0f)));
            i4 = func_001FA898_r(func_001F9878((float)*(int *)&D_L18_00161CA8 * (func_002140F8(-*(float *)&D_L18_00161CAC, *(float *)&D_L18_00161CAC) + 1.0f)));
            a[2] = a[2] - *(float *)&D_L18_00161C94 * D_0015EE70 * (float)i3;
            func_00219780(w, v, a, i0, i1, i2, i3, i4, *(int *)&D_L18_00161CD4);
        }
        d[12] = func_001FA748(d[12], D_0015EE6C * 18.849556f);
        {
            int col = func_001FA8A8(*(int *)&D_L18_00161C8C, *(int *)&D_L18_00161C90, (func_001F9FA8(d[12]) + 1.0f) * 0.5f);
            float k = D_0015EE70 * 10.0f;
            *(int *)(moby + 0x90) = col;
            d[2] = d[2] - k;
        }
        func_001F9BD8(pos, pos, d);
        {
            float t = func_00214358(pos, 0, 0.5f);
            if (*(float *)(moby + 0x18) - 0.333f < t) {
                ((unsigned char *)moby)[0x20] = 1;
                *(float *)(moby + 0x18) = t + 0.333f;
                *(int *)(d + 11) = func_001F9850(1);
            }
        }
        *(float *)(moby + 0x44) = func_001FA748(*(float *)(moby + 0x44), D_0015EE6C * 8.726646f);
        if (*(float *)(moby + 0x18) < 10.0f) {
            func_0020D678(moby);
        } else {
            func_L00_0025B040((unsigned char *)moby, 1.0f);
        }
        break;
    }
    case 1:
        d[12] = func_001FA748(d[12], D_0015EE6C * 18.849556f);
        *(int *)(moby + 0x90) = func_001FA8A8(*(int *)&D_L18_00161C8C, *(int *)&D_L18_00161C90, (func_001F9FA8(d[12]) + 1.0f) * 0.5f);
        if (func_001F9908((int *)(d + 11))) {
            func_L00_0028EF68(0xF, 0, (int)moby, 0x58E);
            ((unsigned char *)moby)[0x20] = 2;
            d[9] = 0.5f;
            *(int *)(moby + 0x94) = 0;
            ((unsigned char *)moby)[0x31] = 0;
            *(unsigned short *)(moby + 0x34) |= 1;
        }
        break;
    case 2:
        d[9] = d[9] + d[8];
        func_L18_002DD270(moby);
        func_001F49B0(func_L18_002DCE10, moby);
        if (d[9] > d[10]) {
            ((unsigned char *)moby)[0x20] = 3;
        }
        break;
    case 3:
        func_0020D678(moby);
        break;
    }
}
