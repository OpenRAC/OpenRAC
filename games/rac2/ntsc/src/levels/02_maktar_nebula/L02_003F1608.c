#include "common.h"

extern void func_002A91F0(void *, float *);

/* Advance the four-slot history and capture the attached object position. */
void func_L02_003F1608(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    *(int *)(state + 0x4D8) = 0;
    *(int *)(state + 0x4D4) = (*(int *)(state + 0x4D4) + 1) % 4;
    if (*(int *)(state + 0x4D0) < 4)
        (*(int *)(state + 0x4D0))++;
    func_002A91F0(state + (*(int *)(state + 0x4D4) * 16 + 0x3A0), (float *)(arg0 + 0xF0));
    if (*(int *)(state + 0x4C4) != 0)
        qcopy(state + *(int *)(state + 0x4D4) * 16 + 0x2A0,
              (char *)(*(int *)(state + 0x4C4) + 0x10));
}
