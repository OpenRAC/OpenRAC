#!/usr/bin/env python3
"""Reconstruct the RAC2 GNU-EE 2.9-ee-991111b compiler source tree.

Recipe (games/rac2/ntsc/docs/COMPILER-NOTES.md):
  archive gnu-ee-binutils-gcc-1.1.tar.gz (sha256 1f518043...)
  + Lombyte's cumulative sce-991111b stack (games/rac1/ntsc/patches), minus 0001
    (the "saves widening")
  + RAC2 adjustments.  Published ones are run verbatim from
    games/rac2/ntsc/scripts/compiler/.  Three earlier steps are not published;
    they are reconstructed here from the notes and from the old patch contexts
    kept in rac2-decomp history (251b0a8, 2690e43):
      U1 save width: GPR saves use 8-byte (word_mode) slots, no TImode widening
      U2 ascending GPR save/restore order (s0, s1, ..., ra), offsets unchanged
      U3 frame-order option default ON (later turned off by the published step)
      U4 the earlier no-argument rac2_mtc1_nop_ok transfer-hazard helper
Usage: rac2_recipe.py <patched-tree> <rac2-scripts-compiler-dir>
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

tree = Path(sys.argv[1])
published = Path(sys.argv[2])
mips_c = tree / "gcc/config/mips/mips.c"
mips_h = tree / "gcc/config/mips/mips.h"
tc_mips = tree / "gas/config/tc-mips.c"


def edit(path: Path, old: str, new: str, count: int = 1) -> None:
    text = path.read_text(encoding="utf-8")
    assert text.count(old) == count, (path, old[:60], text.count(old))
    path.write_text(text.replace(old, new), encoding="utf-8", newline="")


def run(*args: str) -> None:
    subprocess.run(args, check=True)


# U1: no TImode save slots (FUN_002889B8: sd $s0,0($sp); sd $ra,8($sp), frame 16).
edit(mips_c,
     "  /* Saves/restores of general purpose registers on the r5900 use\n"
     "     a mode wider than word_mode.  */\n"
     "  if (TARGET_MIPS5900)\n"
     "    mips_reg_mode[0] = TImode;\n\n",
     "")

# U2: ascending GPR order with the same slots: start at the lowest slot.
text = mips_c.read_text(encoding="utf-8")
start = text.index("  /* Save GP registers if needed.  */")
end = text.index("  /* Save floating point registers if needed.  */", start)
block = text[start:end]
old_loop = ("      for (regno = GP_REG_LAST; regno >= GP_REG_FIRST; regno--)\n")
new_loop = ("      gp_offset = end_offset;\n"
            "      for (regno = GP_REG_FIRST; regno <= GP_REG_LAST; regno++)\n")
assert block.count(old_loop) == 1
block = block.replace(old_loop, new_loop)
old_step = "\t    gp_offset -= GET_MODE_SIZE (mips_reg_mode[0]);\n"
assert block.count(old_step) == 1
block = block.replace(old_step, "\t    gp_offset += GET_MODE_SIZE (mips_reg_mode[0]);\n")
mips_c.write_text(text[:start] + block + text[end:], encoding="utf-8", newline="")

# U3: the earlier recipe enabled the P21 frame-order option by default.
edit(mips_c, "char *mips_astra_keep_frame_order;\n",
     'char *mips_astra_keep_frame_order = "1"; /* RAC2 : defaut ON */\n')

# Published, in history order.
run(sys.executable, str(published / "reorder_save_blocks.py"), str(mips_c))
run(sys.executable, str(published / "enable_loop_padding.py"), str(mips_h))
run(sys.executable, str(published / "count_trap_length.py"), str(mips_c))

# U4: earlier transfer-hazard helper (no argument), at both FPR-write sites.
edit(tc_mips,
     "static int insn_uses_fpr_exact PARAMS ((struct mips_cl_insn *ip,\n"
     "\t\t\t\t\t      unsigned int reg));\n",
     "static int insn_uses_fpr_exact PARAMS ((struct mips_cl_insn *ip,\n"
     "\t\t\t\t\t      unsigned int reg));\n"
     "static int rac2_mtc1_nop_ok PARAMS ((void));\n")
for field in ("FT", "FS"):
    edit(tc_mips,
         "\t      if (mips_optimize == 0\n"
         "\t\t  || insn_uses_fpr_exact (ip,\n"
         f"\t\t\t\t\t   ((prev_insn.insn_opcode >> OP_SH_{field})\n"
         f"\t\t\t\t\t    & OP_MASK_{field})))\n"
         "\t\t++nops;\n",
         "\t      if ((mips_optimize == 0\n"
         "\t\t  || insn_uses_fpr_exact (ip,\n"
         f"\t\t\t\t\t   ((prev_insn.insn_opcode >> OP_SH_{field})\n"
         f"\t\t\t\t\t    & OP_MASK_{field})))\n"
         "\t\t\t\t\t   && rac2_mtc1_nop_ok ())\n"
         "\t\t++nops;\n")
edit(tc_mips,
     "      return 1;\n  return 0;\n}\n\nstatic void\nmacro_build (char *place,\n",
     "      return 1;\n  return 0;\n}\n\n"
     "/* RAC2 : earlier transfer-delay rule (reconstructed). */\n"
     "static int\n"
     "rac2_mtc1_nop_ok ()\n"
     "{\n"
     "  if (prev_insn.insn_mo == 0 || strcmp (prev_insn.insn_mo->name, \"mtc1\") != 0)\n"
     "    return 1;\n"
     "  if (prev_prev_insn.insn_mo == 0 || prev_prev_insn.insn_mo == &dummy_opcode)\n"
     "    return 0;\n"
     "  return 1;\n"
     "}\n\n"
     "static void\nmacro_build (char *place,\n")
run(sys.executable, str(published / "restrict_mtc1_exemption.py"), str(tc_mips))

# Published: zero-store (mips.md), then frame-order default off.
run("patch", "-p1", "-d", str(tree), "-i", str(published / "allow_zero_ti_store.patch"))
run(sys.executable, str(published / "disable_frame_order_default.py"), str(mips_c))
print("RAC2 recipe applied")
