/* func_002C2FD8 - near miss (not linked).
 *
 * Boot 0x002C2FD8, 288 bytes, 28 places (boot + 27 levels).
 *
 * Measured with the profile `-O2 -G0 -ffunction-sections -frac2-qcopy`:
 * the retail's 16-byte copy keeps both addresses in registers
 * (`addiu $v1,$s0,0x10` / `lq $v0,0($s2)` / `sq $v0,0($v1)`), the default
 * profile folds the destination offset into the `sq`.  The matched functions
 * of 002BF6F0.c use the folded form (`lq $v0,0x10($s0)`), so this function
 * cannot live in that unit: it needs a unit of its own with
 *
 *     src/game/002BF6F0-qcopy.c	-O2 -G0 -ffunction-sections -frac2-qcopy
 *
 * What matches: the whole prologue (the frame, the seven parameter copies in
 * the retail's order, the `andi` of the byte argument, the 0x3B in the call's
 * delay slot), the allocation, the null test, the `func_00282CF0` call and
 * its arguments, the colour word, the byte at 9, the 0x44/0x48 choice, the
 * 210000.0f scale, the table lookup and the byte at 2.
 *
 * What is left (292 bytes against 288): the 16-byte copy is scheduled before
 * the `p6 == 1` test instead of after the `func_00282CF0` call, and the store
 * of `p6` is a separate instruction instead of the branch's delay slot.  Both
 * follow from the order of the three statements at the head of the body:
 * putting the copy first gives the retail's prologue but the copy is then
 * scheduled early; putting the store first gives the retail's body but the
 * prologue's parameter copies come out in another order (the `t2` copy is
 * fourth instead of second, and the 0x3B constant moves into the prologue).
 */
extern char *func_002B9030(int size);
extern int func_00283CF0_6(float f);
extern void func_00282CF0(char *, char *, char *);
extern char D_00189EA0[];
extern unsigned char *D_001F2540[];

char *func_002C2FD8(char *p0, char *p1, unsigned char p2, int p3, char *p4,
                    int p5, int p6, float f0)
{
    char *s0 = func_002B9030(0x3B);
    char *sub = s0 + 0x20;

    if (s0 != 0) {
        *(V4_Fc3cb9262 *)(s0 + 0x10) = *(V4_Fc3cb9262 *)p0;
        *(int *)(sub + 0x10) = p6;
        if (p6 == 1)
            func_00282CF0(sub, p0, D_00189EA0);
        *(int *)(s0 + 4) = p1;
        *(unsigned char *)(s0 + 9) = func_00283CF0_6(1.0f) + 0x20;
        *(unsigned char *)(s0 + 3) = (p4 != 0) ? 0x44 : 0x48;
        *(unsigned char *)(s0 + 8) = p2;
        *(float *)(s0 + 0xC) = f0 * 210000.0f;
        *(unsigned char *)(s0 + 1) = 0;
        *(unsigned char *)(s0 + 2) = *(unsigned char *)D_001F2540[p3];
        *(unsigned short *)(s0 + 0xA) = p5;
    }
    return s0;
}
