/* sol-low audit, 2026-10-28: all 6 USA v1.01 placements follow a completed jr $ra and its stack-restoring delay slot plus one nop. Scanned every loaded section for aligned literal pointers and text for direct branch/jump targets into the entire extent: none. Level 05 contains three unmatched +0x30 stack restores and an orphan sh $zero,0x1c($a0). The next real prologue is at 0x33CD60. This supports CATALOGUE, rather than a valid tail call or noreturn body. No catalogue edits made. */
/* Not a C row: a debris mix, not one function - `addiu $sp,$sp,0x30; nop; addiu $sp,$sp,0x30;
   nop; sh $zero,0x1C($a0); nop; addiu $sp,$sp,0x30; nop`, level 05 (6 places). The row opens on
   stack restores (impossible as a prologue) and the lone `sh` is a stray body word; the previous
   row func_L00_00326048 ends at 0x33CD3C and the real entry after the gap is func_002B0A18 at
   0x33CD60.
   Maintainer data: no C spelling; 28 dead bytes. */
