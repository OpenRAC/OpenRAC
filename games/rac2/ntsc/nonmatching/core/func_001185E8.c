/* GPT-6.1 Sol (high), 29 October 2026: FAILED, 1472/1496 bytes,
 * 0/1 USA v1.01 places. Standalone game unit profile:
 * -O2 -G2 -ffunction-sections -mastra-quad-saves.
 * Corrected the older draft's arithmetic against retail: the fraction digit
 * multiplier remains 10.0; only eps grows each iteration. The rounding limit
 * remains len, with len + 1 passed to func_00118460. Keep the original ndigits
 * for decimal-point handling and a separate precision for the small-value case.
 * The small-value loop decrements e once per multiplication, including exit.
 * Reusing digit in the reversal and n for the signed special-value switch
 * improves lifetimes. Same-size loop forms still differ in register allocation,
 * entry save/extraction scheduling and loop rotation. Character stays in $fp
 * here; retail spills it and keeps the invariant 10.0 in $s7. The integer
 * extraction loop also loses retail's explicit top bound check after GCC
 * rotates it. No flags tested repaired this: no-sched1 gives equal size only,
 * no-rerun-cse-after-loop and no-gcse still leave broad differences.
 * Next useful experiment: preserve the digit-loop invariant's lifetime while
 * spilling the format character, without artificial dependencies or volatility.
 * Reconstructed from retail; this is the SDK vfprintf object's static-buffer
 * conversion helper, not the public newlib cvt. All declarations follow.
 */
#include "common.h"

extern char D_0013C080[];

extern char D_0013A490[];
extern char D_0013A498[];
extern char D_0013A4A0[];
extern char D_0013A4A8[];

extern int func_00118460(char *buf, int len);

extern char *func_00118548(int value, char *buf, unsigned int base);

extern double func_00117650(double);

extern double func_00117848(double, double);

char *func_001185E8(double value, int ndigits, char ch, int dot) {
    union {
        double d;
        int i[2];
    } u;
    char *p;
    char *q;
    char c;
    double w;
    double f;
    double eps;
    int e;
    int n;
    int i;
    int j;
    int digit;
    int len;
    int precision;

    u.d = value;
    n = u.i[0] & ~0xFFFF;
    switch (n) {
    case 0x7FF00000:
        return D_0013A490;
    case 0xFFF00000:
        return D_0013A498;
    case 0x7FF80000:
    case 0x7FFC0000:
    case 0xFFFC0000:
        return D_0013A4A0;
    case 0xFFF80000:
        return D_0013A4A8;
    }

    precision = ndigits;
    e = 0;
    n = 0;
    if (value < 0) {
        D_0013C080[0] = '-';
        p = D_0013C080 + 1;
        value = -value;
    } else {
        p = D_0013C080;
    }

    if (value >= 1.0) {
        f = func_00117650(value);
        value = value - f;
        for (; ++e < 163;) {
            digit = (int)func_00117848(f, 10.0);
            p[n] = digit + '0';
            n++;
            f = (f - (double)digit) * 0.1;
            if (f < 1.0)
                break;
        }
        j = n - 1;
        digit = 0;
        while (digit < j) {
            c = p[digit];
            p[digit] = p[j];
            p[j] = c;
            digit++;
            j--;
        }
        e = n;
    }

    if (e == 0) {
        if (ch != 'f') {
            if (value != 0) {
                do {
                    value = value * 10.0;
                } while (value < 1.0 && e >= -1020 && (--e, 1));
                e--;
            }
            if (value >= 1.0) {
                value = value * 0.1;
                precision = precision + 1;
            }
        } else {
            p[0] = '0';
            n = 1;
        }
    }

    if (e < 0) {
        len = precision;
    } else if (ch != 'f') {
        len = precision + 1;
    } else {
        len = precision + (e > 0 ? e : 1);
    }

    eps = 5.5511151231257827e-17;
    w = 10.0;
    fraction_digits:
    value *= w;
    digit = (int)value;
    value -= (double)digit;
    eps *= w;
    if (n < len && value >= eps && value <= 1.0 - eps) {
        p[n++] = digit + '0';
        goto fraction_digits;
    }
    if (value >= 0.5)
        digit = digit + 1;
    p[n] = digit + '0';
    n++;
    while (n <= len) {
        p[n] = '0';
        n++;
    }
    if (func_00118460(p, len + 1) == 0) {
        p[0] = '1';
        e = e + 1;
        len = len + 1;
    }
    if (ndigits == 0) {
        len = len - 1;
    } else {
        i = 1;
        if (ch == 'f' && e > 0)
            i = e;
        if (i < len) {
            q = p + len;
            j = len;
            do {
                j--;
                *q = q[-1];
                q--;
            } while (i < j);
        }
        p[i] = '.';
    }
    if (dot == 0)
        goto skip;
    while (len != 0 && p[len] == '0') len--;
    if (p[len] == '.') len--;
skip:
    if (ch == 'f') {
        p[len + 1] = 0;
    } else {
        p[++len] = ch;
        if (e >= 0) {
            if (e != 0) e--;
            p[++len] = '+';
            if (e < 10) p[++len] = '0';
        } else {
            p[++len] = '-';
            if (e > -10) p[++len] = '0';
            e = -e;
        }
        func_00118548(e, p + ++len, 10);
    }
    return D_0013C080;
}
