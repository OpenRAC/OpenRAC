/* NON_MATCHING func_L06_002F5560 -- src/overlays/l06_blarg/vendor_002B5990.c
 * Best so far: SIZE ours 600 / retail 608, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns 4 rings of sparks around a moby: builds a param struct (func_001F9BC0), then per pass computes two angl
 *   and two vectors, calls func_001F9BD8 twice and func_L00_001EFFF0. Best p4.c (608 vs 600 bytes): registers and 
 *   calls match; remaining diffs are prologue/save scheduling (when `addiu s6,3` and the swc1 saves sit), the
 *   f0/f1 choice for the D_L06_00161D80 and d[0x80] loads (retail loads d[0x80] into $f1 first), and one nop.
 *   Pure scheduler order; no source wording found to move it.
 */
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001FA790(float, float);
extern void func_001F9BC0(void *);
extern float func_001F9FA8(float);
extern float func_001F9F90(float);
extern float func_001FA748(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern short D_L06_00161D7C;
extern short D_L06_00161D80;
extern char D_0013E633[];
typedef struct {
    int pad0;
    int pad4;
    float f8;
    float fC;
    void *m;
    int flags;
    char b18;
    char b19;
    unsigned short h1A;
    float f1C;
    int i20;
} Sx;
typedef struct { float x, y, z, w; } __attribute__((aligned(16))) Vx;

// Spawns four rings of sparks around a moby, sweeping the angle each pass.
void func_L06_002F5560(char *m) {
    Sx s;
    Vx a;
    Vx b;
    int i;
    char *pos = m + 0x10;
    int one = 1;
    char *d;
    int n;
    float f25, f27, cur, prev, ang, ra, rb;
    n = func_001FA898_r(*(float *)&D_L06_00161D80 / (*(float *)&D_L06_00161D7C * D_0015EE6C) * 0.25f);
    d = *(char **)(m + 0x78);
    cur = *(float *)&D_L06_00161D80;
    f25 = *(float *)(d + 0x80) * (float)n;
    f27 = cur * 0.25f;
    ang = func_001FA790(*(float *)(d + 0x7C), f25 * 4.0f);
    s.m = m;
    s.f1C = 1.0f;
    s.flags = *(int *)(D_0013E633 + 0x1F75) != 6 ? 0x10001 : 0x10000;
    s.i20 = one;
    func_001F9BC0(&s);
    s.f8 = 1.0f;
    s.fC = 5627.9248f;
    s.b18 = 3;
    s.b19 = one;
    s.h1A = *(unsigned short *)(m + 0xA6);
    for (i = 3; i >= 0; i--) {
        prev = cur;
        ra = func_001FA748(*(float *)(d + 0x84), *(float *)(d + 0x64) * 0.017453292f * func_001F9FA8(ang));
        cur -= f27;
        ang = func_001FA748(ang, f25);
        rb = func_001FA748(*(float *)(d + 0x84), *(float *)(d + 0x64) * 0.017453292f * func_001F9FA8(ang));
        a.x = func_001F9F90(ra) * prev;
        a.y = func_001F9FA8(ra) * prev;
        a.z = 0.0f;
        b.x = func_001F9F90(rb) * cur;
        b.y = func_001F9FA8(rb) * cur;
        b.z = 0.0f;
        func_001F9BD8(&a, &a, pos);
        func_001F9BD8(&b, &b, pos);
        func_L00_001EFFF0(&a, &b, 0, m, &s);
    }
}
