#include "include_asm.h"

extern unsigned char D_L05_001B2000;
extern unsigned char D_L05_001B2001;
extern unsigned char D_L05_001B2002;
extern float D_L05_001B2004 __attribute__((sda));
extern float D_L05_001B2008 __attribute__((sda));
extern float D_L05_001B200C __attribute__((sda));
extern float D_L05_001B2010 __attribute__((sda));
extern int D_L05_001B2014 __attribute__((sda));
extern unsigned char D_L05_001B3630;
extern unsigned char D_L05_001B3631;
extern unsigned char D_L05_001B3632;
extern float D_L05_001B3634 __attribute__((sda));
extern float D_L05_001B3638 __attribute__((sda));
extern float D_L05_001B363C __attribute__((sda));
extern float D_L05_001B3640 __attribute__((sda));
extern int D_L05_001B3644;

/* The reverse copy of func_L05_00442720. */
void func_L05_004427A8(void)
{
    unsigned int b0 = D_L05_001B3630;
    unsigned int b1 = D_L05_001B3631;
    unsigned int b2 = D_L05_001B3632;
    float f0 = D_L05_001B3638;
    float f1 = D_L05_001B3640;
    float f2 = D_L05_001B3634;
    float f3 = D_L05_001B363C;
    unsigned int w = D_L05_001B3644;

    D_L05_001B2000 = b0;
    D_L05_001B2001 = b1;
    D_L05_001B2002 = b2;
    D_L05_001B2008 = f0;
    D_L05_001B2010 = f1;
    D_L05_001B2004 = f2;
    D_L05_001B200C = f3;
    D_L05_001B2014 = w;
}
