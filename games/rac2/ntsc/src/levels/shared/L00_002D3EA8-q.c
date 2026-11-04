#include "include_asm.h"
#include "common.h"

extern char D_00189E20[];
extern float func_00282E20(char *);
extern float func_00283248(float, float);
extern void func_002830C0(void *, void *, char *);

float func_L00_002D3EA8(char *in)
{
    char *p = D_00189E20;
    int mode = *(unsigned char *)(p + 8899);
    float local[4] __attribute__((aligned(16)));

    if (mode == 0)
        return func_00283248(*(float *)(in + 8), func_00282E20(in));
    if (mode < 0)
        return 0.0f;
    if (mode >= 3)
        return 0.0f;
    qcopy(local, in);
    func_002830C0(local, local, p + 64);
    return func_00283248(local[2], func_00282E20((char *)local));
}
