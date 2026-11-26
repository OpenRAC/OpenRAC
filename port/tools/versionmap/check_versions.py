#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""
Checks every game version's tables, with no disc (CI runs this):

  check_versions.py

- parity.py for every game in its GAMES table;
- release_<game>.py --tables --check for every release folder with a release/release.json,
  carried from the first program of its game, where its game has such a tool (Up Your Arsenal's:
  release_carry_rac3.py --check);
- hostnames.py check for every program with a host/<game>_version.h of rac1's, or of another
  game's release (hostgen.json "release": its tables are written from a disc, so only their
  cover of the shared C is checked);
- no carried hand C of a release calls a func_V1_ name: that is v1.01's function the release
  replaced with C of its own, a stub with no C in the release's program (release.json
  "hand_names" pairs it).

A new game or version needs no change here: it is found by its files and parity.py's GAMES.
"""
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
from parity import GAMES  # noqa: E402


def run(*cmd: str) -> bool:
    print("+", " ".join(cmd), flush=True)
    return subprocess.run([sys.executable, *cmd], cwd=ROOT).returncode == 0


COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
V1_CALL = re.compile(r"\bfunc_V1_\w+(?=\s*\()")


def v1_calls(program: Path) -> list[str]:
    """The func_V1_ names PROGRAM's hand C calls or declares, outside comments."""
    found = []
    for path in sorted((program / "hand").glob("*.c")):
        for name in sorted(set(V1_CALL.findall(COMMENT.sub(" ", path.read_text(errors="replace"))))):
            found.append(f"{path.relative_to(ROOT)}: {name}")
    return found


def main() -> int:
    ok = True
    game_dir = ROOT / "port/game"
    for game, spec in GAMES.items():
        ok &= run("port/tools/versionmap/parity.py", game)
        primary = next(iter(spec["programs"]))
        for program in spec["programs"]:
            # Up Your Arsenal's NTSC-U tables are carried from PAL's by their own tool, which
            # checks them against rac3-pal's tables and the decompilation's.
            carry = ROOT / f"port/tools/hostgen/release_carry_{game}.py"
            if (game_dir / program / "release/release.json").is_file() and carry.is_file():
                ok &= run(f"port/tools/hostgen/{carry.name}", "--check")
                continue
            tool = ROOT / f"port/tools/hostgen/release_{game}.py"
            if (game_dir / program / "release/release.json").is_file() and tool.is_file():
                ok &= run(f"port/tools/hostgen/{tool.name}", "--game", f"port/game/{program}",
                          "--primary", f"port/game/{primary}", "--tables", "--check")
    for header in sorted(game_dir.glob("*/host/*_version.h")):
        program = header.parent.parent
        config = program / "hostgen.json"
        if not config.is_file() or header.name != f"{json.loads(config.read_text())['game']}_version.h":
            continue
        if header.name == "rac1_version.h" or "release" in json.loads(config.read_text()):
            ok &= run("port/tools/versionmap/hostnames.py", "check", f"port/game/{program.name}")
    for release in sorted(game_dir.glob("*/release/release.json")):
        for line in v1_calls(release.parent.parent):
            print(f"{line} has no C in this release: add a release.json \"hand_names\" pair", flush=True)
            ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
