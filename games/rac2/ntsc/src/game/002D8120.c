#include "common.h"

typedef struct {
    short id;
    short active;
    char pad[8];
} Func002D8120Slot;

extern int D_001A7C04 MACRO_ADDR;
extern unsigned char D_0018D0E8[];
extern short D_00254EC8[][2];
extern Func002D8120Slot D_0025ADB0[];

int func_002D8120(void)
{
    int i;

    for (i = 0; i < D_001A7C04; i++) {
        D_0025ADB0[i].active = 1;
        D_0025ADB0[i].id = *D_00254EC8[D_0018D0E8[D_001A7C04 - 1 - i]];
    }
    D_0025ADB0[D_001A7C04].id = 0;
    return 0;
}
