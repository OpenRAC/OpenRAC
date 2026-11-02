#!/usr/bin/env python3
"""Reload never inherits into a register that is not a spill register of the insn.

The archive's reload1.c (2.9-ee-991111b) starts reload_reg_free_for_value_p with

    if (TEST_HARD_REG_BIT (reload_reg_used, regno))
      return 0;

and choose_reload_regs puts every register outside the insn's spill set into reload_reg_used
(IOR_COMPL_HARD_REG_SET (reload_reg_used, chain->used_spill_regs)). A hard register that a
pseudo holds in the insn, such as the output of `slt` given $v0 by local-alloc, is therefore
never taken over as an inherited reload register. Patch 0047 (the GCC 2.95.2 reload1.c) drops
the test, so the tree inherited the value of a spilled pseudo left in $v0 by the previous
insn's output reload and wrote `slt $v0,$v0,128` where retail reloads: `lw $v1,slot($sp)`,
`slt $v0,$v1,128`. This puts the test back; nothing else of 0047 changes.

Retail witnesses: func_L00_003C5B88 and its 22 copies (the count reload after `n++`, and the
round-robin register choice of the index reloads after it) become exact with it. Every matched
function of all 28 programs compiles to the same bytes with and without it.
tests/test_reload_reg_used_guard.py covers the anchor and the reload; see docs/COMPILER-NOTES.md.

Usage: reload_reg_used_guard.py <gcc/reload1.c>
"""
import sys
from pathlib import Path

OLD = ("  int time1;\n"
       "  int i;\n"
       "  int copy = 0;\n"
       "\n"
       "  if (out == const0_rtx)\n"
       "    {\n"
       "      copy = 1;\n")
NEW = ("  int time1;\n"
       "  int i;\n"
       "  int copy = 0;\n"
       "\n"
       "  /* RAC2: as the 991111b archive has it (reload_reg_used_guard.py).\n"
       "     reload_reg_used holds the registers that are not available as\n"
       "     spill registers, including hard registers that are earlyclobbered\n"
       "     in asms.  */\n"
       "  if (TEST_HARD_REG_BIT (reload_reg_used, regno))\n"
       "    return 0;\n"
       "\n"
       "  if (out == const0_rtx)\n"
       "    {\n"
       "      copy = 1;\n")


def patch(text: str) -> str:
    if "reload_reg_used_guard.py" in text:
        raise ValueError("reload_reg_free_for_value_p already patched")
    if text.count(OLD) != 1:
        raise ValueError(f"reload_reg_free_for_value_p: expected one anchor, found {text.count(OLD)}")
    return text.replace(OLD, NEW)


def main() -> None:
    path = Path(sys.argv[1])
    path.write_text(patch(path.read_text(encoding="utf-8")), encoding="utf-8", newline="")
    print("reload_reg_free_for_value_p: no inheritance into a non-spill register")


if __name__ == "__main__":
    main()
