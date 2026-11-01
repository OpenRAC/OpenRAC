/* func_L05_00420FB0, 332 bytes, 1 placement (level 05).  G8 candidate is 328 bytes.
   The body is recovered: the reset-flag test and the one-shot init (the
   D_L05_00221D10 word at +0x2C and the flag itself), the
   func_0037B210/func_00379F18/func_00379EB8/func_00379EC8 sequence, the walk
   over the live-object list (the id at +0xAA against 0xF73, the spawner call,
   the two func_0037CEE0/func_0037D608 calls with the 64.0f scale in $f20 and
   the record index scaled by 128) and the tail (the D_L05_00221D10 word at
   +0x10, the D_L05_001B2048 word against 6, func_L05_00420788 with the
   second argument 2).  The frame, the loop, the calls and the float match.
   Three residues: the tail materialises the D_L05_00221D10 address in a
   register (retail `addiu $v0,$s3,0x1D10` then `lw $v1,0x10($v0)`) where this
   cc1 folds the +0x10 into the %lo displacement (one instruction short);
   retail branches likely on the two guards (`blezl` and `bnel`) where this cc1
   emits the plain `blez`/`bne`; and the base, the count and the id constant
   land in $s3/$s1/$s4 in retail and in $s4/$s1/$s3 here.  Needs the -G8 unit:
   the reset flag (0x001B1364) is reached through $gp in retail.  */

#include "include_asm.h"

typedef signed int s32;
typedef signed short s16;
typedef unsigned char u8;
typedef unsigned int u32;
typedef float f32;

extern s32 func_003062A0(u8 *a0);
extern void func_003404B8(void);
extern void func_0037B210(void);
extern void func_00379F18(s32 a0);
extern u8 **func_00379EB8(void);
extern s32 func_00379EC8(void);
extern void func_0037B280(u8 *a0, s32 a1);
extern void func_0037CEE0(u8 *a0, u8 *a1, f32 f12);
extern void func_0037D608(u8 *a0, u8 *a1, f32 f12);
extern void func_L05_00420788(u8 *a0, s32 a1);

extern s32 D_001B1364 __attribute__((sda));
extern s32 D_L05_001B2048;
extern u8 D_L05_00221D10[];
extern u8 *D_001B2614;

void func_L05_00420FB0(u8 *arg0)
{
    u8 **list;
    u32 count;
    s32 index;
    s32 found;

    if (D_001B1364 == 0) {
        *(s32 *)(D_L05_00221D10 + 0x2C) = 0;
        D_001B1364 = 1;
        func_003404B8();
    }
    func_0037B210();
    func_00379F18(0);
    func_00379EB8();
    found = func_00379EC8();
    if (found > 0) {
        list = (u8 **)found;
        count = (u32)D_L05_00221D10;
        do {
            u8 *entry = *list;

            if (*(s16 *)(entry + 0xAA) == 0xF73) {
                f32 scale = 64.0f;

                count--;
                func_0037B280(entry, 1);
                index = *(u32 *)D_L05_00221D10;
                func_0037CEE0(*list, (u8 *)D_001B2614 + (index << 7) + 0x30, scale);
                index = *(u32 *)D_L05_00221D10;
                func_0037D608(*list, (u8 *)D_001B2614 + (index << 7) + 0x30, scale);
            }
            count--;
            list++;
        } while (count != 0);
    }
    if (*(s32 *)(D_L05_00221D10 + 0x10) == 0 && D_L05_001B2048 >= 6)
        func_L05_00420788(arg0, 2);
}
