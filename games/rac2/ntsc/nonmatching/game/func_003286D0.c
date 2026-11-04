/* func_003286D0 - 132 bytes, 28 places (the boot and every level).
   Retail at 0x003286D0: the shared helper is asked to place the argument's
   vector (func_002761F8(arg, s0+0x10, 0, s0, 0)); when it refuses the body
   answers 0, and otherwise the object the level names is read (the word at
   0x1BF218) and, unless it is one of the two excluded ones (its +0xAA word
   is 0xFB0, or it is the object at 0x18C0B0), the resident vector at 0x1BF220
   is copied into s0+0x10 and the body answers 1.
   The candidate below is that body, and with the opt-in -frac2-qcopy profile
   its 16-byte copy comes out in retail's shape (addiu $a0,$s0,0x10 ;
   lq $v0,0($v1) ; sq $v0,0($a0)), so the unit would need that flag in
   config/cflags.txt. With the unit's default profile two things differ: the
   copy's destination is folded into the store (sq $v1,0x10($s0)) instead of
   being materialised, and the branch layout differs - retail jumps to the
   return-0 block for the first two tests and to the epilogue for the third,
   while this cc1 inverts the second test (bne) and keeps both returns in the
   middle of the body. */
typedef int TI128 __attribute__((mode(TI)));

extern int func_002761F8(void *a, TI128 *b, int mode, void *self, int flag);

extern int ref_L00_003286D0_00;

extern char ref_L00_003286D0_01[];

extern int D_0018C0B0;

int func_003286D0(char *s0, void *a1)
{
    int a0;

    if (func_002761F8(a1, (TI128 *)(s0 + 0x10), 0, s0, 0) == 0)
        return 0;
    a0 = ref_L00_003286D0_00;
    if (a0 != 0) {
        if (*(short *)(a0 + 0xAA) == 0xFB0)
            return 0;
        if (a0 == D_0018C0B0)
            return 0;
    }
    *(volatile TI128 *)(s0 + 0x10) = *(TI128 *)ref_L00_003286D0_01;
    return 1;
}
