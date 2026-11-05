#!/usr/bin/env python3
"""
The part of a game built from the player's disc (asmext.py), where there is no Clang to read the
decompilation: the functions still in assembly, translated from the disc's own programs, for a
build of the port that calls them through slots (hostgen.json "asm" "interface").

  python3 asmdev.py --program PROGRAM.pickle --game hostgen.json --retail-image ISO --out DIR

PROGRAM.pickle is what hostgen --program-out wrote in the port's build (the program before the
assembly stage, no retail bytes). DIR gets asm_part_<k>.c (the translations under their own names)
and asm_part_register.c (openrac_asm_register, which fills the slots), to compile against the
build's game_protos.h and the runtime's headers, and link against the game's library. Stops when
the disc gives the program another interface than the build has (another disc, or a port built
from another decompilation): the part would not fit it.

Pure Python: it runs on the Python an Android launcher carries.
"""

from __future__ import annotations

import argparse
import json
import pickle
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[2] / "tools"))

import asmc  # noqa: E402
import asmext  # noqa: E402
from program import inherit_results  # noqa: E402

FILES = 8


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--program", type=Path, required=True, help="hostgen --program-out's pickle")
    ap.add_argument("--game", type=Path, required=True, help="the game's hostgen.json (its \"asm\" and \"interface\")")
    ap.add_argument("--retail-image", type=Path, required=True, metavar="ISO", help="the player's disc image")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args(argv)

    cfg = json.loads(args.game.read_text())
    a = cfg.get("asm") or {}
    if not a.get("interface"):
        print(f"asmdev: {args.game} names no interface: its build translates the assembly itself", file=sys.stderr)
        return 1
    expected = asmext.read(args.game.resolve().parent / a["interface"])
    # The disc in place of the decompilation's dumps, and no interface: the translation itself.
    cfg["asm"] = {k: v for k, v in a.items() if k not in ("interface", "interface_path")}
    cfg["asm"]["retail"] = {"image": str(args.retail_image.resolve())}
    cfg["asm"]["enabled"] = True
    cfg["asm"].pop("shadows", None)  # the collision check's copies: the port's own build has none

    with open(args.program, "rb") as f:
        saved = pickle.load(f)
    if saved.get("format") != asmext.FORMAT:
        print(f"asmdev: {args.program} is format {saved.get('format')}, this reads {asmext.FORMAT}", file=sys.stderr)
        return 1
    program = saved["program"]
    before = asmext.snapshot(program)
    # As hostgen.py translate_from_assembly does, in its order.
    inherit_results(program)
    stage, results = asmc.run(program, cfg, args.program.parent, saved.get("host_text", ""))
    if not results:
        print(f"asmdev: nothing translated from {args.retail_image}: is it this game's disc?", file=sys.stderr)
        return 1
    got = asmext.interface(program, before)
    differ = asmext.differences(expected, got)
    if differ:
        print(f"asmdev: this disc gives {len(differ)} function(s) another signature than the game was built with "
              f"({', '.join(differ[:8])}{', ...' if len(differ) > 8 else ''}): another version of the disc, or "
              "a game built from another decompilation", file=sys.stderr)
        return 2

    args.out.mkdir(parents=True, exist_ok=True)
    names = sorted(results)
    chunks: list[list[str]] = [[] for _ in range(FILES)]
    for k, sym in enumerate(names):
        chunks[k % FILES].append(asmext.renamed(sym, results[sym].text, asmext.defined_name(program, sym)))
    head = asmc.file_head(saved.get("title", ""))
    for k, chunk in enumerate(chunks):
        lead = asmc.PS2_LEAD if any("openrac_ps2" in t for t in chunk) else []
        (args.out / f"asm_part_{k}.c").write_text("\n".join(head[:2] + lead + head[2:] + chunk) + "\n")
    (args.out / "asm_part_register.c").write_text(asmext.register_text(names))
    print(f"asmdev: {len(names)} functions from {args.retail_image.name} in {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
