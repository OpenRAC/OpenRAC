#!/usr/bin/env python3
"""Add the opt-in -frac2-keep-block-order: no Cygnus block merging after the jump passes.

cygnus_merge_blocks (flow.c, "CYGNUS LOCAL law"; gcc/ChangeLog.Cygnus, 28 May 1998, "Simple
block merging optimization pass") runs after the first jump pass and after the jump passes
that follow cse, gcse and cse2. It moves a block reached only by a jump, from a block with no
other successor, up behind that block and deletes the jump; a switch whose default is its last
case loses its `b default` and gets the default body behind the dispatch. The GCC 2.95 branch
has no such pass. That retail's compiler lacked it is an inference from retail's code, not a
dated GCC fix: retail keeps `b default` and the default body last (func_L18_004283F0, and
func_L02_003FCCA0 with -frac2-gcse-unsplit-edges), and func_002900B8 keeps case 0 behind its
dispatch. So it is off by default and enabled per compilation unit in config/cflags.txt. With
the option the pass returns at once, before its own find_basic_blocks, as if rest_of_compilation
never called it; without it, the source is unchanged in effect.
tests/test_keep_block_order.py covers the case it fixes and controls.

Usage: keep_block_order.py <gcc/flow.c>   (also edits toplev.c and flags.h beside it)
"""
import sys
from pathlib import Path

PASS_OLD = ("  if (reload_completed)\n"
            "    return;\n"
            "\n"
            "  /* ??? This does not work when EH is enabled.")
PASS_NEW = ("  if (reload_completed)\n"
            "    return;\n"
            "\n"
            "  /* RAC2: -frac2-keep-block-order skips this Cygnus pass, which GCC 2.95\n"
            "     does not have.  */\n"
            "  if (flag_rac2_keep_block_order)\n"
            "    return;\n"
            "\n"
            "  /* ??? This does not work when EH is enabled.")
DEFINITION_OLD = "int flag_thread_jumps;\n"
DEFINITION_NEW = ("int flag_thread_jumps;\n"
                  "\n"
                  "/* RAC2: opt-in, no Cygnus block merging after the jump passes (GCC 2.95).  */\n"
                  "int flag_rac2_keep_block_order = 0;\n")
OPTION_OLD = '  {"thread-jumps", &flag_thread_jumps, 1,\n'
OPTION_NEW = ('  {"rac2-keep-block-order", &flag_rac2_keep_block_order, 1,\n'
              '   "Do not merge blocks after the jump passes, as GCC 2.95"},\n'
              '  {"thread-jumps", &flag_thread_jumps, 1,\n')
DECLARATION_OLD = "extern int flag_expensive_optimizations;\n"
DECLARATION_NEW = ("extern int flag_expensive_optimizations;\n"
                   "\n"
                   "/* RAC2: no Cygnus block merging after the jump passes.  */\n"
                   "extern int flag_rac2_keep_block_order;\n")


def replace_once(source: str, before: str, after: str, label: str) -> str:
    if source.count(before) != 1:
        raise ValueError(f"{label}: expected one insertion point, found {source.count(before)}")
    return source.replace(before, after)


def patch(path: Path) -> None:
    top = path.with_name("toplev.c")
    flags = path.with_name("flags.h")
    flow = replace_once(path.read_text(encoding="utf-8"), PASS_OLD, PASS_NEW, "cygnus_merge_blocks")
    driver = replace_once(top.read_text(encoding="utf-8"), DEFINITION_OLD, DEFINITION_NEW,
                          "option definition")
    driver = replace_once(driver, OPTION_OLD, OPTION_NEW, "option table")
    declarations = replace_once(flags.read_text(encoding="utf-8"), DECLARATION_OLD,
                                DECLARATION_NEW, "option declaration")
    # Validate every insertion point before modifying any source.
    for destination, contents in ((path, flow), (top, driver), (flags, declarations)):
        destination.write_text(contents, encoding="utf-8", newline="")


if __name__ == "__main__":
    patch(Path(sys.argv[1]))
