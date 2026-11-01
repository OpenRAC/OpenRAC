#!/usr/bin/env python3
"""Add the opt-in -frac2-hazard-labels: a label keeps its place in front of a hazard nop.

When the instruction after a code label needs a coprocessor hazard nop (an FPU compare
before its bc1f/bc1t, an mtc1 before a reader of the written register), gas emits the nop
in front of that instruction and moves every label defined since the previous instruction
behind it, in append_insn and in mips_emit_delays (the `.set noreorder' that opens a branch).
The retail code never does that. In the boot and the 27 level programs (each function once),
198 branch targets follow a compare or an mtc1 whose consumer needs the nop, and all 198 sit
on the nop: 71 after a compare, 127 after an mtc1. No branch target sits behind such a nop.
SN ProDG 2.0's Ps2EeAs (toolchain/) keeps every label on the nop it inserts, whether
forward, backward, `j' or `.word' references reach it, as the division floor's labels denote
its padding (pad_div_erratum_nops.py). The loop heads found right after one nop following an
mtc1 (56, all 8-byte aligned, none followed by a reader of the register) are the `.p2align 3'
fill before a loop label, not a hazard nop: gas defines those labels after the fill and moves
nothing there.

The option is inferred from retail placements, so it is opt-in per unit: cc1
-frac2-hazard-labels writes `.set rac2_hazard_labels' at the top of its output, and gas then
leaves the labels where they were defined. Without the option neither program's output
changes. The witnesses are func_L18_004308F0 and func_L02_00411530, each in a unit of its own
(src/levels/18_damosel/L18_004308F0.c, src/levels/shared/L02_00411530.c). Regression tests:
tests/test_hazard_labels.py.

Usage: hazard_labels.py <source tree containing gcc/ and gas/>
"""
import sys
from pathlib import Path

STATE_OLD = ("static struct insn_label_list *insn_labels;\n"
             "static struct insn_label_list *free_insn_labels;\n")
STATE_NEW = STATE_OLD + (
    "\n"
    "/* RAC2: set by `.set rac2_hazard_labels' (cc1 -frac2-hazard-labels).  A label then keeps\n"
    "   its place in front of the hazard nops inserted after it: it denotes the padding, as\n"
    "   the original assembler places it, instead of moving to the instruction behind.  */\n"
    "static int rac2_hazard_labels;\n")

APPEND_OLD = "for (l = rac2_div_keep_labels ? NULL : insn_labels; l != NULL; l = l->next)"
APPEND_NEW = ("for (l = (rac2_div_keep_labels || rac2_hazard_labels) ? NULL : insn_labels;\n"
              "\t       l != NULL; l = l->next)")

DELAYS_OLD = ("\t      frag_wane (frag_now);\n"
              "\t      frag_new (0);\n"
              "\t    }\n"
              "\n"
              "\t  for (l = insn_labels; l != NULL; l = l->next)\n")
DELAYS_NEW = ("\t      frag_wane (frag_now);\n"
              "\t      frag_new (0);\n"
              "\t    }\n"
              "\n"
              "\t  for (l = rac2_hazard_labels ? NULL : insn_labels; l != NULL; l = l->next)\n")

SET_OLD = '  else if (strcmp (name, "at") == 0)\n'
SET_NEW = ('  else if (strcmp (name, "rac2_hazard_labels") == 0)\n'
           "    {\n"
           "      rac2_hazard_labels = 1;\n"
           "    }\n" + SET_OLD)

START_OLD = "  ASM_OUTPUT_SOURCE_FILENAME (stream, main_input_filename);\n"
START_NEW = START_OLD + (
    "\n"
    "  /* RAC2: -frac2-hazard-labels has the assembler keep a label in front of the hazard\n"
    "     nops it inserts after the label, where the original assembler places it.  */\n"
    "  if (flag_rac2_hazard_labels)\n"
    "    fprintf (stream, \"\\t.set\\trac2_hazard_labels\\n\");\n")


def replace_once(text: str, before: str, after: str, label: str) -> str:
    if text.count(before) != 1:
        raise ValueError(f"{label}: expected one insertion point, found {text.count(before)}")
    return text.replace(before, after)


def patch(root: Path) -> None:
    tc_mips = root / "gas/config/tc-mips.c"
    flags, top, mips = root / "gcc/flags.h", root / "gcc/toplev.c", root / "gcc/config/mips/mips.c"
    sources = {path: path.read_text(encoding="utf-8") for path in (tc_mips, flags, top, mips)}
    if any("rac2_hazard_labels" in text for text in sources.values()):
        raise ValueError("hazard-label option already present")
    assembler = replace_once(sources[tc_mips], STATE_OLD, STATE_NEW, "insn_labels state")
    assembler = replace_once(assembler, APPEND_OLD, APPEND_NEW, "append_insn label loop")
    assembler = replace_once(assembler, DELAYS_OLD, DELAYS_NEW, "mips_emit_delays label loop")
    assembler = replace_once(assembler, SET_OLD, SET_NEW, ".set directive")
    declarations = replace_once(sources[flags], "extern int flag_rac2_qcopy;",
                                "extern int flag_rac2_qcopy;\n"
                                "extern int flag_rac2_hazard_labels;", "option declaration")
    driver = replace_once(sources[top], "int flag_rac2_qcopy = 0;",
                          "int flag_rac2_qcopy = 0;\n"
                          "/* Opt-in: keep a label in front of the assembler's hazard nops. */\n"
                          "int flag_rac2_hazard_labels = 0;", "option definition")
    anchor = '  {"rac2-qcopy", &flag_rac2_qcopy, 1,'
    driver = replace_once(driver, anchor,
                          '  {"rac2-hazard-labels", &flag_rac2_hazard_labels, 1,\n'
                          '   "Keep a label in front of the assembler\'s hazard nops"},\n' + anchor,
                          "option table")
    backend = replace_once(sources[mips], START_OLD, START_NEW, "assembly file start")
    # Check every anchor before writing any source: drift must leave the tree intact.
    for path, contents in ((tc_mips, assembler), (flags, declarations), (top, driver), (mips, backend)):
        path.write_text(contents, encoding="utf-8", newline="")


if __name__ == "__main__":
    patch(Path(sys.argv[1]))
