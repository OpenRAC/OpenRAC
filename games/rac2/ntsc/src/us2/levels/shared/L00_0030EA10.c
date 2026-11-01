#include "include_asm.h"

extern int D_L00_001A904C_2 __attribute__((sda));
extern int func_0033C590(int, int);

/* Reads a bound global word; when it is non-zero it calls the object's handler with that value
 * plus a fixed base offset as the first argument and the function's own argument as the second,
 * returning the handler's result, otherwise returns 0. */
int func_US2_L00_0030EA10(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_0033C590(value + 0x3D4B8, argument);
    return 0;
}

extern void func_0033C738(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030EA50(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033C738(value + 0x3D4B8);
}

extern int func_L00_00432448(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030EA80(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_L00_00432448(value + 0x3D798, argument);
    return 0;
}

extern void func_0033CC20(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030EAC0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033CC20(value + 0x3D798);
}

extern int func_L00_00432ED0(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030EAF0(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_L00_00432ED0(value + 0x3DDE8, argument);
    return 0;
}

extern void func_0033D638(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030EB30(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033D638(value + 0x3DDE8);
}

extern int func_0033BE98(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030EB60(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_0033BE98(value + 0x3E0C8, argument);
    return 0;
}

extern void func_0033C028(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030EBA0(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033C028(value + 0x3E0C8);
}

extern int func_0033C2D8(int, int);

/* Same bound-global dispatch as its neighbours, with its own base offset and handler. */
int func_US2_L00_0030EBD0(int argument)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        return func_0033C2D8(value + 0x3E3A8, argument);
    return 0;
}

extern void func_0033C378(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_US2_L00_0030EC10(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033C378(value + 0x3E3A8);
}

