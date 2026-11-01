#include "include_asm.h"
extern int D_L05_001B1504 __attribute__((sda));
extern int D_001A79F0;
extern float func_00282E48(void *, void *);
extern float func_002AA458(void *, void *, void *, float, float, float);
extern float func_00283B98(float, float);
extern void func_L05_0037AB88(int, int);
extern void func_0029CC50(int, int, int);
extern int func_002E5DA0(int, int, char *, int);
extern void func_0029F338(void *);
extern void func_L05_00426E70(void *);

/* Turns the object towards the global target and clamps its aim cone; a target within
   half a unit takes the fast path, the rest the slow one (an angle step and an update). */
void func_US2_L05_00428968(char *object)
{
    char *state = *(char **)(object + 0x68);
    float distance;

    if (D_L05_001B1504 != 0) {
        char *pos = object + 0x10;
        char *aim;
        float scaled;
        float capped;

        distance = func_00282E48((char *)D_L05_001B1504 + 0x10, pos);
        if (D_001A79F0 == 0x14) {
            func_002AA458(pos, (char *)D_L05_001B1504 + 0x10, state + 0x40,
                          0.008888889f, 0.0033333333f, 1.0666667f);
        } else {
            if (distance < 60.0f) {
                aim = *(char **)(object + 0x24);
                scaled = *(float *)(aim + 0x24) * 0.022222222f * distance;
                *(float *)(object + 0x2C) = scaled;
                capped = *(float *)(aim + 0x24) * 0.3f;
                if (capped > scaled)
                    *(float *)(object + 0x2C) = capped;
                func_002AA458(pos, (char *)D_L05_001B1504 + 0x10, state + 0x40,
                              0.0055555556f, 0.0016666667f, 0.5f);
            }
        }
        if (distance < 0.5f) {
            func_L05_0037AB88(4, 1);
            func_0029CC50(0x1285, 1, -1);
            func_002E5DA0(0xC, 0, object, 0xFBF);
            func_0029F338(object);
            return;
        }
    }
    *(float *)(object + 0xF8) = func_00283B98(*(float *)(object + 0xF8), 0.04f);
    func_L05_00426E70(object);
}
