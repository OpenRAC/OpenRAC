/* Copies the quad at +0x10 of the object to the stack with the approved qcopy helper and
   stores func_L08_003D8290(&copy, v + 3, v - 20), v the float at +0x3E8 of its state, at
   +0x434 of the state. */
#include "common.h"
typedef unsigned int ti_word __attribute__((mode(TI)));
extern float func_L08_003D8290(void *, float, float);

void func_L08_003D82F0(char *obj) {
    ti_word copy;
    char *state = *(char **)(obj + 0x68);
    float value;
    qcopy(&copy, obj + 0x10);
    value = *(float *)(state + 0x3E8);
    *(float *)(state + 0x434) = func_L08_003D8290(&copy, value + 3.0f, value - 20.0f);
}
