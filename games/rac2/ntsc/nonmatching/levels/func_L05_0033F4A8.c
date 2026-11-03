/* sol-low audit, 2026-10-28: all 5 USA v1.01 placements follow a completed jr $ra and its stack-restoring delay slot. Scanned every loaded section for aligned literal pointers and text for direct branch/jump targets into the entire extent: none. Level 05 contains unmatched positive stack restores and an orphan lw $v0,0x18($v1) at +0x38. The next real prologue is at 0x33F548. This supports CATALOGUE, rather than a valid tail call or noreturn body. No catalogue edits made. */
/* Not a C row: same "addiu $sp,$sp,+N ; nop" opener as func_L00_00358670 in this lot.
   Retail at 0x33F4A8, level 05 (5 places) opens `addiu $sp,$sp,0x30; nop; addiu $sp,$sp,0x50; nop;
   addiu $sp,$sp,0x70` - 156 bytes of restores and padding. A function cannot begin by adding to the
   stack pointer, so the extent is a catalogue split.
   Maintainer data: no C spelling; the 156-byte row is dead bytes between real entries. */
