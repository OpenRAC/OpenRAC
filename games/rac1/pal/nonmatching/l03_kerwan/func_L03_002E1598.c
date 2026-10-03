/* NON_MATCHING func_L03_002E1598 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 516 / retail 524, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1548: state 0 arms the moby; state 1 picks an object by a mode value, then fires two spark bursts a
 *   Only the first block differs: retail computes the object index with a branch (bne a,4; i=0 in delay; i=2) and 
 *   Tried local t, direct field reads, ternary, i at function scope: all movn. Would need a way to stop the compil
 */
extern void func_L00_00264870(int);
extern int func_001F9850(int);
extern void func_L00_00250800(void *, int, void *);
extern float func_002140F8(float, float);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026DD70(void *, void *, int, int, float, int);
extern int D_L03_0015F6A8 MACRO_ADDR;
typedef int u128 __attribute__((mode(TI)));
struct L3G {
    char pad0[0x30];
    int a;
    int b;
    char pad1[0x140];
    char *objs[4];
};
extern struct L3G D_L03_0016C9E0;

/* Updates a trigger moby: arms on spawn, then fires sparks at random times. */
void func_L03_002E1598(char *m) {
    switch ((unsigned char)m[0x20]) {
    case 0:
        m[0x20] = 1;
        m[0x30] = 0xFF;
        break;
    case 1:
        if (D_L03_0015F6A8 == 2) {
            int t = D_L03_0016C9E0.a;
            if (t == 4 || t == 10) {
                func_L00_00264870((int)D_L03_0016C9E0.objs[t == 4 ? 2 : 0]);
            }
            if (D_L03_0015F6A8 == 2) {
                if (D_L03_0016C9E0.a == 3) {
                    if (D_L03_0016C9E0.b >= func_001F9850(900) && D_L03_0016C9E0.b <= func_001F9850(10000)) {
                        char *o = D_L03_0016C9E0.objs[3];
                        if (o) {
                            float a[4];
                            float b[4];
                            float c[4];
                            *(u128 *)c = 0;
                            func_L00_00250800(o, 0, a);
                            func_L00_00250800(o, 1, b);
                            func_L00_0026DD70(a, c, 0x7F7F7F7F, 0x7F7F7F, func_002140F8(0.25f, 0.5f) * 210000.0f, func_001F9850(func_L00_00258BC8(0x2D, 0x3C)));
                            func_L00_0026DD70(b, c, 0x407F7F7F, 0x7F7F7F, func_002140F8(0.25f, 0.5f) * 210000.0f, func_001F9850(func_L00_00258BC8(0x2D, 0x3C)));
                        }
                    }
                }
            }
        }
        break;
    }
}
