/* NON_MATCHING func_L00_00260460 -- src/overlays/shared/mobyutil_00258BC8.c
 * Best so far: BYTES 15/836 (98.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns three bursts of coloured sparks (func_L00_0026B890) around a point, then two glow pieces (func_L00_002A
 *   Remaining diff: in the second 002ADBB0 call, t3/a2/a3 move order (lines 0x210-0x224), and the two li constants
 *   Would unblock: a spelling of the second r[] initialisation that swaps $v0/$v1 (an allocator tie); not found in
 */
typedef struct { float x, y, z, w; } Vy10 __attribute__((aligned(16)));
typedef struct { int v[6]; } S6y10;
extern S6y10 D_L00_001E93B0;
extern S6y10 D_L00_001E93C8;
extern float D_L00_001B0670[] NOT_SDA;
extern float D_0015EE6C MACRO_ADDR;
extern void func_001F9BC0(void *);
extern float func_002140F8(float, float);
extern int func_002140B0(int);
extern unsigned func_L00_0025D140(unsigned, int);
extern void func_L00_0025D0E0(int *, int *, int *, int);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026B890(void *, void *, int, int, float, int, int, int, int, float);
extern void func_L00_002ADBB0(void *, void *, void *, float, int, int, int, int, int);
extern void func_L00_002D4CE8(void *, void *, int, int);

// Spawn three bursts of coloured sparks around a point, then optional glow pieces and a flash.
void func_L00_00260460(char *a, char *b, float sc, float fl) {
    Vy10 m;
    S6y10 c0;
    S6y10 c1;
    int r[3];
    int i;
    float sp, big, t1, t2;
    func_001F9BC0(&m);
    for (i = 2; i >= 0; i--) {
        sp = func_002140F8(8.0f, 10.0f) * D_0015EE6C;
        big = sc * 500000.0f;
        c0 = D_L00_001E93B0;
        c1 = D_L00_001E93C8;
        func_L00_0026B890(b, &m, func_L00_0025D140(c0.v[func_002140B0(6)], 1), func_L00_0025D140(c1.v[func_002140B0(6)], 1),
            big, func_L00_00258BC8(func_001F9850(0xF), func_001F9850(0x14)),
            func_L00_00258BC8(func_001F9850(0x19), func_001F9850(0x1E)), 0, 0, sp * sc);
    }
    if (a != 0) {
        r[0] = 0x7F; r[1] = 0x40; r[2] = 0;
        func_L00_0025D0E0(&r[0], &r[1], &r[2], 1);
        t1 = sc * 4.0f;
        func_L00_002ADBB0(a, b, &m, t1, func_001F9850(0x14), *(unsigned char *)&r[0], *(unsigned char *)&r[1], *(unsigned char *)&r[2], 0x30);
        r[0] = 0x60; r[1] = 0x20; r[2] = 0;
        func_L00_0025D0E0(&r[0], &r[1], &r[2], 1);
        t2 = sc * 3.0f;
        func_L00_002ADBB0(a, b, &m, t2, func_001F9850(0x1D), *(unsigned char *)&r[0], *(unsigned char *)&r[1], *(unsigned char *)&r[2], 0x20);
    }
    if (fl != 0.0f) {
        if (fl > 0.0f) {
            D_L00_001B0670[9] = fl;
            D_L00_001B0670[10] = fl;
            D_L00_001B0670[8] = fl;
        } else {
            D_L00_001B0670[9] = 13.0f;
            D_L00_001B0670[10] = 13.0f;
            D_L00_001B0670[8] = 13.0f;
        }
        func_L00_002D4CE8(D_L00_001B0670, b, 0, 0);
    }
}
