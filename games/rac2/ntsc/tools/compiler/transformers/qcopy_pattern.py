"""Add an opt-in grouped R5900 copy for ordinary TImode memory assignments.

The pattern keeps both addresses in registers and declares its v0 scratch.
It preserves the memory operands' flags and does not add a memory barrier.
Default code generation, scalar transfers and zero stores are unchanged.
"""
from pathlib import Path
import sys


EXPANSION = '''  /* Keep a complete aligned copy together, with explicit address operands.
     change_address preserves the source memory flags, including volatile. */
  if (flag_rac2_qcopy && Pmode == SImode
      && !reload_in_progress && !reload_completed
      && GET_CODE (operands[0]) == MEM && GET_CODE (operands[1]) == MEM
      && !MEM_UNALIGNED_P (operands[0]) && !MEM_UNALIGNED_P (operands[1]))
    {
      rtx dest = force_reg (Pmode, XEXP (operands[0], 0));
      rtx src = force_reg (Pmode, XEXP (operands[1], 0));
      rtx copy = gen_rac2_qcopy (dest, src);
      rtx set = XVECEXP (copy, 0, 0);
      SET_DEST (set) = change_address (operands[0], TImode, dest);
      SET_SRC (set) = change_address (operands[1], TImode, src);
      MEM_ALIAS_SET (SET_DEST (set)) = MEM_ALIAS_SET (operands[0]);
      MEM_ALIAS_SET (SET_SRC (set)) = MEM_ALIAS_SET (operands[1]);
      emit_insn (copy);
      DONE;
    }

'''

PATTERN = r''';; A grouped aligned copy, not an opaque asm or a memory clobber.
;; The length attribute in this backend counts instructions, not bytes.
(define_insn "rac2_qcopy"
  [(set (mem:TI (match_operand:SI 0 "register_operand" "d"))
        (mem:TI (match_operand:SI 1 "register_operand" "d")))
   (clobber (reg:TI 2))]
  "TARGET_MIPS5900 && flag_rac2_qcopy"
  "lq $2,0(%1)\;sq $2,0(%0)"
  [(set_attr "type" "multi")
   (set_attr "length" "2")
   (set_attr "mode" "DI")])

'''


def replace_once(text: str, before: str, after: str, label: str) -> str:
    if text.count(before) != 1:
        raise ValueError(f"{label}: expected one insertion point")
    return text.replace(before, after)


def patch(root: Path) -> None:
    flags, top, md = root / "flags.h", root / "toplev.c", root / "config/mips/mips.md"
    if any("flag_rac2_qcopy" in path.read_text() for path in (flags, top, md)):
        raise ValueError("qcopy option already present")
    declarations = replace_once(flags.read_text(), "extern int flag_rac2_zero_ti_barrier;",
        "extern int flag_rac2_zero_ti_barrier;\nextern int flag_rac2_qcopy;", "option declaration")
    driver = replace_once(top.read_text(), "int flag_rac2_zero_ti_barrier = 0;",
        "int flag_rac2_zero_ti_barrier = 0;\nint flag_rac2_qcopy = 0;", "option definition")
    anchor = '  {"rac2-zero-ti-barrier", &flag_rac2_zero_ti_barrier, 1,'
    driver = replace_once(driver, anchor,
        '  {"rac2-qcopy", &flag_rac2_qcopy, 1,\n'
        '   "Use a grouped R5900 quadword-copy pattern"},\n' + anchor, "option table")
    machine = md.read_text()
    begin = machine.index('(define_expand "movti"')
    end = machine.index('(define_insn "movti_internal"', begin)
    expansion = machine[begin:end]
    anchor = "  /* Make operand1 a register if it isn't already.  */"
    expansion = replace_once(expansion, anchor, EXPANSION + anchor, "TImode expansion")
    machine = machine[:begin] + expansion + PATTERN + machine[end:]
    # Check every anchor before writing any source: drift must leave the tree intact.
    for path, contents in ((flags, declarations), (top, driver), (md, machine)):
        path.write_text(contents, encoding="utf-8", newline="")


if __name__ == "__main__":
    patch(Path(sys.argv[1]))
