#include "include_asm.h"

extern void func_L00_00426248(int);
extern void func_L00_00425490_1(int);
extern void func_L00_00424F98(int, float *, float *);
extern void func_L00_00425118(int, float, float);
extern void func_L00_004243C0(int);
extern void func_L00_00425CD0(int);
extern void func_L00_00425560(int);

/* runs a fixed sequence of seven callees on one object handle, passing the address of two stack floats to the third callee and then their values to the fourth */
void func_PAL_L00_0042B858(int object)
{
    float x;
    float y;

    func_L00_00426248(object);
    func_L00_00425490_1(object);
    func_L00_00424F98(object, &x, &y);
    func_L00_00425118(object, x, y);
    func_L00_004243C0(object);
    func_L00_00425CD0(object);
    func_L00_00425560(object);
}

