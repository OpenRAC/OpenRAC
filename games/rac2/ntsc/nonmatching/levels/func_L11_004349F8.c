/* GPT-6.1 Sol high, task v0887; USA v1.01.
 * Best: 176/176 bytes, 0/1 places. Profile: -O2 -G0 -ffunction-sections -g.
 * Five instructions differ in the 0x30/0x31/0x32 stores and their constants.
 * The prologue, initial vector saves, final copies, and epilogue agree.
 * Keeping the second stack-vector address as a local before its initial save
 * recovers s1 and the 0x50 frame. Only the two materialized final transfers use
 * qcopy; the initial saves and the middle transfer remain ordinary TImode C.
 * Reordering the halfword store merges the two 255 constants (172 bytes);
 * typed fields and -fno-gcse tie; disabling sched2 worsens the rest of the body.
 * Next: evidence for the original field/store-expression semantics or schedule.
 * All required declarations follow.
 */
#include "common.h"
typedef int TI128 __attribute__((mode(TI)));
extern char *func_3224B0(int);

char *func_L11_004349F8(char *obj, TI128 *p1, TI128 *p2, float f)
{
    TI128 t[2];
    TI128 *saved = &t[1];
    char *n;
    char *st;

    t[0] = *p1;
    *saved = *p2;
    n = func_3224B0(0xD1B);
    if (n != 0) {
        *(unsigned char *)(n + 0x31) = 1;
        *(unsigned char *)(n + 0x30) = 0xFF;
        *(unsigned short *)(n + 0x32) = 0xFF;
        *(unsigned char *)(n + 0x20) = 0;
        st = *(char **)(n + 0x68);
        *(unsigned long long *)(n + 0x38) = *(unsigned long long *)(obj + 0x38);
        qcopy(n + 0x10, st);
        *(TI128 *)st = t[0];
        qcopy(st + 0x20, saved);
        *(float *)(st + 0x3C) = f;
        *(int *)(st + 0x38) = 0;
        *(int *)(st + 0x34) = 0;
    }
    return n;
}
