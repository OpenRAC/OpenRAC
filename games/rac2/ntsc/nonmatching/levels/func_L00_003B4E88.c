/* func_L00_003B4E88 (112 bytes, 27 places) -- NEAR MISS, not integrated.  G8

   New measurement (wave 12, worker s4): the `-g8` route works and the body is the
   right size, but only the register allocation is wrong.  In a `-G8` sibling of its
   own (`L00_003B4E88-g8.c`: the existing `L00_003B3158-g8.c` is a matched unit and
   may not grow) with

       extern int D_L00_001AFA18 __attribute__((sda));
       extern int D_L00_001AFA1C __attribute__((sda));
       extern float D_L00_001B22D8 __attribute__((sda));
       extern int D_L00_001B28D0 __attribute__((nosda));
       extern int D_L00_001B28D4 __attribute__((nosda));

   the three small-data globals come out as retail's `lw $v0,0xA28($gp)` /
   `lw $v1,0xA2C($gp)` / `swc1 $f0,0x38E8($gp)`, and the two stores to 0x001B28D0
   and 0x001B28D4 stay absolute.  (The arrays 0x001B28B0/C0/C8 must be declared
   *without* `sda`; with the attribute they would come out `$gp`-relative in a -G8
   unit, where retail has `lui`/`addiu`.)

   What is left is the allocation: retail puts the counter in $v0, the three
   pointers in $v1/$a0/$a1 and the two constants in $a2/$a3; this cc1 always puts
   the three pointer addresses in $v0/$v1/$a0 and the counter in $a1, whatever the
   order of the declarations and of the first uses (twelve spellings measured:
   the counter before or after the pointers, the pointers in both orders, the
   index form `D[3-i]`, for / while / do-while, the increments in the body or in
   the for-clause).  The trailing two loads also follow the allocation: retail
   reuses $v0/$v1 for them and $at for the addresses, this cc1 uses $t1/$t0 and
   $v0/$v1.

   Wave 12 (worker s4). */
extern short D_L00_001B28B0[];
extern short D_L00_001B28C0[];
extern short D_L00_001B28C8[];
extern int D_L00_001B28D0 __attribute__((nosda));
extern int D_L00_001B28D4 __attribute__((nosda));
extern int D_L00_001AFA18 __attribute__((sda));
extern int D_L00_001AFA1C __attribute__((sda));
extern float D_L00_001B22D8 __attribute__((sda));

void func_L00_003B4E88(void)
{
    int i = 3;
    short *b0 = D_L00_001B28B0;
    short *b1 = D_L00_001B28C0;
    short *b2 = D_L00_001B28C8;

    for (; i >= 0; i--) {
        b0[0] = 2;
        b1[0] = 2;
        b2[0] = 1;
        b0++;
        b1++;
        b2++;
    }
    D_L00_001B28D0 = D_L00_001AFA18;
    D_L00_001B28D4 = D_L00_001AFA1C;
    D_L00_001B22D8 = 1.0f;
}
