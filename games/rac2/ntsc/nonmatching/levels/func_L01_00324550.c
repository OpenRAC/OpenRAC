/* Current retained result, 28 October 2026: 132/132 bytes with game -mastra-quad-saves, but
 * mixed s0/ra save widths and order differ. The SDK profile emits 116/132 and lacks FP spills.
 * The real variadic forwarding body is retained without fabricated frame padding. No strict
 * match. Older diagnostic notes below are historical and do not rule out C. */

/* Retained readable C; not matching and no exact-C credit.
 * 2026-10-28 bounded retry: Game profile -O2 -G0 -ffunction-sections -mastra-quad-saves: 132/132 bytes, exact varargs forwarding/body but both s0 and ra save width/order differ from retail mixed saves. SDK profile: 116/132 and missing FP argument spills. The register-save area is reached with the existing builtin-next-arg convention; no fabricated frame padding was used.
 * Reopen with an independently measured save-profile or allocation witness.
 * No strict linked-build claim. Historical screen-only notes follow the source.
 */
#include "common.h"
extern int func_00288FB8(int);
extern void func_L01_00324410(char *, int, void *);
extern void func_0028BCA0(int);
extern void func_0026FEB8(const char *, ...);
extern const char D_001AA590[];
void func_L01_00324550(int id, ...)
{
    char text[0x200];
    int format = func_00288FB8(id);
    if (format) {
        /* Retail forwards the seven saved integer argument slots. */
        func_L01_00324410(text, format, (char *)__builtin_next_arg(id) - 0x38);
        func_0028BCA0((int)text);
    } else func_0026FEB8(D_001AA590, id);
}

/* func_L01_00324550 (132 bytes, 11 places) - diagnosis only, no C measured this wave.
 *
 * The body is ordinary: v0 = func_2E99D0(a0); when v0 is nonzero it runs
 * func_324410(sp, v0, sp+0x268) then func_2ECB20(sp), otherwise func_2A6CF0(0x001AA590, a0).
 * What blocks it is the prologue.  Before that first call retail has already stored
 * $a1-$a3 and $t0-$t3 to 0x268..0x298($sp) and $f12/$f14/$f16/$f18 to 0x258..0x264($sp)
 * on a 0x2A0 frame - the variadic register-save area - and it holds $s0 in a 128-bit slot
 * at 0x210($sp) with `sq $s0,0x210($sp)` (0x7FB00210), reading it back with
 * `lq $s0,0x210($sp)` (0x7BB00210) in the delay slot of the branch at 0x003245AC and
 * storing it again at 0x003245C4.  $s0 is an ordinary 64-bit value (`daddu $s0,$a0,$zero`
 * in the first delay slot), so the quad slot is the prologue's own register-save shape and
 * not a 128-bit object.
 * Missing: a C spelling for that variadic prologue (the natural `...` + va_start route was
 * not measured here, and the saved-integer area is passed on to func_324410, so the two
 * are the same mechanism).  ASM_FUNC candidate until a lever appears.
 */
