#!/usr/bin/env python3
"""Add the opt-in -frac2-gcse-unsplit-edges: gcse's block LCM on the unsplit flow graph.

With -fno-edge-lcm (the default since patch 0016), gcse_main builds its predecessor and
successor lists with compute_preds_succs (..., 1). That last argument is a Cygnus-local
addition (flow.c, "CYGNUS LOCAL edge splitting/law"; gcc/ChangeLog.Cygnus, 21 January to
14 September 1998): it splits every critical edge that needs no new jump, a fall-through edge
or an edge into a label reached only by jumps, and pre_lcm then inserts on the new block.
The GCC 2.95 branch never had it: its gcse_main calls compute_preds_succs (s_preds, s_succs,
num_preds, num_succs), which only builds the lists, and its PRE inserts at the end of a
block. That retail's compiler lacked the split is an inference from retail's code, not a
dated GCC fix: func_L02_003E79F8 and func_002CD720 match only with this option,
func_L02_003FCCA0 with it and -frac2-keep-block-order, and func_L23_0041B9F0 then has
retail's size. So it is off by default and enabled per compilation unit in config/cflags.txt. With
the option, the call passes 0, exactly GCC 2.95's call; without it, the source is unchanged
in effect. tests/test_gcse_unsplit_edges.py covers the case it fixes and controls.

Usage: gcse_unsplit_edges.py <gcc/gcse.c>   (also edits toplev.c and flags.h beside it)
"""
import sys
from pathlib import Path

CALL_OLD = "      if (compute_preds_succs (s_preds, s_succs, num_preds, num_succs, 1) != 0)\n"
CALL_NEW = ("      /* RAC2: -frac2-gcse-unsplit-edges builds the lists without splitting\n"
            "\t any edge, as GCC 2.95's gcse_main does.  */\n"
            "      if (compute_preds_succs (s_preds, s_succs, num_preds, num_succs,\n"
            "\t\t\t       ! flag_rac2_gcse_unsplit_edges) != 0)\n")
DEFINITION_OLD = "int flag_edge_lcm = 0;\n"
DEFINITION_NEW = ("int flag_edge_lcm = 0;\n"
                  "\n"
                  "/* RAC2: opt-in, gcse without the Cygnus critical-edge split (GCC 2.95).  */\n"
                  "int flag_rac2_gcse_unsplit_edges = 0;\n")
OPTION_OLD = '  {"edge-lcm", &flag_edge_lcm, 1,\n'
OPTION_NEW = ('  {"rac2-gcse-unsplit-edges", &flag_rac2_gcse_unsplit_edges, 1,\n'
              '   "Run gcse without splitting critical edges, as GCC 2.95"},\n'
              '  {"edge-lcm", &flag_edge_lcm, 1,\n')
DECLARATION_OLD = "extern int flag_edge_lcm;\n"
DECLARATION_NEW = ("extern int flag_edge_lcm;\n"
                   "\n"
                   "/* RAC2: gcse without the Cygnus critical-edge split.  */\n"
                   "extern int flag_rac2_gcse_unsplit_edges;\n")


def replace_once(source: str, before: str, after: str, label: str) -> str:
    if source.count(before) != 1:
        raise ValueError(f"{label}: expected one insertion point, found {source.count(before)}")
    return source.replace(before, after)


def patch(path: Path) -> None:
    top = path.with_name("toplev.c")
    flags = path.with_name("flags.h")
    gcse = replace_once(path.read_text(encoding="utf-8"), CALL_OLD, CALL_NEW, "gcse_main's split")
    driver = replace_once(top.read_text(encoding="utf-8"), DEFINITION_OLD, DEFINITION_NEW,
                          "option definition")
    driver = replace_once(driver, OPTION_OLD, OPTION_NEW, "option table")
    declarations = replace_once(flags.read_text(encoding="utf-8"), DECLARATION_OLD,
                                DECLARATION_NEW, "option declaration")
    # Validate every insertion point before modifying any source.
    for destination, contents in ((path, gcse), (top, driver), (flags, declarations)):
        destination.write_text(contents, encoding="utf-8", newline="")


if __name__ == "__main__":
    patch(Path(sys.argv[1]))
