/* func_0028BCE8 - near miss (worker w8, 22 October 2026).

   128 bytes, 28 places.  The body publishes the count 8 in D_001B18D0, the address of the
   word at 0x001A8DD0 in D_001B18D4 and 0 in the word at 0x001A8D48, and then walks eight
   28-byte records at 0x00255170: for each of the eight words of the table at 0x001A73B8
   it stores the word in the record's +0x18 and, through the byte index the word selects in
   the table at 0x00139568, the 16-bit value at +0x3C of the 0xE0-byte record at 0x00239BA0
   in the record's +0.  Retail:

     addiu $v0,$zero,8
     lui   $v1,0x14
     addiu $t1,$v1,-0x6A98      ; the index table, 0x139568
     addiu $a0,$gp,-0x6220      ; the word at 0x1A8DD0
     lui   $at,0x1B
     sw    $v0,0x18D0($at)      ; D_001B18D0 = 8
     lui   $v0,0x24
     addiu $t2,$v0,-0x6460      ; the records, 0x239BA0
     lui   $v1,0x25
     addiu $a1,$v1,0x5170       ; the destination, 0x255170
     lui   $at,0x1B
     sw    $a0,0x18D4($at)      ; D_001B18D4 = the address
     sw    $zero,-0x62A8($gp)   ; the word at 0x1A8D48 = 0
     addiu $t0,$zero,0xE0       ; the record size
     lui   $a3,0x1A
     addiu $a3,$a3,0x73B8       ; the table, 0x1A73B8
     addiu $a2,$zero,7          ; the counter
     nop
     loop:
     lw    $a0,0x0($a3)
     addiu $a2,$a2,-1
     addiu $a3,$a3,4
     sw    $a0,0x18($a1)
     addu  $v1,$a0,$t1
     lbu   $v0,0x0($v1)
     mult  $v1,$v0,$t0
     addu  $v0,$v1,$t2
     lhu   $v1,0x3C($v0)
     sw    $v1,0x0($a1)
     bgez  $a2,loop
     addiu $a1,$a1,0x1C
     jr $ra
     nop

   The candidate below is 128 bytes and its loop is retail's loop instruction for
   instruction, register for register - the 28-byte record, the u32 field at +0x18, the u16
   field at +0x3C of the 0xE0-byte record and the counter that runs from 7 down to -1 are
   all right.  What is left is the order of the prologue: retail materialises the index
   table, the record table, the destination and the source each as an adjacent lui/addiu
   pair and interleaves them with the three stores, while this cc1 groups the loop's
   constants after the stores (and, with the default split-address profile, keeps the high
   halves of the addresses in their own registers with the low halves added later).
   -mno-split-addresses gives the adjacent pairs but not the order; it also breaks the
   already matched functions of the unit, so it cannot be given to the unit.
   tools/check.py puts the first difference at +0x3c, the sixteenth word: the fifteen words
   before it hold the same symbols in another order, and the check compares them through
   their relocations.

   The body reaches small data ($gp-0x6220 and $gp-0x62A8), so it belongs in the -g8
   sibling of the unit; the two absolute globals keep their absolute form there without a
   qualifier (adding __attribute__((nosda)) to them changes the register allocation of the
   loop and costs the match).

   tools/check.py func_0028BCE8: 0 of 28 places match (src/game/00288D88.c, default
   profile; the candidate belongs in src/game/00288D88-g8.c). */

#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct {
    u32 value;
    u8 pad[0x14];
    u32 id;
} Entry_0028BCE8;

typedef struct {
    u8 pad[0x3C];
    u16 value;
} Rec_0028BCE8;

extern u32 D_001B18D0;
extern u32 D_001B18D4;
extern u32 D_001A8DD0 __attribute__((sda));
extern u32 D_001A8D48 __attribute__((sda));
extern u32 D_001A73B8[];
extern u8 D_00139568[];
extern u8 D_00239BA0[];
extern Entry_0028BCE8 D_00255170[];

void func_0028BCE8(void)
{
    Entry_0028BCE8 *dst = D_00255170;
    u32 *src = D_001A73B8;
    int i = 7;
    u32 v;

    D_001B18D0 = 8;
    D_001B18D4 = (u32)&D_001A8DD0;
    D_001A8D48 = 0;
    while (i >= 0) {
        v = *src++;
        i--;
        dst->id = v;
        dst->value = ((Rec_0028BCE8 *)(D_00239BA0 + D_00139568[v] * 0xE0))->value;
        dst++;
    }
}
