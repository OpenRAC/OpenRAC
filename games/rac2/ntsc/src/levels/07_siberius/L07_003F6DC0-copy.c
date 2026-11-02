#include "common.h"
typedef int SibQuad __attribute__((mode(TI)));
typedef union { SibQuad quad; float value[4]; } SibVector;
extern float D_L07_001B0FE0 __attribute__((sda));
extern float D_L07_001B0FE4 __attribute__((sda));
extern void func_00282CF0(void *, void *, void *);
extern float func_002AA548(float *, float *, float, float, float, float);
extern void func_00282F20(void *, void *, float);
extern void func_002833F0(void *, void *);
extern void func_00283098(void *, void *, void *);
extern void func_002AC9F8(float *, float);
extern void func_00282CC0(void *, void *, void *);
/* The incoming second pointer is live at the first vector subtraction. */
void func_L07_003F6DC0(u8 *self, float *target)
{
    SibVector offset, velocity, delta, angles, rotationInput;
    float matrix[12] __attribute__((aligned(16)));
    u8 *state;
    u8 *movement;
    u8 *position = self + 0x10;
    float rate, elapsed;
    state = *(u8 **)(self + 0x68);
    func_00282CF0(&offset, target, position);
    rate = D_L07_001B0FE0 * (1.0f / 3600.0f);
    elapsed = D_L07_001B0FE4 * (1.0f / 60.0f);
    movement = state + 0x50;
    offset.value[2] = 0.0f;
    func_002AA548((float *)(self + 0x18), (float *)(state + 0x5C), *(float *)(state + 0x60), rate, rate, elapsed);
    func_00282F20(&velocity, &offset, elapsed);
    rotationInput.quad = 0;
    rotationInput.value[2] = *(float *)(state + 0x64);
    angles.quad = rotationInput.quad;
    func_002833F0(matrix, &angles);
    func_00283098(&velocity, &velocity, matrix);
    func_00282CF0(&delta, &velocity, movement);
    func_002AC9F8(delta.value, rate);
    func_00282CC0(movement, movement, &delta);
    func_00282CC0(position, position, movement);
}
