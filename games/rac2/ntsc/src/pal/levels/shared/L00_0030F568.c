#include "include_asm.h"

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F568);

extern int D_L00_001A904C_2 __attribute__((sda));
extern void func_0033D970(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F5A8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033D970(value + 0x3D288);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F5D8);

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F618);

extern void func_0033C738(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F658(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033C738(value + 0x3D568);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F688);

extern void func_0033CC20(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F6C8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033CC20(value + 0x3D848);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F6F8);

extern void func_0033D638(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F738(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033D638(value + 0x3DE98);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F768);

extern void func_0033C028(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F7A8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033C028(value + 0x3E178);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F7D8);

extern void func_0033C378(int);

/* Reads the object base from a shared global and, when it is non-zero, calls the handler at a
 * fixed byte offset in it. */
void func_PAL_L00_0030F818(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033C378(value + 0x3E458);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F848);

void func_PAL_L00_0030F888(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033E428(value + 0x3F4A0);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F8B8);

void func_PAL_L00_0030F8F8(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033D2C0(value + 0x3DB28);
}

INCLUDE_ASM("asm/pal/levels", func_PAL_L00_0030F928);

void func_PAL_L00_0030F968(void)
{
    int value = D_L00_001A904C_2;

    if (value != 0)
        func_0033DCC8(value + 0x3EC00);
}

