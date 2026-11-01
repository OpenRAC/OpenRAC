#include "include_asm.h"

extern int func_002A7820(int, int);
extern float func_002A7940(void);

extern int func_002A7820(int, int);
extern float func_002A7940(void);
extern void func_L16_00414038(void *, int, float, int, float, float, int, int, void *);
extern float D_L16_001B1948 __attribute__((sda));
extern float D_L16_001B194C __attribute__((sda));
extern int D_L16_001B1940 __attribute__((sda));
extern int D_L16_001B1944 __attribute__((sda));
extern int D_L16_001B1908 __attribute__((sda));
extern int D_L16_001B190C __attribute__((sda));
void func_L16_00413EC8(char *object) {
    int value;
    int value2;
    float scale;
    float scale2;
    float random;
    scale = D_L16_001B1948;
    scale *= *(float *)(object + 0x2C);
    value = func_002A7820(95, 111);
    random = func_002A7940();
    func_L16_00414038(object + 0x10, D_L16_001B1908, scale, value, random, 1.0f, D_L16_001B1940, 2, object);
    scale2 = D_L16_001B1948;
    scale2 *= *(float *)(object + 0x2C);
    scale2 *= D_L16_001B194C;
    value2 = func_002A7820(111, 127);
    random = func_002A7940();
    func_L16_00414038(object + 0x10, D_L16_001B190C, scale2, value2, random, 1.0f, D_L16_001B1944, 2, object);
}
