#!/usr/bin/env python3
"""The level's mobys as the game starts them, worked out by the game's own code.

  moby_states.py PROJECT --data DIR --levels DIR [--level N ...] [--program FILE] [--ticks N]

Runs the port's states program (openrac-rac1-<version>-states, port/game/common/program/moby_states.h)
on each level of the Godot project PROJECT (one project, or a folder of per-level projects): it
loads the level, runs the mobys' updates for a few ticks and writes each placed moby's state. The
result goes to levels/level_NN/moby_states.json beside level.json, where rc1/moby.gd reads it.

Packing a level (extract.py pack --states FILE --data DIR --levels DIR) works its mobys out again
from the packed level (after_pack): the packed table's order differs from the disc's once mobys
are added or deleted, so each entry also names its moby's node.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

# The states program the port's build makes for a version (port/cmake/Games.cmake).
PROGRAM = "port/build/release/openrac-rac1-{version}-states"

# A level takes well under a second; a stuck run is ended.
TIMEOUT = 60


def level_folders(project: Path, wanted: list[int] | None) -> dict[int, Path]:
    """The project's level folders (levels/level_NN), by number."""
    found = {}
    for stats in [*project.glob("levels/level_*/level.json"), *project.glob("level_*/levels/level_*/level.json")]:
        number = json.loads(stats.read_text())["level"]
        if wanted is None or number in wanted:
            found[number] = stats.parent
    return dict(sorted(found.items()))


def states(program: Path, data: Path, levels: Path, level: int, ticks: int, mods: Path | None = None) -> dict:
    """One level's moby states, from a run of the program (with the packed levels MODS, if given)."""
    with tempfile.TemporaryDirectory() as temp:
        out = Path(temp) / "states.json"
        command = [str(program), "--data", str(data), "--levels", str(levels), "--level", str(level),
                   "--moby-states", str(out), "--moby-states-ticks", str(ticks)]
        command += ["--mods", str(mods)] if mods is not None else ["--no-mods"]
        run = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
        if run.returncode != 0 or not out.is_file():
            raise RuntimeError(f"level {level}: the states program failed: {run.stderr.strip()[-400:]}")
        return json.loads(out.read_text())


def after_pack(program: Path, data: Path, levels: Path, packed: Path, project: Path,
               moby_names: dict[int, tuple[Path, list[str]]], ticks: int = 300) -> list[str]:
    """The states of the levels whose mobys were packed (MOBY_NAMES, from pack.pack), worked out
    from the packed levels PACKED and written beside their scenes, each entry with its node's
    name. A level of PROJECT packed as the disc has it loses a file written from an earlier pack,
    so the editor works it out from the disc's level again. What happened, a line each."""
    lines = []
    for stats in [*project.glob("levels/level_*/level.json"), *project.glob("level_*/levels/level_*/level.json")]:
        number, file = json.loads(stats.read_text()).get("level"), stats.parent / "moby_states.json"
        if number in moby_names or not file.is_file():
            continue
        try:
            stale = json.loads(file.read_text()).get("packed", False)
        except (OSError, ValueError, AttributeError):
            stale = True
        if stale:
            file.unlink()
    for number, (folder, names) in sorted(moby_names.items()):
        try:
            result = states(program, data, levels, number, ticks, packed)
        except (OSError, RuntimeError, subprocess.TimeoutExpired, ValueError) as error:
            lines.append(f"level {number:02}: moby start states failed ({error})")
            continue
        for moby in result["mobys"]:
            key = moby["key"]
            moby["name"] = names[key] if 0 <= key < len(names) else None
        result["packed"], result["names"] = True, names
        (folder / "moby_states.json").write_text(json.dumps(result, separators=(",", ":")) + "\n")
        lines.append(f"level {number:02}: moby start states updated")
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("project", type=Path)
    parser.add_argument("--data", type=Path, required=True, help="the port's game data")
    parser.add_argument("--levels", type=Path, required=True, help="the port's levels, or a custom map's packed levels")
    parser.add_argument("--level", type=int, action="append", help="a level (repeatable; default: all in PROJECT)")
    parser.add_argument("--program", type=Path, default=os.environ.get("OPENRAC_STATES"),
                        help="the states program (default: OPENRAC_STATES, else the build's for the levels' version)")
    parser.add_argument("--ticks", type=int, default=300, help="ticks the level runs first (default: 300, 5 seconds)")
    args = parser.parse_args()
    folders = level_folders(args.project, args.level)
    if not folders:
        sys.exit(f"{args.project} has no levels")
    for number, folder in folders.items():
        version = json.loads((folder / "level.json").read_text()).get("version", "pal")
        program = Path(args.program) if args.program else ROOT / PROGRAM.format(version=version)
        if not program.is_file() and program.with_suffix(".exe").is_file():
            program = program.with_suffix(".exe")
        if not program.is_file():
            sys.exit(f"no states program at {program}: build the port's openrac-rac1-{version}-states target")
        result = states(program, args.data, args.levels, number, args.ticks)
        (folder / "moby_states.json").write_text(json.dumps(result, separators=(",", ":")) + "\n")
        print(f"level {number}: {len(result['mobys'])} mobys")


if __name__ == "__main__":
    main()
