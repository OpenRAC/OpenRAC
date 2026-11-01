#include "include_asm.h"

/* Bounded replay, 28 October 2026, current USA v1.01 profile:
 * -G0 compiles this to 40 bytes, but the final movn/store use $v1 as the
 * selected pointer and $a1 as the condition; retail uses $a1 and $v1,
 * respectively. Reordering the pointer declarations and storing the
 * conditional expression directly produced the same register allocation.
 * An isolated -G8 unit is 32 bytes, not retail's 40. Keep this as a near
 * miss; do not add frame or register-control tricks to force the remaining
 * instructions.
 */

extern volatile int D_001A7318;
extern char D_001AB658 __attribute__((sda));
extern char D_001AB628 __attribute__((sda));

int func_002CC528(void *object)
{
    void *alternate = &D_001AB628;
    void *selected = &D_001AB658;
    int enabled = D_001A7318;
    if (enabled)
        selected = alternate;
    *(void **)((char *)object + 0x34) = selected;
    return 0;
}
