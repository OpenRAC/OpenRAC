#include "common.h"

extern float D_L11_001B1FC8 SDATA(D_L11_001B1FC8);
extern float D_L11_001B1FCC SDATA(D_L11_001B1FCC);
extern int D_L11_001B1FBC SDATA(D_L11_001B1FBC);
extern int D_L11_001B1FC0 SDATA(D_L11_001B1FC0);
extern float func_002A7878(float, float);
extern int func_002A7820(int, int);
extern float func_002A7940(void);
extern void func_L11_00458DF0(void *, int, float, int, float, int, int);

void func_L11_00458C38(unsigned char *object)
{
    float size, random_size;
    random_size = func_002A7878(0.0f, D_L11_001B1FC8 * D_L11_001B1FCC);
    size = D_L11_001B1FC8;
    size += random_size;
    func_L11_00458DF0(object + 0x10, D_L11_001B1FBC, size, func_002A7820(80, 96),
                      func_002A7940(), 49, 2);
    size *= 1.2f;
    func_L11_00458DF0(object + 0x10, D_L11_001B1FC0, size, func_002A7820(8, 22),
                      func_002A7940(), 11, 2);
}
