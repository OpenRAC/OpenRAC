/* Near miss: the load path is the retail's, but the prologue's order and the
   empty path's handle address differ.

   func_002B6828, boot 0x002B6828, catalogue size 152 (0x98).
   Profile: the game's default `-O2 -G0 -ffunction-sections`.

   The retail bytes are

        002b6828  addiu  $sp,$sp,-0x10
        002b682c  lui    $v1,0x15
        002b6830  sd     $ra,0($sp)
        002b6834  addiu  $a2,$v1,-0x4AC0
        002b6838  sll    $v0,$a0,3
        002b683c  addu   $v0,$a2,$v0
        002b6840  lw     $t0,0x52E0($v0)
        002b6844  beqz   $t0,. + 4 + (0x15 << 2)
        002b6848  lui    $a1,0x19
        002b684c  sll    $a3,$a0,2
        002b6850  addiu  $a1,$a1,-0x79A0
        002b6854  addiu  $v1,$a0,0x1
        002b6858  addiu  $v0,$a1,0x17A4
        002b685c  sll    $v1,$v1,2
        002b6860  addu   $a3,$a3,$v0
        002b6864  lw     $a0,0x529C($a2)
        002b6868  lui    $v0,0xFFFF
        002b686c  addu   $a1,$a1,$v1
        002b6870  ori    $v0,$v0,0xFFFF
        002b6874  dsll32 $a3,$a3,0
        002b6878  lui    $a2,0x2B
        002b687c  sw     $v0,0x17A0($a1)
        002b6880  addu   $a0,$t0,$a0
        002b6884  addiu  $a2,$a2,0x6770
        002b6888  dsrl32 $a3,$a3,0
        002b688c  jal    func_00132498
        002b6890  daddu  $a1,$zero,$zero
        002b6894  b      . + 4 + (0x8 << 2)
        002b6898  ld     $ra,0($sp)
        002b689c  addiu  $v1,$a0,0x1
        002b68a0  lui    $v0,0x19
        002b68a4  addiu  $v0,$v0,-0x79A0
        002b68a8  sll    $v1,$v1,2
        002b68ac  addu   $v0,$v0,$v1
        002b68b0  sw     $zero,0x17A0($v0)
        002b68b4  ld     $ra,0($sp)
        002b68b8  jr     $ra
        002b68bc  addiu  $sp,$sp,0x10

   The level's table of contents holds the bank's sector at 0x0014B540 + 0x52E0
   + index*8 and the level's base sector at 0x0014B540 + 0x529C; the handle the
   load leaves behind is the word at 0x00188660 + 0x17A0 + (index+1)*4, the one
   after the global bank's; the load's second argument is zero and its user
   data is the handle's address as a 64-bit value.

   This C compiles to 144 bytes, eight fewer than the retail, and matches 0 of
   28 places.  The load path is the retail's - the sector pair, the base
   sector, the -1 sentinel before the call, the zero second argument, the
   64-bit user data and the empty slot written as zero - and the store's
   address keeps the retail's 0x17A0 displacement.  What is left is
   allocation: the prologue's order (the retail opens with the frame and
   saves `$ra` before it materialises the table's address, this cc1
   materialises an address first), and the empty path, where the retail
   rebuilds the handle's base from scratch in six instructions while this cc1
   keeps the base in a register across the branch and needs four.  The user
   data is spelled the other way round as well: the retail adds the index to a
   base that already holds 0x17A4, this cc1 adds 0x17A4 to a base that holds
   (index+1)*4.

   Tried: writing the user data with the same expression as the store (144
   bytes, the store's displacement folded to 0x17A4 and the empty path reused
   the base); writing it as base + index*4 + 0x17A4 (128 bytes, both stores
   folded into the index*4 form); a slot pointer for the store (140 bytes, the
   store kept the retail's displacement but the empty path still reused the
   base); the slot pointer declared in each branch (144 bytes, the best of the
   four).
*/

extern char D_00188660[];
extern int D_0014B540[];
extern void func_002B6770(int, long);
extern void func_00132498(void *, int, void (*)(int, long), unsigned long);

void func_002B6828(int index)
{
    char *b = D_00188660;
    int *d = D_0014B540;
    int lbn = d[0x14B8 + index * 2];

    if (lbn != 0) {
        char *slot = b + (index + 1) * 4;
        *(unsigned int *)(slot + 0x17A0) = 0xFFFFFFFFu;
        func_00132498((void *)(lbn + d[0x14A7]), 0, func_002B6770,
                      (unsigned long)(unsigned int)(b + index * 4 + 0x17A4));
    } else {
        char *slot = b + (index + 1) * 4;
        *(unsigned int *)(slot + 0x17A0) = 0;
    }
}
