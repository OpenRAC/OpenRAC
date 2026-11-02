/* func_0029CFB0 - 160 bytes, 28 places (the boot and every level), retail at
   0x0029CFB0: lui $v0,0x1000 ; addi $v1,$zero,0x400 ; ori $at,$v0,0xD400 ;
   ori $v0,$v0,0xD000 ; ori $t7,$zero,0x100 ; nop ; pminw $a3,$a2,$v1 ;
   sw $zero,0x80($at) ; sw $a3,0x20($at) ; sll $t0,$a3,4 ; sw $a1,0x10($at) ;
   add $a1,$a1,$t0 ; sw $t7,0x0($at) ; sub $a2,$a2,$a3 ; lw $t1,0x0($at) ;
   nop ; nop ; nop ; andi $t1,$t1,0x100 ; nop ; bnez $t1,-7 ; nop ;
   sw $zero,0x80($v0) ; sw $a3,0x20($v0) ; sw $a0,0x10($v0) ; add $a0,$a0,$t0 ;
   sw $t7,0x0($v0) ; nop ; lw $t1,0x0($v0) ; nop ; nop ; nop ;
   andi $t1,$t1,0x100 ; nop ; bnez $t1,-7 ; nop ; bgtz $a2,0x0029cbc8 ;
   nop ; jr $ra ; nop.

   The body is hand-written assembly, not C.  Measured (wave 4, worker 3):

   1. The word at 0x0029CFB8+0x10 (0x70C338E8) is the MMI `pminw $a3,$a2,$v1`
      - the vector unit's word-wise minimum of the count and 0x400.  This cc1
      has no vector support at all: `__attribute__((vector_size(16)))` is
      ignored with "vector_size attribute directive ignored", so there is no
      type that reaches the MMI unit and no way to emit a 0x70xxxxxx word.
      The scalar minimum costs three words instead of one
      (`li`+`slt`+`movn`, or `li`+`li`+`slt`+`movz` for the if/else spelling).
   2. The retail's `addi $v1,$zero,0x400` and `add $a1,$a1,$t0` /
      `add $a0,$a0,$t0` are the trapping variants; this cc1 emits `addiu` and
      `addu`.  The matched witness of the same DMA pattern,
      src/core/0012D808.c (func_0012D808), has `addiu $v1,$zero,0x101` and
      `addu $s2,$s1,$fp` - the retail's own compiler writes the same
      constants the same way there.
   3. The retail's poll loop puts its three nops *before* the `andi`
      (lw ; nop ; nop ; nop ; andi ; nop ; bnez), this cc1 puts them *after*
      it (lw ; andi ; nop ; nop ; nop ; bnez).  The matched witness
      func_0012D808 has the compiler's shape (lw ; andi ; nop ; nop ; nop ;
      bnez), so the difference is the writer, not the compiler.
   4. The retail keeps the SPR block's address in `$at` and the DMA channel
      0 address in `$v0`; this cc1 never allocates `$at` as a general
      register.
   5. The last branch, `bgtz $a2,0x0029cbc8`, jumps into the *middle* of
      func_0029CBA8 (0x0029CBA8 + 52 bytes): 0x0029cbc8 is the `jal
      func_34CE98` of that function's body, so the tail call skips the
      callee's prologue and its null check.  No C source can express a jump
      into another function's body; a C tail call would be a `jal` to the
      callee's entry and would need a frame, and the retail body has no
      frame at all.
   6. The retail keeps two dead pointer advances (`add $a1,$a1,$t0` and
      `add $a0,$a0,$t0`; the destination advance is read by nobody), which
      this cc1 deletes as dead assignments.
   7. The first poll loop's branch target is 0x0029cfec, one word *after* the
      load at 0x0029cfe8, so that loop never re-reads the status word; the
      second loop's target (0x0029d020) is the load itself.  The asymmetry is
      a hand-written quirk.

   The candidate below is the DMA sequence the retail's words describe: the
   MMIO blocks at 0x1000D400 and 0x1000D000, the count
   clamped to 0x400 blocks of 16 bytes, and the two poll loops, written the
   way the matched helper src/core/0012D808.c writes the same blocks.  It
   compiles to 204 bytes against the retail's 160 (152 bytes without the tail
   call, which then costs the two pointer advances and the count
   subtraction).  What would unblock it: the MMI unit's own instructions and
   a tail jump into another function - that is, hand-written assembly.

   Recommendation: this is a candidate for ASM_FUNC ("asm/game",
   func_0029CFB0) - the body uses instructions the compiler never emits (the
   MMI `pminw`), uses `$at` as a working register and jumps into the middle
   of another function.  A maintainer has to agree before the marker
   changes. */

extern void func_0034CE98(void *, int);

void func_0029CFB0(unsigned int dst, unsigned int src, int count)
{
    volatile unsigned int *spr = (volatile unsigned int *)0x1000D400;
    volatile unsigned int *d0 = (volatile unsigned int *)0x1000D000;
    int blocks = count < 0x400 ? count : 0x400;
    int bytes = blocks * 16;

    spr[0x20] = 0;
    spr[0x8] = blocks;
    spr[0x4] = src;
    src += bytes;
    spr[0] = 0x100;
    while (spr[0] & 0x100) {
    }

    d0[0x20] = 0;
    d0[0x8] = blocks;
    d0[0x4] = dst;
    dst += bytes;
    d0[0] = 0x100;
    while (d0[0] & 0x100) {
    }

    count -= blocks;
    if (count > 0)
        func_0034CE98((void *)dst, (int)src);
}
