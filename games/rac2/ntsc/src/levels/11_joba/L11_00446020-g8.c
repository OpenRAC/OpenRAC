#include "common.h"

typedef struct { char pad[0x2A0]; short counters[17]; } JobaCounters46020;
extern int D_00189E20[];
extern float D_L11_001B1870 __attribute__((sda));
extern int func_002A77E0(int);
extern int func_L11_00445D28(char *, int);

/* Selects an available action from the class-specific counter table, then dispatches it. */
void func_L11_00446020(char *self) {
    int action;
    char *state = *(char **)(self + 0x68);
    char *target = *(char **)(state + 0x180);

    if (target != 0) {
        action = -1;
        if (*(short *)(target + 0xAA) != 0xB3A) {
            if (*(short *)(target + 0xAA) == 0xD4A) {
                char *targetState = *(char **)(target + 0x68);
                if ((float)D_00189E20[0x933] <= 0.0f) {
                    action = func_002A77E0(3) + 0xC;
                    if (((JobaCounters46020 *)state)->counters[action] != 0) {
                        action = -1;
                    }
                }
                if (*(short *)(state + 0x2BE) == 0) {
                    if ((float)D_00189E20[0x933] < D_L11_001B1870) {
                        action = 0xF;
                    }
                }
                if (*(short *)(state + 0x2C0) == 0) {
                    if (*(float *)(targetState + 0x20) <= 0.0f) {
                        if (*(int *)(state + 0x140) == 0) {
                            action = 0x10;
                        }
                    }
                }
            }
        } else {
            char *targetState = *(char **)(target + 0x68);
            if ((float)D_00189E20[0x933] <= 0.0f) {
                action = func_002A77E0(2) + 6;
                if (((JobaCounters46020 *)state)->counters[action] != 0) {
                    action = -1;
                }
            }
            if (*(short *)(state + 0x2B0) == 0) {
                if ((float)D_00189E20[0x933] < D_L11_001B1870) {
                    action = 8;
                }
            }
            if (*(short *)(state + 0x2B2) == 0) {
                if (*(float *)(targetState + 0x20) <= 0.0f) {
                    action = 9;
                }
            }
        }
        if (action > 0) {
            func_L11_00445D28(self, action);
        }
    }
}
