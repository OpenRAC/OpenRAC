#include "include_asm.h"


extern int D_L01_001A90C8 __attribute__((sda));
extern float D_L01_001A90C4 __attribute__((sda));
extern int D_L01_001A90CC __attribute__((sda));
extern int D_L01_001A90D0 __attribute__((sda));
extern int func_L01_002F1888(void);
extern int func_L01_002F1588(void);
extern int func_0029B7C0(int (*)(void),int (*)(void),int);

void func_L01_002F14E8(int first, int second, float value)
{
    if (D_L01_001A90C8 == 0)
        func_0029B7C0(func_L01_002F1888, func_L01_002F1588, 0);
    D_L01_001A90C8 = 5;
    D_L01_001A90C4 = value;
    if (value > 1.0f)
        D_L01_001A90C4 = 1.0f;
    else if (value < 0.0f)
        D_L01_001A90C4 = 0.0f;
    D_L01_001A90D0 = second;
    D_L01_001A90CC = first;
}
