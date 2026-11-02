/* Not landable: this body reaches a 128-bit copy through `qcopy`/`qzero` or a local
   `__asm__` helper, and AGENTS.md's "Plain C" rule (no inline assembly in a function)
   forbids that as a matching route. Kept as a starting point. Removed from src/ 2026-10-13. */

func_L20_0039C540(char *arg0, char *arg1, char *arg2, float f12) {
    char *v;
    char *o = func_0029EFD0(0x12F2);

    if (o != 0) {
        *(int *)(o + 0xB8) = (int)arg0;
        qcopy(o + 0x10, arg1);
        v = *(char **)(o + 0x68);
        *(unsigned char *)(o + 0x30) = 0xFF;
        qcopy(v + 0x10, arg2);
        *(float *)(v + 0x2C) = f12;
        *(int *)(v + 0x20) = 0x12C;
    }
    return o;
}

/* ------------------------------------------------------------------
   Second attempt (worker 07, level-20 wave of 2026-10-20): the C
   below replaces the `qcopy` draft above (which needs inline assembly
   and is therefore not a legal matching route under AGENTS.md).  The
   16-byte copies are written with a 128-bit integer type, which is the
   only spelling that reaches `lq`/`sq`.

   Result: 132 bytes against the retail's 140.  Everything matches
   except the two 128-bit stores and the order of a few instructions:

     retail: addiu $v1,$a2,0x10 ; lq $v0,0x0($s1) ; sq $v0,0x0($v1)
     this cc1:                     lq $v0,0x0($s1) ; sq $v0,0x10($a2)

   The retail materialises the destination pointer `o + 0x10` into $v1
   (in the delay slot of the `beqz $a2`) and stores through it; this
   cc1 folds the constant offset into the `sq`.  Same wall as in
   `func_L20_0039C448`: gcc/cse.c's `find_best_addr` picks the
   `reg + const` address over the register (both cost 1 under MIPS's
   `ADDRESS_COST`, the tie is broken on the expression cost).  Measured
   here and all folding: the destination as a pointer variable, a
   `volatile` store, a `volatile` pointer variable (keeps the address in
   a register but spills the pointer), the destination computed before
   the `if`, a TI128 temporary, `__builtin_memcpy(...,16)`, and a 64-bit
   address (that one does keep the address in a register, but it adds a
   `dsll`/`dsra` pair and a `daddu` where the retail has `addiu`).

   The second copy (`v + 0x10`, where `v` is loaded from memory) is
   folded the same way.  The rest of the body - the frame (0x30), the
   save order (`s0, s1, s2, ra`, ascending, which is what the retail
   has), the `mov.s $f20,$f12`, the call sequence and the return value
   - matches.

   This body belongs in the `-G8` unit?  No: it reaches no global through
   `$gp`.
   ------------------------------------------------------------------ */

typedef int TI128_w07 __attribute__((mode(TI)));

char *func_L20_0039C540_w07(char *arg0, char *arg1, char *arg2, float f12) {
    char *v;
    char *o = func_0029EFD0(0x12F2);

    if (o != 0) {
        *(int *)(o + 0xB8) = (int)arg0;
        *(TI128_w07 *)(o + 0x10) = *(TI128_w07 *)arg1;
        v = *(char **)(o + 0x68);
        *(unsigned char *)(o + 0x30) = 0xFF;
        *(TI128_w07 *)(v + 0x10) = *(TI128_w07 *)arg2;
        *(float *)(v + 0x2C) = f12;
        *(int *)(v + 0x20) = 0x12C;
    }
    return o;
}

/* ------------------------------------------------------------------
   Third attempt (worker 06, level-20 wave of 2026-10-20): the wall is
   the same one as in func_L20_0039C448 and it is measured exactly; see
   the note there for the mechanism (the CSE substitutes the
   `plus(reg,const)` of the destination pointer into the address of the
   `sq` and the reload pass then keeps the offset in the instruction).

   What this body needs: `sq $v0,0x0($v1)` with `addiu $v1,$a2,0x10`
   (the destination `o + 0x10` in a register) and the same for the
   second copy (`addiu $v1,$a0,0x10` into $v1, where $a0 is the state
   pointer loaded from `o + 0x68`).  The LOADS are already in the
   retail's form, because their source is a parameter (`lq $v0,0x0($s1)`
   and `lq $v0,0x0($s2)`), and this compiler reproduces that.

   The candidate below is 132 bytes against the retail's 140: the two
   `addiu` instructions that materialise the destination pointers are
   missing, because the store's address is folded instead
   (`sq $v0,0x10($a2)` where the retail has `sq $v0,0x0($v1)`).

   The volatile-pointer escape (a `volatile` pointer object) reaches the
   retail's zero-displacement form, but it costs a stack store and a
   reload per copy: eight bytes per copy, so sixteen for the two
   copies - the body would be 148 bytes, not 140.

   This body belongs in the `-G8` unit?  No: it reaches no global
   through `$gp`.
   ------------------------------------------------------------------ */

