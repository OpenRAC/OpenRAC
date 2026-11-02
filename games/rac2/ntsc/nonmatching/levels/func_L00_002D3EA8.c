/* Canonical callees, qcopy/barrier plus no-schedule-insns: 132/136 bytes in us/pal/us2, 0/27 each. The copy stays grouped, but the stack argument setup still fills the mode branch delay slot. No exact placement; next useful input is a comparable solved conditional stack-copy wrapper.
 * Profile: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier -fno-schedule-insns
 * us: 132 bytes, expected 136
 * pal: 132 bytes, expected 136
 * us2: 132 bytes, expected 136
 */
#include "include_asm.h"

#ifndef RAC2_T_V4_F6A70B950
#define RAC2_T_V4_F6A70B950
typedef int V4_F6a70b950 __attribute__((mode(TI)));
#endif

extern char D_00189E20[];

extern float func_00282E20(char *);

extern float func_00283248(float, float);

extern void func_002830C0(void *, void *, char *);

/* Blends the object's field 8 with a callee float when the shared table's mode byte
   is zero; outside the modes 0..2 it answers 0.0f, and inside them the object is copied
   into a 16-byte local, filled through the per-level dispatcher, and the same two floats
   are combined. */
float func_L00_002D3EA8(char *in)
{
    char *p = D_00189E20;
    int mode = *(unsigned char *)(p + 8899);
    V4_F6a70b950 local;

    if (mode == 0)
        return func_00283248(*(float *)(in + 8), func_00282E20(in));
    if (mode < 0)
        return 0.0f;
    if (mode >= 3)
        return 0.0f;
    local = *(V4_F6a70b950 *)in;
    func_002830C0(&local, &local, p + 64);
    return func_00283248(*(float *)((char *)&local + 8), func_00282E20((char *)&local));
}
