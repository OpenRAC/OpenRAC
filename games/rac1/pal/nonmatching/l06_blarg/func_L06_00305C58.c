/* NON_MATCHING func_L06_00305C58 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: BYTES 10/480 (97.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Particle burst: loop count from level global, per iteration random params, calls func_L00_0026B890 with obj.
 *   p1.c best (10 bytes differ): retail reuses $s0 for both func_001F9850 results (0xF and 0x19) and keeps the fir
 *   Remaining: register choice + order of mov.s f12 / daddu t0 before the call.
 */
typedef struct { int v[6]; } V6;
extern void func_001F9BC0(void *);
extern float func_002140F8(float, float);
extern int func_002140B0(int);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026B890(void *, void *, int, int, int, int, int, int, float, float);
extern V6 D_L06_00201D38;
extern V6 D_L06_00201D20;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L06_00162248;
extern short D_L06_00162244;

// Spawns a burst of particles around an object, count from a level global.
void func_L06_00305C58(int unused, void *obj) {
    float pos[4];
    V6 a;
    V6 b;
    int i = 0;
    func_001F9BC0(pos);
    if (*(int *)&D_L06_00162248 > 0) {
        do {
            float s;
            float f;
            int *pa;
            int *pb;
            int r1;
            int r2;
            int r3;
            i++;
            s = func_002140F8(8.0f, 10.0f);
            a = D_L06_00201D20;
            b = D_L06_00201D38;
            s = s * D_0015EE6C;
            pa = a.v + func_002140B0(6);
            pb = b.v + func_002140B0(6);
            f = *(float *)&D_L06_00162244 * 400000.0f;
            r1 = func_001F9850(0xF);
            r1 = func_L00_00258BC8(r1, func_001F9850(0x14));
            r2 = func_001F9850(0x19);
            r3 = func_L00_00258BC8(r2, func_001F9850(0x1E));
            func_L00_0026B890(obj, pos, *pa, *pb, r1, r3, 0, 0, f, s * *(float *)&D_L06_00162244);
        } while (i < *(int *)&D_L06_00162248);
    }
}