/* ------------------------------------------------------------------
   Fourth attempt (worker 07, level-20 wave of 2026-10-20): the note on
   func_L20_0039C448 (same directory) has the wall measured out of the
   compiler's own source (build/downloads/gnu-ee-binutils-gcc-1.1.tar.gz
   holds it); the summary:

   The CSE (gcc/cse.c, `find_best_addr`) replaces the address of a 128-bit
   access by the cheapest equivalent in the class of the address pseudo.
   `plus(reg, const_int)` costs 1 (`mips_address_cost`: `SMALL_INT (plus1)
   ? 1 : 2`), the same as a plain register, and the tie goes to the plus;
   the reload then prints the offset as the displacement.  The retail
   keeps the address in a register (`addiu $v1, $a2, 0x10 ; sq $v0,0($v1)`),
   which is what the reload produces when the address is still a pseudo.

   Measured today in this level, and none of them is a register base with
   an offset: a pointer loaded from memory (the class member is a MEM,
   cost 4, so nothing is substituted); a pointer with different values in
   two branches (the class is ambiguous - but the compiler then emits
   `addu`+`addu`+`movn`, three instructions for one address); a volatile
   pointer object (the address is materialised, but the pointer is
   spilled and reloaded, eight bytes over); a symbol base (the only plain
   C spelling that reaches the zero displacement, and it needs a symbol
   address).

   So the two `addiu` the retail has and this compiler does not are the
   wall, and the record above stands: the body is 132 bytes against the
   retail's 140 and everything else matches (the frame 0x30, the save
   order `s0, s1, s2, ra`, `mov.s $f20, $f12`, the call sequence and the
   return value).

   This body belongs in the `-G8` unit?  No: it reaches no global through
   `$gp`.
   ------------------------------------------------------------------ */

/* ------------------------------------------------------------------
   level-20 wave of 2026-10-21, worker 07: the wall is confirmed and the
   escapes measured.

   THE RETAIL'S SHAPE (both copies are MATERIALISED into a register with a
   ZERO displacement, which is the corrected rule of wave 5):

     retail:  addiu $v1, $a2, 0x10 ; lq $v0,0x0($s1) ; sq $v0,0x0($v1)
     this cc1:                         lq $v0,0x0($s1) ; sq $v0,0x10($a2)

     retail:  addiu $v1, $a0, 0x10 ; lq $v0,0x0($s2) ; sq $v0,0x0($v1)
     this cc1:                         lq $v0,0x0($s2) ; sq $v0,0x10($a0)

   Both bases are runtime pointers (`o`, the allocator's result, and the
   state pointer loaded from o+0x68), so both addresses are `base + const`
   with a runtime base - the case the corrected rule says plain C never
   reproduces.

   MEASURED THIS WAVE (in a scratch unit, with the game's cc1):

     *(TI128 *)(o + 0x10) = *(TI128 *)src;        -> sq $v0,0x10($o)      (folded)
     TI128 *d = (TI128 *)(o + 0x10); *d = v;     -> sq $v0,0x10($o)      (folded)
     the same with the offset 0x2010, which is
       bigger than 0x2000 so that mips_address_cost() gives the plus the
       cost 2 and the CSE tie should break the other way -> sq $v0,0x2010($o)
       (STILL folded - the front end builds the address as plus(reg,const)
       and the RELOAD accepts it for TImode, so the CSE never sees it)
     TI128 * volatile d = (TI128 *)(o + 0x10);   -> sw $a0,0($sp) ;
                                                        lq $v0,0x0($s1) ;
                                                        lw $a0,0($sp) ;
                                                        sq $v0,0x0($a0)
       (the zero displacement, but four instructions and a 16-byte stack
       slot against the retail's three)
     a pointer LOADED FROM MEMORY (the state pointer of the second copy)
       does NOT survive: `char *v = *pp; TI128 *d = (TI128 *)(v + 0x10);
       *d = v2;` gives sq $v0,0x10($v).

   So the two `addiu` the retail has and this compiler does not are the
   wall, and the record above stands: 132 bytes against the retail's 140
   and everything else matches (the frame 0x30, the save order
   s0, s1, s2, ra, the `mov.s $f20, $f12`, the call sequence and the
   return value).

   This body belongs in the `-G8` unit?  No: it reaches no global through
   `$gp`.
   ------------------------------------------------------------------ */
