/* func_002F0FA0 - near miss (112 bytes of the retail 108).
 *
 * Clears the flags of every record of a list of 0x20-byte records.
 *
 * The retail (the boot's copy at 0x002F0FA0, the same body in every program):
 *
 *   lui    $v0, 0x1C
 *   lui    $a1, 0x1B
 *   lw     $a1, 0x21B4($a1)      <- the first record, 0x001B21B4 in the boot
 *   lw     $v1, -0x512C($v0)     <- the value, 0x001BAED4 in the boot
 *   lui    $a0, 0x1B
 *   lw     $a0, 0x21B8($a0)      <- the end of the list, 0x001B21B8 in the boot
 *   lui    $at, 0x1B
 *   sw     $v1, 0x21C4($at)      <- the value is copied here, 0x001B21C4
 *   lui    $at, 0x1B
 *   sw     $zero, 0x21E0($at)    <- 0x001B21E0 cleared
 *   lui    $at, 0x1B
 *   sw     $zero, 0x21E4($at)    <- 0x001B21E4 cleared
 *   beq    $a1, $a0, +0xC        <- an empty list: done
 *   sw     $zero, 0x31F8($gp)    <- one more global cleared, through $gp
 *   ori    $a0, $zero, 0xFFFF
 *   addiu  $v1, $zero, 0xFF
 *   sb     $v1, 0x1F($a1)        <- the record's byte at +0x1F
 *   nop
 *   sh     $zero, 0x16($a1)      <- the record's halfword at +0x16
 *   sh     $a0, 0x14($a1)        <- the record's halfword at +0x14
 *   lui    $v0, 0x1B
 *   lw     $v0, 0x21B8($v0)      <- the end of the list, read again
 *   addiu  $a1, $a1, 0x20
 *   bnel   $a1, $v0, -0x6        <- the loop, branch likely
 *   sb     $v1, 0x1F($a1)        <- the next record's byte, in the delay slot
 *   jr     $ra
 *   nop
 *
 * The list's addresses differ from program to program (they are all above the boot's
 * resident area), so the globals need ref_ names; the one at $gp - 0x31F8 is small
 * data and needs the sda attribute, the others are read with the coalesced lui + lw
 * of a -G8 unit (no attribute: nosda would give the two-register split instead).
 *
 * What is still missing, against the 112 bytes this C compiles to:
 *
 *   1. the register allocation: the retail keeps the record pointer in $a1 and the end
 *      of the list in $a0, this C keeps the pointer in $a0 and the end in $v1.  The
 *      declaration order does not move it (the end first, the pointer second and the
 *      value third were all measured).
 *   2. the two constants: the retail has 0xFFFF in $a0 and 0xFF in $v1, this C has
 *      0xFF in $a1 and 0xFFFF in $v1.
 *   3. the load order: the retail loads the pointer, the value and the end, this C
 *      loads them in another order whatever the source order is.
 *   4. one nop too many: the retail schedules the reload of the end of the list before
 *      the increment of the pointer, so the delay slot of the branch holds the store
 *      and no nop is needed.
 *
 * The body needs a -G8 unit (one global is reached through $gp), so it belongs in
 * 002EEB00-g8.c.
 *
 * 24 October 2026 update (new measurement, source retained below):
 * A -G8 sibling form that declares the four resident addresses absolutely
 * (D_001B21B4/B8/C4/E0/E4 as `nosda`, D_001B21E8 as `sda`, D_001BAED4 plain)
 * compiles to 108/108 bytes, retail's size and store order, and keeps retail's
 * loop rotation and delay-slot store. In that form the remaining differences
 * are the same two: retail puts 0xFFFF in $a0 and 0xFF in $v1; this source the
 * reverse, with the cursor in $a3 rather than $a1. Writing the byte store
 * through a signed `char *` turns 0xFF into `addiu -1`, so the byte must be
 * `unsigned char` and the halfword `unsigned short`/0xFFFF. Reversing the
 * three stores makes the constants agree but changes the store order. Hoisting
 * the constants into locals moves their materialisation before the null test
 * and gives 112 bytes. A -G0 -mno-split-addresses unit is also 112 bytes with
 * the back edge no longer rotated; qcopy is irrelevant to this body. These
 * nine measured forms do not move the two loop-invariant constants or the
 * cursor allocation, so the 112-byte retained body below is kept unchanged.
 * 28 October 2026: rewriting the cursor and end as unsigned integer addresses
 * rather than pointers also gives 112 bytes and 0/28 USA v1.01 placements. The
 * pointer/end registers and loop schedule do not move. A volatile declaration
 * for the list-end reference, matching retail's per-iteration reload, also
 * gives 112 bytes and 0/28. It reloads after advancing the cursor and still
 * leaves a nop before the branch-likely store; register allocation is
 * unchanged. The unresolved ref relocations limit interpretation of the
 * disassembly beyond this emitted shape and size.
 * 29 October 2026: a volatile end reference with an outer empty-list test and
 * a `for` loop emits 120 bytes and 0/28. The compiler adds a second end test
 * after the loop and keeps the same cursor/end register allocation, so this
 * structured loop form does not improve the candidate.
 */

extern int ref_L00_002F0FA0_00;

extern int ref_L00_002F0FA0_01;

extern int ref_L00_002F0FA0_02;

extern int ref_L00_002F0FA0_03;

extern int ref_L00_002F0FA0_04;

extern int ref_L00_002F0FA0_05;

extern int ref_L00_002F0FA0_06 __attribute__((sda));

/* Clears the flags of every record of the list. */
void func_002F0FA0(void)
{
    char *end = (char *)ref_L00_002F0FA0_01;
    char *p = (char *)ref_L00_002F0FA0_00;
    int v = ref_L00_002F0FA0_02;

    ref_L00_002F0FA0_03 = v;
    ref_L00_002F0FA0_04 = 0;
    ref_L00_002F0FA0_05 = 0;
    ref_L00_002F0FA0_06 = 0;

    if (p == end)
        return;

    do {
        *(unsigned char *)(p + 0x1F) = 0xFF;
        *(unsigned short *)(p + 0x16) = 0;
        *(unsigned short *)(p + 0x14) = 0xFFFF;
        p += 0x20;
    } while (p != (char *)ref_L00_002F0FA0_01);
}
