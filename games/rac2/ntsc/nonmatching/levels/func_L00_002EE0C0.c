/* from src/levels/shared/L00_002E5918.c: retail, 16 bytes, 27 places:
   sd $zero,0($a0) ; nop ; jr $ra ; sw $zero,8($a0) - a twelve-byte clear of
   the object at $a0.  The candidate gives the two stores in the retail's order
   (the 8-byte store first) but is 12 bytes: sd $zero,0($a0) ; jr $ra ;
   sw $zero,8($a0).  The missing word is the `nop` between the first store and
   the jump, the same wall as func_L00_002EDCB8 and its three twins.
   Measured this wave (wave 8 w06): -O0, -O1, -O2, -O3, -fno-schedule-insns
   and -fno-delayed-branch (which gives the two stores first and the `nop` in
   the return slot) never give the retail's 16 bytes; the store order follows
   the source order, so the fields must be cleared in the order `second` then
   `first`.  Assembling the retail's exact four instructions with the game's
   `as` gives 12 bytes too, so the `nop` is not the assembler's nop: the
   compiler has to emit it. */
struct ResetBlock {
    long long first;
    int second;
};

void func_L00_002EE0C0(void *a0) {
    struct ResetBlock *p = (struct ResetBlock *)a0;
    p->second = 0;
    p->first = 0;
}
