/* func_L04_003A1868, 384 bytes, 5 placements (levels 04, 06, 08, 18, 20).
   Needs its own unit with -O2 -G8 -ffunction-sections -mno-split-addresses.

   M30: declaring the absolute globals NOT_SDA under -mno-split-addresses gives
   retail's `lui $at; swc1` macro stores (GCC counts them as two instructions, so
   they no longer fill the call slots), and the size is now retail's 384 bytes.
   What is left is order only: retail loads the table address first, then the
   five constants (0.9f before 0.1f), and stores the eight floats in source order
   with the small-data 16.0f fifth; here the constants come first and the
   small-data store moves.  Queued for the permuter.
*/
#include "include_asm.h"
#include "common.h"

/* the level's table of two floats */
typedef struct {
    u8  pad00[0x10];
    f32 field10;
    f32 field14;
} L04Table;

/* the globals the level's lighting reads; the small-data ones are reached
   through $gp, the others by their absolute address */
extern L04Table ref_L04_003A1868_00;

extern f32 ref_L04_003A1868_01 NOT_SDA;
extern f32 ref_L04_003A1868_02 NOT_SDA;
extern f32 ref_L04_003A1868_03 NOT_SDA;
extern f32 ref_L04_003A1868_04 NOT_SDA;
extern f32 ref_L04_003A1868_05 __attribute__((sda));
extern f32 ref_L04_003A1868_06 NOT_SDA;
extern f32 ref_L04_003A1868_07 NOT_SDA;
extern f32 ref_L04_003A1868_08 NOT_SDA;
extern f32 ref_L04_003A1868_09 NOT_SDA;
extern f32 ref_L04_003A1868_10 NOT_SDA;
extern f32 ref_L04_003A1868_11 NOT_SDA;
extern u32 ref_L04_003A1868_12 NOT_SDA;
extern u32 ref_L04_003A1868_13 NOT_SDA;
extern u8  ref_L04_003A1868_14 NOT_SDA;
extern f32 ref_L04_003A1868_15 __attribute__((sda));
extern f32 ref_L04_003A1868_16 __attribute__((sda));
extern f32 ref_L04_003A1868_17 __attribute__((sda));
extern u8  ref_L04_003A1868_18 NOT_SDA;
extern u32 ref_L04_003A1868_19 __attribute__((sda));
extern u32 ref_L04_003A1868_20 NOT_SDA;
extern u32 ref_L04_003A1868_21 NOT_SDA;
extern u32 ref_L04_003A1868_22 NOT_SDA;

extern f32 func_00283248(f32 a, f32 b);
extern f32 func_00283180(f32 a);
extern f32 func_00283198(f32 a);

/* Fills the level's lighting globals: the engine's helpers give the light's
   direction, the two of them are scaled by 1/sqrt(3). */
void func_L04_003A1868(void *a0, void *a1, void *a2) {
    f32 base;
    f32 scale;

    ref_L04_003A1868_01 = 16.0f;
    ref_L04_003A1868_02 = -8.0f;
    ref_L04_003A1868_03 = 1.0f;
    ref_L04_003A1868_04 = 0.9f;
    ref_L04_003A1868_06 = -8.0f;
    ref_L04_003A1868_07 = 1.0f;
    ref_L04_003A1868_08 = 0.1f;
    ref_L04_003A1868_05 = 16.0f;
    {
        L04Table *t = &ref_L04_003A1868_00;
        base = func_00283248(t->field10, t->field14);
    }
    scale = 0.57735f;
    ref_L04_003A1868_09 = func_00283180(base) * scale;
    ref_L04_003A1868_10 = func_00283198(base) * scale;
    ref_L04_003A1868_11 = -0.57735f;
    ref_L04_003A1868_12 = 63;
    ref_L04_003A1868_13 = 64;
    ref_L04_003A1868_14 = 127;
    ref_L04_003A1868_15 = 32768.0f;
    ref_L04_003A1868_16 = 255.0f;
    ref_L04_003A1868_17 = 48.0f;
    ref_L04_003A1868_18 = 127;
    ref_L04_003A1868_19 = 0;
    ref_L04_003A1868_20 = (u32)a0;
    ref_L04_003A1868_21 = (u32)a1;
    ref_L04_003A1868_22 = (u32)a2;
}
