/* func_00332590 - near miss (wave 2, worker 5).
 *
 * The retail body (boot 0x00332590, 196 bytes; the same code is in the 27 levels):
 *
 *   the frame is 0x40 and $s0-$s5 are saved; the arguments go to $s5 (a0), $s3 (a1),
 *   $s4 (a2) and $s2 (a3);
 *   jal func_29EFD0 with 0xEAB; the result is $s1;
 *   beqz $s1 -> the epilogue with 0xFF in the delay slot (the null case returns 0xFF);
 *   lw $s0,0x68($s1)                    ; the sub-object
 *   sb 0xFF -> 0x30($s1) ; sh 0xFF -> 0x32($s1) ; sb 0 -> 0x20($s1)
 *   addiu $v1,$s1,0x10 ; lq $v0,0($s3) ; sq $v0,0($v1)   ; a 16-byte copy to $s1+0x10
 *   lq $v0,0($s2) ; sq $v0,0($s0)                     ; a 16-byte copy to the sub-object
 *   func_2FED90($s1, 0, 1.0f, 1.0f, 1.0f) ; its result -> 0x30($s0)
 *   func_282CF0($s0+0x20, $s4, $s3)
 *   sw $s5 -> 0x34($s0) ; the return value is $s1
 *
 * The whole body is recovered.  What is left is the materialisation of the two
 * 16-byte copies and the scheduling around them:
 *
 *  - the default profile folds the constant offset of the first copy into the
 *    store (`sq $v0,0x10($s1)`), where the retail materialises the destination
 *    address first (`addiu $v1,$s1,0x10` ; `sq $v0,0($v1)`).  The `-frac2-qcopy`
 *    profile gives exactly the retail's form (`addiu $a2,$s1,0x10` ;
 *    `sq $v0,0($a2)`), so the function needs that flag in a unit of its own;
 *  - with `-frac2-qcopy` the register allocation is still $s2 = a1 and $s3 = a3
 *    where the retail has $s2 = a3 and $s3 = a1; adding `-fno-schedule-insns`
 *    fixes that (the second scheduler is what moves the value between the
 *    registers) and also puts the two copies in the retail's order;
 *  - the compiled body is 208 bytes against the retail's 196: the constant 0xFF is
 *    materialised twice (the retail keeps one register), the argument setup of
 *    func_2FED90 is hoisted in front of the copies, and the 1.0f constant comes
 *    early.
 *
 * The measured profile for the coordinator (the function has to live in a file of
 * its own, the flag is per file):
 *
 *   src/game/00332590-rc2.c	-O2 -G0 -ffunction-sections -frac2-qcopy -fno-schedule-insns
 *
 * Evidence: work/relcmp.py work/c_32590b.c func_00332590 with that profile gives
 * 208 bytes and the first difference at +0x3c; without -frac2-qcopy the copies
 * fold into the store (204 bytes, +0x14) and without -fno-schedule-insns the
 * allocation differs (+0x14).
 */
typedef int TI128 __attribute__((mode(TI)));

extern int func_29EFD0(int);
extern int func_2FED90(void *, int, float, float, float);
extern void func_282CF0(void *, void *, void *);

int func_00332590(void *a0, void *a1, void *a2, void *a3)
{
    int obj = func_29EFD0(0xEAB);
    void *sub;

    if (obj == 0)
        return 0xFF;
    sub = *(void **)((char *)obj + 0x68);
    *(unsigned char *)((char *)obj + 0x30) = 0xFF;
    *(unsigned short *)((char *)obj + 0x32) = 0xFF;
    *(unsigned char *)((char *)obj + 0x20) = 0;
    *(TI128 *)((char *)obj + 0x10) = *(TI128 *)a1;
    *(TI128 *)sub = *(TI128 *)a3;
    *(int *)((char *)sub + 0x30) = func_2FED90((void *)obj, 0, 1.0f, 1.0f, 1.0f);
    func_282CF0((char *)sub + 0x20, a2, a1);
    *(int *)((char *)sub + 0x34) = (int)a0;
    return obj;
}
