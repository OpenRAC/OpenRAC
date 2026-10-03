/* NON_MATCHING func_L17_002F1558 -- src/overlays/l17_fleet/vendor_002F1558.c
 * Best so far: SIZE ours 760 / retail 764, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fleet enemy/turret state update (arg a1 = skip flag): rebuilds the +0x1D4 timer, queries func_L00_0025B4D0, on
 *   Best is p3.c (768 vs 764 bytes; prologue and control flow right, 6 runs spent). Two differences left:
 *   1. In the hit block retail loads y (sp+0x14) first (it sits in the beq delay slot), then D_0015EE70, data+0x20
 *   2. The tail extra 4 bytes: ours stores the second func_L00_001FF860 result to ang (sp+0x18, address taken for 
 *   Next worker: use a separate float local in the tail (no memory spill), then try more orders of the D_0015EE70/
 *   u02 (l17n3, r0-r3, 6 runs): r0 = p3.c failed to compile (p3's own `typedef ... u128` clashes with the file's l
 *   Left (r3, same size, 95/764 bytes): only the schedule of the hit block at +0xdc..+0x160. Retail: lui/lwc1 D_00
 *   v10 (l17n4, s0-s5, 6 runs): best is s2.c (760 vs 764; the packet's best.c is older than r3.c, s2 = r3 with `t 
 */
typedef int q128 __attribute__((mode(TI)));
extern char D_0013E633[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L08_00211A38(float *);
extern char *func_L00_0025B478(void *, int, int);
extern int func_L00_0025B4D0(void *, void *, void *, int, void *, void *, int, int);
extern void func_L00_0025BBA0(void *, float *, float *, float *);
extern float func_L00_001FF860(float, float);
extern void func_L00_0025D5B0(float, char *, float *, int, int, int);
extern void func_L00_0025E4B0(void *, short *);
extern int func_L00_00260FB0(float, char *, char *, int, int, int *, int);
extern float func_001F9D10(void *, void *);
extern float func_001FA850(float, float);
extern int func_L00_001EFFF0(void *, void *, int, int, int);

/* Fleet turret/enemy state update: refreshes the hit reaction, fires the aim query and sets the lock flags. */
void func_L17_002F1558(unsigned char *moby, int arg) {
    char *data;
    char *q;
    float t;
    float t0;
    float s0;
    float s1;
    float tmp[4];
    int x;
    float y;
    float ang;
    int r;
    float yaw;
    float ang2;
    data = *(char **)(moby + 0x78);
    if (moby[0x20] == 0) return;
    if (*(int *)(data + 0x38) != 0) {
        *(float *)(data + 0x1D4) = func_001FA898_r(func_001F9878(func_002140F8(180.0f, 240.0f)));
        *(int *)(data + 0x38) = 0;
    }
    func_L08_00211A38((float *)(data + 0x1D4));
    y = 0;
    q = func_L00_0025B478(moby, 0x330000, 0);
    func_L00_0025B4D0(moby, q, data + 0x20, 0, &x, &y, 0, 4);
    if (x != 1 && moby[0x20] != 15) {
        t0 = y;
        s0 = D_0015EE70;
        t = *(float *)(data + 0x20) - t0;
        data[0x15D] = 0;
        s1 = D_0015EE6C;
        *(float *)(data + 0x130) = s0 * 42.0f;
        *(float *)(data + 0x134) = s0 * 24.0f;
        *(int *)(data + 0x144) = 9;
        *(float *)(data + 0x13C) = s1 * 15.7f;
        *(float *)(data + 0x138) = s1 * 19.0f;
        *(float *)(data + 0x20) = t;
        *(float *)(data + 0x170) = -1.0f;
        *(float *)(data + 0x174) = -1.0f;
        *(q128 *)tmp = *(q128 *)(q + 0x10);
        func_L00_0025BBA0(tmp, &ang, (float *)(data + 0x138), (float *)(data + 0x13C));
        if (q != 0 && *(int *)(q + 0x20) != 0 && *(short *)(*(char **)(q + 0x20) + 0xA6) == 0x63) {
            ang = func_L00_001FF860(*(float *)(data + 0x1F0) - *(float *)(moby + 0x10), *(float *)(data + 0x1F4) - *(float *)(moby + 0x14));
        }
        func_L00_0025D5B0(ang, moby, (float *)(data + 0x120), 0xB, 1, 0);
        moby[0x20] = 15;
        *(unsigned char *)(data + 0x117) = 0xF0;
        func_L00_0025E4B0(moby, (short *)(data + 0x110));
    }
    moby[0xA4] = 0xFF;
    if (arg != 0) return;
    if (func_L00_00260FB0(128.0f, moby, data + 0x180, 0, 0, 0, 0) == 2) return;
    if (func_001F9D10(moby + 0x10, data + 0x1B0) < 64.0f) {
        ang2 = func_L00_001FF860(*(float *)(data + 0x180) - *(float *)(moby + 0x10), *(float *)(data + 0x184) - *(float *)(moby + 0x14));
        yaw = func_001FA850(*(float *)(moby + 0x48), ang2);
        qcopy(tmp, moby + 0x10);
        tmp[2] += 1.0f;
        r = func_L00_001EFFF0(tmp, D_0013E633 + 0xEED, 2, 0, 0) == 0;
        data[0x1E4] = r;
        if (yaw < 1.0471976f) data[0x1E4] = r << 1;
    }
}
