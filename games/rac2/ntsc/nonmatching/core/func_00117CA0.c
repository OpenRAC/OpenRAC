/* GPT-6.1 Sol (high effort), USA v1.01: 1556/1556 bytes, 0/1 places.
 * Final check.py: instruction at +0x4 differs; 81/389 instruction words differ
 * when relocation fields are excluded. No linked build was run.
 * Reconstructed natural-logarithm range reduction and polynomial.
 * Required profile: -O2 -G2 -ffunction-sections -mastra-quad-saves.
 *
 * Improvement over the earlier draft: explicit calls to the six real software-
 * double helpers resolve their symbols; declarations alone do not redirect
 * compiler-generated libcalls with the game compiler. The half-square product
 * also preserves retail's operand order. All required declarations are below.
 *
 * Remaining: save scheduling, input union copies, normalization scheduling,
 * a missing nop at +0x74, and persistent register allocation (f/dk/i use
 * s4/s5/s6 rather than retail's s5/s6/s4). The frame and branch layout otherwise
 * agree after the early normalization blocks.
 *
 * Measured here: struct-to-array word view is byte-identical; a 64-bit union
 * view removes an input copy but gives 1552 bytes; keeping all arithmetic in
 * the union gives 1532 bytes. Disabling sched2 keeps 1556 bytes but moves the
 * entry copy after the saves; disabling sched1 on the 64-bit view gives 1532.
 * -mno-split-addresses gives 1564; -mastra-keep-frame-order keeps 1556 but still
 * interleaves work with the saves. None improves on this retained candidate.
 * Earlier-tier measurements (not repeated): disabling both schedulers gave
 * 1536 bytes; direct pointer word access gave 1516 and an unwanted stack spill.
 * Next useful evidence: establish the original double bit-access idiom and
 * scheduling profile with another matched function from this library pocket.
 * The zero-input constant is D_0013A460; negative inputs return zero in retail.
 */
#include "common.h"

extern double D_0013A460;
extern double dpadd(double, double) __asm__("func_00122A40");
extern double dpsub(double, double) __asm__("func_00122A98");
extern double dpmul(double, double) __asm__("func_00122B00");
extern double dpdiv(double, double) __asm__("func_00122DA8");
extern int dpcmp(double, double) __asm__("func_00123028");
extern double litodp(int) __asm__("func_00123078");

double func_00117CA0(double x) {
    union { double d; struct { int lo, hi; } w; } u;
    double hfsq, f, s, z, R, w, t1, t2, dk;
    int k, hx, i, j;
    unsigned int lx;
    u.d = x;
    hx = u.w.hi;
    lx = u.w.lo;
    k = 0;
    if (hx < 0x00100000) {
        if (((hx & 0x7fffffff) | lx) == 0) return D_0013A460;
        if (hx < 0) return 0.0;
        k -= 54;
        x = dpmul(x, 1.80143985094819840000e+16);
        u.d = x;
        hx = u.w.hi;
    }
    if (hx >= 0x7ff00000) return dpadd(x, x);
    k += (hx >> 20)-1023;
    hx &= 0x000fffff;
    i = (hx+0x95f64)&0x100000;
    u.d = x;
    u.w.hi = hx|(i^0x3ff00000);
    x = u.d;
    k += (i>>20);
    f = dpsub(x, 1.0);
    if ((0x000fffff & (2+hx)) < 3) {
        if (dpcmp(f, 0.0) == 0) {
            if (k == 0) return 0.0;
            dk = litodp(k);
            return dpadd(dpmul(dk, 6.93147180369123816490e-01), dpmul(dk, 1.90821492927058770002e-10));
        }
        R = dpmul(dpmul(f, f), dpsub(0.5, dpmul(f, 0.33333333333333333333)));
        if (k == 0) return dpsub(f, R);
        dk = litodp(k);
        return dpsub(dpmul(dk, 6.93147180369123816490e-01), dpsub(dpsub(R, dpmul(dk, 1.90821492927058770002e-10)), f));
    }
    s = dpdiv(f, dpadd(f, 2.0));
    dk = litodp(k);
    z = dpmul(s, s);
    i = hx-0x6147a;
    w = dpmul(z, z);
    j = 0x6b851-hx;
    t1 = dpmul(w, dpadd(dpmul(w, dpadd(dpmul(w, 1.531383769920937332e-01), 2.222219843214978396e-01)), 3.999999999940941908e-01));
    t2 = dpmul(z, dpadd(dpmul(w, dpadd(dpmul(w, dpadd(dpmul(w, 1.479819860511658591e-01), 1.818357216161805012e-01)), 2.857142874366239149e-01)), 6.666666666666735130e-01));
    i |= j;
    R = dpadd(t2, t1);
    if (i > 0) {
        hfsq = dpmul(f, dpmul(f, 0.5));
        if (k == 0) return dpsub(f, dpsub(hfsq, dpmul(s, dpadd(hfsq, R))));
        return dpsub(dpmul(dk, 6.93147180369123816490e-01), dpsub(dpsub(hfsq, dpadd(dpmul(s, dpadd(hfsq, R)), dpmul(dk, 1.90821492927058770002e-10))), f));
    }
    if (k == 0) return dpsub(f, dpmul(s, dpsub(f, R)));
    return dpsub(dpmul(dk, 6.93147180369123816490e-01), dpsub(dpsub(dpmul(s, dpsub(f, R)), dpmul(dk, 1.90821492927058770002e-10)), f));
}
