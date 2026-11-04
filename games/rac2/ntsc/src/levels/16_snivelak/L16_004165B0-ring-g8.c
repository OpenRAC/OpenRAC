/* func_L16_0041F360: own -G8 unit (default game profile by name). */
#include "common.h"

extern f32 D_L16_001B2008 __attribute__((sda));
extern u32 D_L16_001B2000 __attribute__((sda));
extern s32 D_L16_001B1FF8 __attribute__((sda));
extern u32 D_L16_001B2004[] MACRO_ADDR;
extern s32 D_L16_001B1FFC[] MACRO_ADDR;
extern f32 D_L16_001B200C[] MACRO_ADDR;
extern f32 D_L16_001B2010[] MACRO_ADDR;
extern f32 D_L16_001B2014[] MACRO_ADDR;
extern char D_L16_001A8F20[] MACRO_ADDR;
extern f32 func_00283180(f32);
extern f32 func_00283198(f32);
extern void func_00282CC0(char *, char *, char *);
extern void func_002A8C00(char *, char *, s32, char *, f32);
extern s32 func_002761F8(char *, char *, s32, char *, char *);
extern u32 func_00283D38(u32, u32, f32);
extern void func_L00_002EBE88(char *, s32, s32, s32, u32, u32, f32, f32, f32);

/* Constructs and clips the expanding ring, then draws its two surfaces. */
