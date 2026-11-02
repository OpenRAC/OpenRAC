/* func_00329DC0 (USA v1.01 boot, 156 bytes, 28 places): validates the move set stored at
   self+0x10 against the current collision object, then copies the identity orientation into
   self+0x10 and its state+0x20.

   Near miss: 152 bytes, 0 of 28 places match, first difference +0x30 - the whole tail from the
   `beqz $v0` after the call is retail's and only the prologue's register plan differs. Retail
   keeps `self` in `$s0` (`daddu $s0,$a0,$zero` ; `lw $s1,0x68($s0)` ; `addiu $a1,$s0,0x10` ;
   `daddu $a3,$s0` in the call's delay slot) and recomputes `$s0+0x10` after the call
   (`addiu $a1,$s0,0x10` inside the copy block). This C keeps `self` in `$a3`, hoists the argument
   address `self+0x10` into `$s0` and reuses it for both copies, so it never materialises the
   second `addiu`. Measured wordings that give the same 152 bytes: nested `if`s (which also invert
   the branches), one `&&`/`||` condition, and a local `V4 *q = (V4 *)(self + 0x10)`. A variant that
   copies the parameter into a local (`char *self = (char *)owner;`) in the hope of keeping `self`
   in `$s0` was written but not measured.
   Three facts pinned here: func_002761F8 takes five arguments (the fifth is the `0` in
   `$t0`, so the prototype has to declare it); `D_001BAF98` is an array of pointers whose first
   element is the current collision object; and the two 16-byte copies come out as `lq`/`sq` only
   under the `-frac2-qcopy` profile that src/game/00327038.c needs in general. */
typedef int V4_qcopy_00329DC0 __attribute__((mode(TI)));

extern unsigned char *D_001BAF98[];
extern char D_001CAFA0[];
extern void *D_0019C0B0;
extern int func_002761F8(void *, void *, int, void *, int);

int func_00329DC0(void *self, void *arg)
{
    char *state = *(char **)((char *)self + 0x68);
    char *object;

    if (!func_002761F8(arg, (char *)self + 0x10, 0, self, 0))
        return 0;
    object = (char *)D_001BAF98[0];
    if (object == 0
        || (*(short *)(object + 0xAA) != 0xFB0 && object != (char *)D_0019C0B0)) {
        *(V4_qcopy_00329DC0 *)((char *)self + 0x10) = *(V4_qcopy_00329DC0 *)D_001CAFA0;
        *(V4_qcopy_00329DC0 *)(state + 0x20)
            = *(V4_qcopy_00329DC0 *)((char *)self + 0x10);
        return 1;
    }
    return 0;
}
