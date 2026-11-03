"""Count an asm statement and the grouped qcopy by their words in loop padding.

The retail assembler lengthens a short loop by counting machine words. The
post-delay-slot padding pass counts insns, so a two-instruction `qcopy` asm
or the grouped `rac2_qcopy` pattern (lq/sq) was counted as one word and its
loop received one NOP too many. Both report their word count through the
length attribute (asm_insn_count for an asm; "length" 2 for rac2_qcopy).
"""
from pathlib import Path
import sys

path = Path(sys.argv[1])
source = path.read_text(encoding="utf-8")
start = source.index("void\nmips_r5900_pad_loops (first)")
end = source.index("/* ASTRA P25:", start)
fragment = source[start:end]
old = "\t  n += pat == TRAP_IF ? get_attr_length (insn) : 1;"
new = ("\t  /* An asm statement and the grouped qcopy occupy their word count,\n"
       "\t     which is what the assembler counts. */\n"
       "\t  n += (pat == TRAP_IF || pat == ASM_INPUT\n"
       "\t\t|| asm_noperands (PATTERN (insn)) >= 0\n"
       "\t\t|| recog_memoized (insn) == CODE_FOR_rac2_qcopy)\n"
       "\t       ? get_attr_length (insn) : 1;")
# After count_block_move_length.py the counter is already a multi-line condition.
old_bm = "\t  n += (pat == TRAP_IF\n"
new_bm = ("\t  n += (pat == TRAP_IF || pat == ASM_INPUT\n"
          "\t\t|| asm_noperands (PATTERN (insn)) >= 0\n"
          "\t\t|| recog_memoized (insn) == CODE_FOR_rac2_qcopy\n")
if fragment.count(old) == 1:
    fragment = fragment.replace(old, new)
else:
    assert fragment.count(old_bm) == 1, "Unexpected or already patched loop counter"
    fragment = fragment.replace(old_bm, new_bm)
path.write_text(source[:start] + fragment + source[end:], encoding="utf-8", newline="")
print("Asm statements and rac2_qcopy use their word count in post-DBR padding")
