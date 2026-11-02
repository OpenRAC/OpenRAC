/* GPT-6.1 Sol, task v0364, USA v1.01.
   228/240 bytes, 0/1 places with -frac2-qcopy -frac2-qcopy-barrier -g; missing saved stack pointer and scheduling differences remain. Corrected old draft to consume func_00283198 return; expressing first scaled add directly fixes multiply operand order. qcopy_nc does not retain the stack pointer across calls.
   Required declarations are included below. */
#include "common.h"
typedef int TI128 __attribute__((mode(TI)));
extern float func_00283248(float, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern float func_002A7878(float, float);



void func_L11_0043ACB0(float *obj, char *arg1, float f12, float f13, float f14)
{
    struct {
        float x;
        float y;
        float z;
    } v;
    float f;
    float g;
    float h;
    float t;

    *(TI128 *)&v = *(TI128 *)arg1;
    f = func_00283248(obj[0] - v.x, obj[1] - v.y);
    g = f13 * 0.017453294f;
    h = func_002A7878(-g, g);
    f = f + h;
    h = func_002A7878(-f14, f14);
    f12 = f12 + h;
    qcopy_nc(obj, &v);
    t = func_00283180(f);
    obj[0] = obj[0] + f12 * t;
    t = func_00283198(f);
    f12 = f12 * t;
    obj[2] = v.z;
    obj[1] = obj[1] + f12;
}
