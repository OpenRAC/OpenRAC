/* NON_MATCHING func_L13_002EAC18 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 612 / retail 616, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Initialises a path-following moby (init of state 7): clears flags, loads first path segment point/heading from
 *   D_L13_001B0AB0 table, sets bitflags on the linked moby, calls func_L10_002F6E10/func_L13_002E9B30, resets mess
 *   Best p1.c/p2.c (604 vs 616 bytes). Left: retail keeps seg=p+0xB0 ($s2) set in the prologue before the $ra save
 *   `sh $zero,0x9C` into the jal delay slot after the FF4B0 arg setup (ours puts the sh/sb stores early; swapping 
 *   not move them), and reloads D_L13_0016016C/the 0x134 index per statement where ours shares them. p3.c (separat
 *   Unblock: the store/arg scheduling around func_L00_001FF4B0 and the late lw of D_L13_00160058.
 *   Round q30/w04: p6.c is the best (612 vs 616): ta = t + i*16 as a pointer local, offsets folded as t + (i*16 + 
 */
extern void func_L00_002584A8(void *, int, int);
extern void func_L00_0025E4B0(void *m, short *p);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(void *, void *);
extern void func_L10_002F6E10(int);
extern int func_001F9850(int);
extern void func_L13_002E9B30(void *, void *, int, int);
extern void func_001FFDA0(int arg0, int arg1);
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L13_0016016C;
extern char *D_L13_001B0AB0[];
extern char *D_L13_00160058 MACRO_ADDR;

// Initialises a path-following moby: loads its first path segment, aims it and resets its state.
void func_L13_002EAC18(char *m, unsigned char *p, int *q) {
    int *seg = (int *)(p + 0xB0);
    char **tbl;
    *q = 0;
    *(unsigned short *)(m + 0x34) = *(unsigned short *)(m + 0x34) & 0xEFFF;
    func_L00_002584A8(m, 0, -1);
    p[0x67] = 0x78;
    func_L00_0025E4B0(m, (short *)(p + 0x60));
    p[0x13B] = 4;
    *(short *)(p + 0x9C) = 0;
    func_L00_001FF4B0(p + 0x70, D_L13_0016016C + (*(int *)(p + 0x130) << 7), D_0015EE6C * 10.0f);
    tbl = D_L13_001B0AB0;
    {
        char *t = tbl[seg[p[0x13B]]];
        int i = *(short *)(p + 0x9C);
        char *ta = t + i * 16;
        char *tb = t + (i + 1) * 16;
        qcopy(m + 0x10, ta + 0x10);
        *(float *)(m + 0x48) = func_L00_001FF860(*(float *)(tb + 0x10) - *(float *)(ta + 0x10), *(float *)(tb + 0x14) - *(float *)(ta + 0x14));
    }
    {
        char *t = tbl[seg[p[0x13B]]];
        int i = *(short *)(p + 0x9C);
        float dd = func_001F9D48(t + (i * 16 + 0x10), t + (i * 16 + 0x20));
        char *t2 = tbl[seg[p[0x13B]]];
        int i2 = *(short *)(p + 0x9C);
        char *ua = t2 + i2 * 16;
        char *ub = t2 + (i2 + 1) * 16;
        *(float *)(m + 0x44) = -func_L00_001FF860(dd, *(float *)(ub + 0x18) - *(float *)(ua + 0x18));
    }
    p[0x10C] = 1;
    {
        char *g = D_L13_00160058;
        *(unsigned short *)(g + (*(int *)(p + 0x134) << 8) + 0x34) |= 2;
        *(unsigned short *)(g + (*(int *)(p + 0x134) << 8) + 0x34) |= 1;
        g[(*(int *)(p + 0x134) << 8) + 0x31] = 0;
    }
    func_L10_002F6E10(*(int *)(p + 0x12C));
    m[0x20] = 7;
    *(float *)(p + 0xAC) = D_0015EE6C * 80.0f;
    *(int *)(p + 0xF8) = func_001F9850(0x258);
    func_L13_002E9B30(m, p, 0x22, 1);
    *(short *)(p + 0x138) = 0;
    p[0x148] = 0;
    if (*(int *)(p + 0x104) != -1) {
        func_001FFDA0(*(int *)(p + 0x104), 0);
        *(int *)(p + 0x104) = -1;
    }
    *(int *)(p + 0x13C) = -1;
}
