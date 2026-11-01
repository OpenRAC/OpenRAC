#include "common.h"
typedef int TQ __attribute__((mode(TI)));

extern void func_L00_002D7958(float *dst, void *src, float x);
extern void func_L00_002D7940(char *p);
extern void func_L00_002EE0C0(char *p);
extern void func_L00_00427B40(char *p);

/* Copies the full aligned target vector into both state slots, records the
 * weight and refreshes the two five-float blend records and owner transform. */
void func_L00_00427BE8(char *a0, void *a1, TQ *a2, f32 f12)
{
    char *s0 = *(char **)(a0 + 0x70);
    char *target = s0 + 0xB0;
    f32 weight = f12;

    qcopy(target, a2);
    qcopy(s0 + 0x80, a2);
    *(f32 *)(target + 0x18) = weight;
    func_L00_002D7958((float *)(s0 + 0x14), a1, weight);
    func_L00_002D7958((float *)(a0 + 0x50), a1, 0.0f);
    func_L00_002D7940(s0);
    func_L00_002EE0C0(s0 + 0x90);
    func_L00_00427B40(a0);
}
