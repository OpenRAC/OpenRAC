#include "include_asm.h"
extern float D_001A8C7C __attribute__((sda));
extern float D_001B15A0;
extern void func_0027D330(int);

/* Publishes the slow-down factor and, while the frame rate is below one, asks
   for the six-frame path. */
void func_PAL_002F6C18(void)
{
    D_001A8C7C = 1.2f;
    if (D_001B15A0 < 1.0f)
        func_0027D330(5);
}
