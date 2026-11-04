/* Retained complete C reconstruction; not linked or counted.
 * Profile: -O2 -G0 -ffunction-sections
 * 180/172 bytes; 0/1 placements in us and us2; pal has no catalogue entry.
 * Residual: The aligned record address stays live across the random call, adding a saved register. Three full-buffer layouts give the same 180-byte shape.
 * Reopen: Independent compiler/source lifetime evidence for rematerializing the record address across the call.
 * Trial-only markers and profiles are restored after checking.
 */

#include "common.h"
typedef int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vector16;
extern s32 func_002A77E0(s32);
extern void func_0029FCC0(void *, s32, void *);
extern float func_002A7878(float, float);
extern void func_L16_0041F1F0(void *, void *, void *, s32, float, float);

/* Builds a full aligned acceleration record and emits it at a selected joint. */
void func_L16_00411ED0(u8 *owner)
{
    u8 *state = *(u8 **)(owner + 0x68);
    float point[4] __attribute__((aligned(16)));
    float acceleration[4] __attribute__((aligned(16)));
    float t;
    func_0029FCC0(owner, func_002A77E0(2), point);
    qzero(acceleration);
    t = func_002A7878(5.0f, 10.0f);
    acceleration[2] = -(t * (1.0f / 60.0f));
    func_L16_0041F1F0(owner, point, acceleration, 1,
                      1.0f / 180.0f, (float)state[0x36]);
}
