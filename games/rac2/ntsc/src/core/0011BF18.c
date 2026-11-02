#include "common.h"
#include "structs.h"
#include <stdarg.h>

extern void r1_func_00119840(char *);

extern int r1_D_0012FCFC;

extern char r1_D_00154E00[];

extern void func_0011BF18();

extern int r1_func_00119D88();

extern int r1_func_00119D88(int c);

extern void func_0011C7E8(const char *, ...);

/* The print core: walks fmt with the va_list, writing through the hook
   r1_D_0012FD00, and returns the count. */
extern int r1_func_0011A0A0(const char *, va_list);

/* rac1-decomp src/core/00119D88.c: func_00119DC0 */
/* deci2Putchar (libkernl.a:kprintf.o): the kprintf hook used for the
   deci2 debug console. Buffers characters into r1_D_00154E00 (a 0x80-byte
   line buffer) counted by r1_D_0012FCFC, flushing (r1_func_00119840) and
   resetting the count once it reaches 0x7E; on '\n' it also flushes
   immediately (appending the '\n' first) instead of just buffering it. */
void func_0011BF18(int c) {
    int count = r1_D_0012FCFC;

    if (count >= 0x7E) {
        r1_D_0012FCFC = 0;
        r1_D_00154E00[0x7F] = 0;
        r1_func_00119840(r1_D_00154E00);
        count = r1_D_0012FCFC;
    }
    if (c == 0xA) {
        r1_D_0012FCFC = 0;
        r1_D_00154E00[count] = c;
        r1_D_00154E00[count + 1] = 0;
        r1_func_00119840(r1_D_00154E00);
    } else {
        r1_D_0012FCFC = count + 1;
        r1_D_00154E00[count] = c;
    }
}

/* rac1-decomp src/core/00119D88.c: func_00119E70 */
/* serialPutchar (libkernl.a:kprintf.o): the printf hook used when going to
   the serial port. '\n' is expanded to a CR/LF pair (write '\r' then the
   original character); anything else is written as-is via kputchar. */
void func_0011BFC8(int c) {
    if (c == 0xA) {
        r1_func_00119D88(0xD);
        r1_func_00119D88(0xA);
    } else {
        r1_func_00119D88(c);
    }
}

/* rac1-decomp src/core/00119D88.c: func_00119EA8 (kprintf float helper). */
int func_0011C000(unsigned long bits) {
    long exponent = (long)((bits << 1) >> 53);
    exponent -= 1075;
    if (exponent < -53) return 0;
    if (exponent >= 13) return 9999;
    bits = (bits << 12) >> 12;
    bits |= 1UL << 52;
    if (exponent < 0) {
        exponent = -exponent;
        bits >>= (int)(exponent - 2);
        if ((bits & 3) == 3) bits = (bits >> 2) + 1;
        else bits >>= 2;
    } else {
        bits <<= (int)exponent;
    }
    return bits;
}

/* func_0011C090 is in the dedicated 0011C090.c formatter unit. */

extern s32 DIntr_A0A0(void) __asm__("r1_func_0011D960");
extern s32 EnableInterrupts_A0A0(void) __asm__("r1_func_0011D9A8");
extern void printfloat_A0A0(double) __asm__("r1_func_00119F38");
extern void (*r1_D_0012FD00_A0A0[])(s32) __asm__("r1_D_0012FD00");

