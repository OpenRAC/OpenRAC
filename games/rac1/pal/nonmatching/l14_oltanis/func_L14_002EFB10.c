/* NON_MATCHING func_L14_002EFB10 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: BYTES 18/552 (96.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns n pairs of particles (func_L00_00272F00) at a position with random velocity/size.
 *   Best p4.c: 18 bytes differ, only schedule: retail places mov.s f13,f22 (the "a" arg of the first 00272F00 call
 *   Unblock: wording that moves the float arg move earlier in that first call.
 */
extern float func_002140F8(float, float);
extern int func_002140B0(int);
extern float func_00214158(void);
extern void func_00215C00(void *, float, float, float);
extern float func_L00_00258C80(float lo, float hi);
extern int func_001F9850(int);
extern unsigned char *func_L00_00272F00(float *pos, int a, int col, int mode, int b, float *vel, float f0, float f1, float f2);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern short D_L14_00161D70;
extern short D_L14_00161D74;
extern short D_L14_00161D78;
extern short D_L14_00161D7C;
extern short D_L14_00161D80;

// Spawns n pairs of particles around a position with random velocity and size.
void func_L14_002EFB10(float *pos, int n) {
    float v[4];
    float p2[4];
    float a, f21, f20;
    int s, r0, i, id;
    unsigned char *r;
    if (n > 0) {
    i = n;
    do {
        a = func_002140F8(*(float *)&D_L14_00161D7C, *(float *)&D_L14_00161D80);
        s = func_002140B0(2);
        if (s == 0) s--;
        f21 = func_00214158();
        f20 = func_002140F8(0.34906584f, 1.5358897f);
        func_00215C00(v, func_002140F8(*(float *)&D_L14_00161D70, *(float *)&D_L14_00161D74), f21, f20);
        qcopy(p2, pos);
        pos[0] += func_L00_00258C80(0.0f, 0.15f);
        pos[1] += func_L00_00258C80(0.0f, 0.15f);
        pos[2] += func_L00_00258C80(0.0f, 0.15f);
        id = func_001F9850(0x3C);
        r = func_L00_00272F00(pos, id, 0x7F7F2020, 0, s, v, a * 0.1f, a, *(float *)&D_L14_00161D78);
        if (r != 0) {
            r[9] = func_001FA898_r(4.0f) + 0x70;
        }
        s = -s;
        r = func_L00_00272F00(pos, func_001F9850(0x3C), 0x7F7F7F7F, 1, s, v, a * 0.07f, a * 0.7f, *(float *)&D_L14_00161D78);
        if (r != 0) {
            r[9] = func_001FA898_r(4.0f) + 0x70;
        }
    } while (--i != 0);
    }
}
