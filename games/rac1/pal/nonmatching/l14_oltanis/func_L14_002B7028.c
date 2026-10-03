/* NON_MATCHING func_L14_002B7028 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 560 / retail 568, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Emits two particle bursts (func_L00_0026DEA0) around a moby: 2 randomized-sign, then 3 with doubling ids and d
 *   Best p1.c: prologue/loops right except register allocation: ours hoists lui %hi(D_L14_0015F660) into a saved r
 *   Unblock: unrun p5.c = p4.c + MACRO_ADDR on the string array (stops the hoist, in p2 it did lui per call).
 */
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_001F9850(int);
extern char D_L14_0015F660[] MACRO_ADDR;

// Emits two bursts of trail particles around a moby, one fixed and one randomized.
void func_L14_002B7028(char *m) {
    float v[4];
    float a[4];
    char *pos;
    char *r;
    char *q;
    int i, s, ev, k;
    float f;
    if (*(unsigned char *)(m + 0x20) >= 2 && *(unsigned char *)(m + 0x20) != 9) {
        func_001F9C30(v, m + 0xC0, -1.25f);
        func_001F9BD8(a, m + 0x10, v);
        for (i = 1; i >= 0; i--) {
            s = func_002140B0(0x10);
            ev = s;
            if (func_002140B0(2) != 0) ev = -s;
            r = func_L00_0026DEA0(a, ev, D_L14_0015F660, 0x7F3030FF, 0.2f, 1.0f, 0.75f, 160000.0f);
            if (r != 0) {
                *(short *)(r + 0xA) = func_001F9850(6);
                q = r + 0x20;
                *(int *)(q + 4) = 2;
                q[0xA] = 0x7F;
                q[0xB] = r[0xA];
            }
        }
        f = 100000.0f;
        func_001F9C30(v, m + 0xC0, -1.0f);
        func_001F9BD8(a, m + 0x10, v);
        ev = 0x10;
        k = func_001F9850(2);
        for (i = 2; i >= 0; i--) {
            r = func_L00_0026DEA0(a, ev, D_L14_0015F660, 0x7FFFFFFF, 0.05f, 1.0f, 1.0f, f);
            if (r != 0) {
                *(short *)(r + 0xA) = k;
                r[8] = func_002140B0(0xFF);
                q = r + 0x20;
                *(int *)(q + 4) = 2;
                q[0xA] = 0x7F;
                q[0xB] = r[0xA];
            }
            ev = -ev;
            k = k << 1;
            f -= 20000.0f;
        }
    }
}
