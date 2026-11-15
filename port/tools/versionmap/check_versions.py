#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""
Checks every game version's tables, with no disc (CI runs this):

  check_versions.py

- parity.py for every game in its GAMES table;
- release_rac1.py --tables --check for every release folder with a release/release.json,
  carried from the first program of its game;
- hostnames.py check for every program with a host/rac1_version.h.

A new game or version needs no change here: it is found by its files and parity.py's GAMES.
"""
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


def main() -> int:
    ok = True
    game_dir = ROOT / "port/game"
    for game, spec in GAMES.items():
        ok &= run("port/tools/versionmap/parity.py", game)
        primary = next(iter(spec["programs"]))
        for program in spec["programs"]:
            if (game_dir / program / "release/release.json").is_file():
                ok &= run("port/tools/hostgen/release_rac1.py", "--game", f"port/game/{program}",
                          "--primary", f"port/game/{primary}", "--tables", "--check")
    for header in sorted(game_dir.glob("*/host/rac1_version.h")):
        ok &= run("port/tools/versionmap/hostnames.py", "check", f"port/game/{header.parent.parent.name}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
