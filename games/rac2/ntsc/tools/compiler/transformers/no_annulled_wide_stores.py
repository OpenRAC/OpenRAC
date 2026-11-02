#!/usr/bin/env python3
"""Add the opt-in -frac2-no-annulled-wide-stores: no 64- or 128-bit store in an annulled slot.

In the game's compiled code (the boot from 0x26EA00 and the 27 levels, each catalogued function
once, hand-written ones apart), the delay slots of branch-likely instructions hold 2,895 stores,
every one sw, swc1, sb or sh, while the slots of the other branches and jumps hold 547 sq and
497 sd; wide loads do go into annulled slots (1,215 ld, 23 lq). This cc1's reorg fills an
annulled slot with a wide store when it takes one from the target thread of a conditional
branch (fill_slots_from_thread) or steals one from the delay list of the branch at that target
(steal_delay_list_from_target): a loop that starts with `*(TI128 *)buf = 0;' ends in
`bnel ; sq $zero,0(...)', its label behind the copied store, where retail keeps the store at the
loop top and fills the branch's slot from the fall-through, or leaves it empty
(func_L02_0042EDD0, func_L02_003E7370).

With the option, fill_slots_from_thread stops searching a thread at a store wider than 32 bits
that cannot go into an ordinary slot, and steal_delay_list_from_target refuses one. Stopping,
not refusing and searching on, is what the measured functions need: searching on takes a later
insn of the thread into the slot instead (func_L23_00422668: the `$a0' move behind a 16-byte
copy). reorg's search stops the same way at an asm insn (stop_search_p). optimize_skip, which
annuls the one insn a branch skips, keeps wide stores: the boot's libc has two such
`bnel ; sd' (func_00116460, matched with this cc1). The rule is inferred from retail's code, so
it is off by default and enabled per compilation unit in config/cflags.txt; without it reorg.c
behaves as before, and -fno-rac2-no-annulled-wide-stores disables it again.
tests/test_no_annulled_wide_stores.py covers the anchors, the two paths and the controls.

Usage: no_annulled_wide_stores.py <gcc/reorg.c>   (also edits toplev.c and flags.h beside it)
"""
import sys
from pathlib import Path

PREDICATE_OLD = "/* INSN branches to an insn whose pattern SEQ is a SEQUENCE.  Given that\n"
PREDICATE_NEW = (
    "/* RAC2: nonzero if INSN stores more than 32 bits to memory (sd, sq).  With\n"
    "   -frac2-no-annulled-wide-stores such a store is not put into an annulled\n"
    "   delay slot from a thread or from the delay list of a target branch: the\n"
    "   game's compiled code has none there.  */\n"
    "\n"
    "static int\n"
    "rac2_wide_store_p (insn)\n"
    "     rtx insn;\n"
    "{\n"
    "  rtx set = single_set (insn);\n"
    "\n"
    "  return (set != 0\n"
    "\t  && GET_CODE (SET_DEST (set)) == MEM\n"
    "\t  && GET_MODE_SIZE (GET_MODE (SET_DEST (set))) > 4);\n"
    "}\n"
    "\n" + PREDICATE_OLD)
STEAL_OLD = ("\t         && eligible_for_annul_false (insn, total_slots_filled,\n"
             "\t\t\t\t\t      trial, flags)))\n")
STEAL_NEW = ("\t         && eligible_for_annul_false (insn, total_slots_filled,\n"
             "\t\t\t\t\t      trial, flags)\n"
             "\t         && ! (flag_rac2_no_annulled_wide_stores\n"
             "\t\t       && rac2_wide_store_p (trial))))\n")
THREAD_OLD = ("\t\tgoto winner;\n"
              "\t    }\n"
              "\t  else if (0\n")
THREAD_NEW = ("\t\tgoto winner;\n"
              "\t    }\n"
              "\t  /* RAC2: -frac2-no-annulled-wide-stores.  A wide store does not go\n"
              "\t     into an annulled slot, and the search of this thread ends there.  */\n"
              "\t  else if (flag_rac2_no_annulled_wide_stores\n"
              "\t\t   && rac2_wide_store_p (trial))\n"
              "\t    break;\n"
              "\t  else if (0\n")
DEFINITION_OLD = "int flag_rac2_hazard_labels = 0;\n"
DEFINITION_NEW = (DEFINITION_OLD
                  + "/* Opt-in: no 64- or 128-bit store in an annulled delay slot. */\n"
                    "int flag_rac2_no_annulled_wide_stores = 0;\n")
OPTION_OLD = '  {"rac2-qcopy", &flag_rac2_qcopy, 1,\n'
OPTION_NEW = ('  {"rac2-no-annulled-wide-stores", &flag_rac2_no_annulled_wide_stores, 1,\n'
              '   "Keep 64- and 128-bit stores out of annulled delay slots"},\n' + OPTION_OLD)
DECLARATION_OLD = "extern int flag_rac2_hazard_labels;\n"
DECLARATION_NEW = DECLARATION_OLD + "extern int flag_rac2_no_annulled_wide_stores;\n"


def replace_once(source: str, before: str, after: str, label: str) -> str:
    if source.count(before) != 1:
        raise ValueError(f"{label}: expected one insertion point, found {source.count(before)}")
    return source.replace(before, after)


def patch(path: Path) -> None:
    top = path.with_name("toplev.c")
    flags = path.with_name("flags.h")
    sources = {p: p.read_text(encoding="utf-8") for p in (path, top, flags)}
    if any("rac2_no_annulled_wide_stores" in text for text in sources.values()):
        raise ValueError("annulled wide-store option already present")
    reorg = replace_once(sources[path], PREDICATE_OLD, PREDICATE_NEW, "wide-store predicate")
    reorg = replace_once(reorg, STEAL_OLD, STEAL_NEW, "steal_delay_list_from_target's annulled case")
    reorg = replace_once(reorg, THREAD_OLD, THREAD_NEW, "fill_slots_from_thread's annulled case")
    driver = replace_once(sources[top], DEFINITION_OLD, DEFINITION_NEW, "option definition")
    driver = replace_once(driver, OPTION_OLD, OPTION_NEW, "option table")
    declarations = replace_once(sources[flags], DECLARATION_OLD, DECLARATION_NEW,
                                "option declaration")
    # Validate every insertion point before modifying any source.
    for destination, contents in ((path, reorg), (top, driver), (flags, declarations)):
        destination.write_text(contents, encoding="utf-8", newline="")


if __name__ == "__main__":
    patch(Path(sys.argv[1]))
