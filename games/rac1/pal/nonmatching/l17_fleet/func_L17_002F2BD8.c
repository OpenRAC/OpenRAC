/* NON_MATCHING func_L17_002F2BD8 -- src/overlays/l17_fleet/vendor_002F1558.c
 * Best so far: SIZE ours 856 / retail 848, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Not exact (p2.c best; three of six runs were lost to my own compile errors). Stack layout matches (Quad4 t fir
 *   Next try (p3.c, unrun): flat float arrays indexed i*3, i*3+1, i*3+2 instead of float[][3]. Sibling func_L15_00
 *   # u01 (l17n3) round: budget spent (6 runs: p3..p8). Not exact; best by size p7.c (840/848), p4.c has the 2nd 4
 *   Found: loop counters i must be `unsigned int` (retail sltiu; p4 made the 2nd loop (DC50/DE090) match exactly).
 *   Left: loop 1 stores out[1] in retail as ($21=12*i running giv) + ($30 = DE70+4 held in a register) with a kept
 *   # v07 (l17n4) round: budget spent (6 runs: p9..p13; p13 == p12 because my sed -i failed on macOS). Not exact; 
 *   Fixed: delay slot at func_001F9878 (load old=data[4] and ang=D_..404 into locals first, then data[0]=old). Out
 *   Left in p12.c: (1) `lui $a0` for D_L17_0015F6B0 hoisted early in ours (declared plain `extern int`; try `MACRO
 */
extern float func_001FA888(int);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern float func_001F9878(float);
extern float func_001FA748(float, float);
extern void func_L00_00251358(void *, void *, void *, void *);
extern float func_001F9B50(float);
extern float func_L00_00200210(float, float);
extern float func_001FA7D8(float x);
extern float func_001F9FA8(float);
extern void func_001F7868(void);
extern void func_L00_001FDE48(int, int, int, void *, int);
extern float D_0015EE6C MACRO_ADDR;
extern int D_L17_0015F6B0;
extern float D_L17_001DDE70[];
extern float D_L17_001DDA30[];
extern int D_L17_001DE1F8[];
extern float D_L17_001DDC50[][2];
extern float D_L17_001DE090[][2];
typedef struct { float f[4]; } Quad4;
extern Quad4 D_L17_00162410;
extern short D_L17_0016240C;
extern short D_L17_001623F8;
extern short D_L17_001623F4;
extern short D_L17_00162404;
extern short D_L17_001623FC;
extern short D_L17_00162400;
extern short D_L17_00162408;
extern short D_L17_001623C0;
extern short D_L17_001623E0;
extern short D_L17_001623E8;
extern short D_L17_001623F0;

// Builds and draws the animated ring of lights around a moby.
void func_L17_002F2BD8(char *moby) {
    unsigned int off = 0;
    char *data = *(char **)(moby + 0x78);
    unsigned int i = 0;
    int j = 0;
    int k;
    int color;
    Quad4 t;
    int c[3];
    float scale;
    float old;
    float ang;
    float *p;
    scale = func_001FA888(D_L17_0015F6B0) * (*(float *)&D_L17_0016240C * D_0015EE6C);
    func_00234C98(6, func_001F4868(*(int *)&D_L17_001623F8));
    func_00234C98(0x42, ((long)*(int *)&D_L17_001623F4 << 32) | 0x44);
    func_00234C98(8, 0);
    func_00234C98(0x14, 0xFF9000000260L);
    old = *(float *)(data + 4);
    ang = *(float *)&D_L17_00162404;
    *(float *)(data + 0) = old;
    *(float *)(data + 4) = func_001FA748(*(float *)(data + 4), 360.0f / func_001F9878(ang) * 0.017453292f * D_0015EE6C);
    func_L00_00251358(moby, &c[0], &c[1], &c[2]);
    p = &t.f[1];
    color = (*(int *)&D_L17_001623FC << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
    do {
        float *s = &D_L17_001DDA30[i * 3];
        float *o = &D_L17_001DDE70[i * 3];
        float x = s[0];
        float y = s[1];
        float r = func_001F9B50(x * x + y * y);
        float a = func_L00_00200210(r, *(float *)&D_L17_00162400);
        float b = func_001FA7D8(a * 6.2831855f / *(float *)&D_L17_00162400);
        float z = func_001F9FA8(func_001FA748(b, *(float *)(data + 4)));
        o[0] = s[0] + *(float *)(moby + 0x10);
        *(float *)((char *)D_L17_001DDE70 + 4 + off) = s[1] + *(float *)(moby + 0x14);
        o[2] = s[2] + *(float *)(moby + 0x18) + *(float *)&D_L17_00162408 * z;
        D_L17_001DE1F8[i] = color;
        i++;
        off += 12;
    } while (i < 45);
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 45; i++) {
            t = D_L17_00162410;
            D_L17_001DE090[i][0] = D_L17_001DDC50[i][0] + func_L00_00200210(scale * t.f[j * 2], 1.0f);
            D_L17_001DE090[i][1] = D_L17_001DDC50[i][1] + func_L00_00200210(scale * p[j * 2], 1.0f);
        }
        func_001F7868();
        for (k = 0; k < 1; k++) {
            func_L00_001FDE48(((int *)&D_L17_001623C0)[k], ((int *)&D_L17_001623E0)[k], ((int *)&D_L17_001623E8)[k], (void *)((int *)&D_L17_001623F0)[k], 1);
        }
    }
}