/* rac1-decomp src/core/00119D88.c: func_0011C1F8 */
int func_0011C1F8(const char *fmt, va_list ap) {
    char buf[32];
    char *pad;
    char *p;
    s32 mod;
    s32 n;
    unsigned long d;
    u8 ch;
    char second;
    s64 v;
    s32 ie;

    ie = DIntr_A0A0();
    for (; *fmt != 0; fmt++) {
        p = (char *)fmt;
        pad = 0;
        mod = 0;
        if (*fmt == '%') {
        again:
            fmt++;
            switch ((char)(*fmt - '0')) {
            case 0:
                n = fmt[1] - '0';
                second = fmt[2];
                if ((u8)n < 10) {
                    if ((u32)(second - '0') < 10) {
                        n = n * 10 + (second - '0');
                        fmt += 2;
                        if (n >= 32) {
                            n = 31;
                        }
                    } else {
                        fmt += 1;
                    }
                    pad = &buf[31 - n];
                    while (n > 0) {
                        buf[31 - n] = '0';
                        n--;
                    }
                    goto again;
                }
                goto again;
            case 'l' - '0':
                mod = 'l';
                goto again;
            case 'h' - '0':
                mod = 'h';
                goto again;
            case 'o' - '0':
                if (mod == 'l') {
                    v = (ap += 8, *(s64 *)(ap - 8));
                } else if (mod == 'h') {
                    v = (ap += 8, *(u16 *)(ap - 8));
                } else {
                    v = (ap += 8, *(u32 *)(ap - 8));
                }
                p = &buf[31];
                *p = 0;
                if (v == 0) {
                    *--p = '0';
                } else {
                    do {
                        *--p = (v & 7) + '0';
                        v = (u64)v >> 3;
                    } while (v != 0);
                }
                if (pad != 0 && pad < p) {
                    p = pad;
                }
                while (*p != 0) {
                    r1_D_0012FD00_A0A0[0](*p++);
                }
                break;
            case 'x' - '0':
                if (mod == 'l') {
                    v = (ap += 8, *(s64 *)(ap - 8));
                } else if (mod == 'h') {
                    v = (ap += 8, *(u16 *)(ap - 8));
                } else {
                    v = (ap += 8, *(u32 *)(ap - 8));
                }
                p = &buf[31];
                *p = 0;
                if (v == 0) {
                    *--p = '0';
                } else {
                    do {
                        d = v & 0xF;
                        if (d < 10) {
                            *--p = d + '0';
                        } else {
                            *--p = d + 'a' - 10;
                        }
                        v = (u64)v >> 4;
                    } while (v != 0);
                }
                if (pad != 0 && pad < p) {
                    p = pad;
                }
                while (*p != 0) {
                    r1_D_0012FD00_A0A0[0](*p++);
                }
                break;
            case 'd' - '0':
                if (mod == 'l') {
                    v = (ap += 8, *(s64 *)(ap - 8));
                } else if (mod == 'h') {
                    v = (ap += 8, *(s16 *)(ap - 8));
                } else {
                    v = (ap += 8, *(s32 *)(ap - 8));
                }
                p = &buf[31];
                *p = 0;
                if (v == 0) {
                    *--p = '0';
                } else {
                    if (v < 0) {
                        v = -v;
                        r1_D_0012FD00_A0A0[0]('-');
                    }
                    while (v != 0) {
                        *--p = v % 10 + '0';
                        v = v / 10;
                    }
                }
                if (pad != 0 && pad < p) {
                    p = pad;
                }
                while (*p != 0) {
                    r1_D_0012FD00_A0A0[0](*p++);
                }
                break;
            case 'u' - '0':
                if (mod == 'l') {
                    v = (ap += 8, *(s64 *)(ap - 8));
                } else if (mod == 'h') {
                    v = (ap += 8, *(u16 *)(ap - 8));
                } else {
                    v = (ap += 8, *(u32 *)(ap - 8));
                }
                p = &buf[31];
                *p = 0;
                if (v == 0) {
                    *--p = '0';
                } else {
                    do {
                        *--p = (u64)v % 10 + '0';
                        v = (u64)v / 10;
                    } while (v != 0);
                }
                if (pad != 0 && pad < p) {
                    p = pad;
                }
                while (*p != 0) {
                    r1_D_0012FD00_A0A0[0](*p++);
                }
                break;
            case 'e' - '0':
            case 'f' - '0': {
                f32 f = (ap += 8, *(f32 *)(ap - 8));
                if (f == 0.0f) {
                    r1_D_0012FD00_A0A0[0]('0');
                } else {
                    printfloat_A0A0(f);
                }
            } break;
            case 's' - '0': {
                char *string = (ap += 8, *(char * *)(ap - 8));
                if (*string == 0) {
                    r1_D_0012FD00_A0A0[0]('(');
                    r1_D_0012FD00_A0A0[0]('n');
                    r1_D_0012FD00_A0A0[0]('u');
                    r1_D_0012FD00_A0A0[0]('l');
                    r1_D_0012FD00_A0A0[0]('l');
                    r1_D_0012FD00_A0A0[0](')');
                } else {
                    p = string;
                    if (*p != 0) {
                        do {
                            r1_D_0012FD00_A0A0[0](*p++);
                        } while (*p != 0);
                    }
                }
                break;
            }
            case 'c' - '0':
                v = (ap += 8, *(char *)(ap - 8));
                r1_D_0012FD00_A0A0[0](v);
                break;
            default:
                break;
            }
        } else {
            r1_D_0012FD00_A0A0[0](*fmt);
        }
    }
    if (ie != 0) {
        EnableInterrupts_A0A0();
    }
}

/* rac1-decomp src/core/00119D88.c: func_0011A690 */
/*
 * A real varargs forwarder: va_start, hand fmt and the va_list to
 * r1_func_0011A0A0, va_end. Its frame is printf's (r1_func_00116078, also
 * 2.9-ee): $ra at 0 and the $5-$11 save area at 0x58, which is where ap
 * points. Exact under 2.9-ee; under the game's 2.95.3 a varargs
 * definition also spills $f12-$f18 (SIZE 72/56), which is why this was
 * once a hand-built `long args[16]` forwarder (13/56).
 */
void func_0011C7E8(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    r1_func_0011A0A0(fmt, ap);
    va_end(ap);
}
