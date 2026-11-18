#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks the parts of a Going Commando release table that need no disc (CI runs it).

A release's table (port/game/rac2-pal/release/release_names.json, written by release.py build) is
built from the decompilation built for the release, so the disc. One part of it rests only on the
decompilation's own files: the release's own C (src/<version>/) calls a level hook by the name
its catalogue gives it, while the table pairs v1.01's hook with another by its callers
(pair_stubs). So each word of that C naming such a hook, by its address name or a readable one
(stub_empty_17), needs a "units" entry with the catalogue's name, or that C calls the hook 8
bytes off (PAL levels 17 and 19 crashed so). This checks those entries against the
decompilation's catalogues and C: none missing, none wrong, none left over.

    python3 port/tools/hostgen/release_rac2.py --game port/game/rac2-pal --primary port/game/rac2-ntsc \\
        --tables --check

--tables and --check are accepted as release_rac1.py takes them (check_versions.py): this only
checks; release.py build writes the table again.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import release as rel  # noqa: E402

ROOT = HERE.parents[2]


def check(game: Path, source: Path | None = None) -> list[str]:
    """What the table at GAME (port/game/<release>) says otherwise than the decompilation."""
    cfg = json.loads((game / "hostgen.json").read_text())
    version = cfg["release"]["version"]
    source = source or ROOT / cfg["source"]
    table = json.loads((game / cfg["release"]["table"]).read_text())
    release = rel.Release(table)
    known = rel.catalogue_hooks(source, version, table.get("hooks", []))
    readable = rel.primary_aliases(source, cfg)
    aliases = rel.aliases_of(readable, set(table.get("hooks", [])))
    words = {h: [h, *aliases.get(h, ())] for h in table.get("hooks", [])}
    exact = {w for h in known for w in words[h]}
    # A hook the catalogue lacks is named by the release's build (the disc): its entry is only
    # checked to name a word the file still has.
    loose = {w for h, ws in words.items() if h not in known for w in ws}
    units = table.get("units", {})
    problems = []
    seen = set()
    for d in cfg.get("sources", ["src"]):
        for path in sorted((source / d).rglob("*.c")):
            name = path.relative_to(source).as_posix()
            if not name.startswith(f"src/{version}/") or name in seen:
                continue
            seen.add(name)
            text = path.read_text(errors="replace")
            want = rel.hook_words(text, known, aliases, release)
            have = {w: v for w, v in units.get(name, {}).items() if w in exact}
            for word in sorted(set(want) | set(have)):
                if word not in have:
                    problems.append(f"{name}: {word} has no unit entry; the catalogue calls it {want[word]}")
                elif word not in want:
                    problems.append(f"{name}: {word} -> {have[word]} is left over (the C or the pairing changed)")
                elif want[word] != have[word]:
                    problems.append(f"{name}: {word} -> {have[word]}, the catalogue calls it {want[word]}")
            for word in sorted(set(units.get(name, {})) & loose):
                if not rel.names_word(text, word, address=word in words):
                    problems.append(f"{name}: {word} is left over (the file no longer names it)")
    for name in sorted(set(units) - seen):
        if name.startswith(f"src/{version}/") and set(units[name]) & (exact | loose):
            problems.append(f"{name}: hook entries left over (no such file)")
    return problems


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game", type=Path, required=True, help="the release's folder (port/game/rac2-pal)")
    ap.add_argument("--primary", type=Path, help="the primary release's folder (unused; check_versions.py passes it)")
    ap.add_argument("--source", type=Path, help="the decompilation (default: hostgen.json's)")
    ap.add_argument("--tables", action="store_true", help="the parts that need no disc (the only ones checked)")
    ap.add_argument("--check", action="store_true", help="fail on a difference (always)")
    args = ap.parse_args(argv)
    game = args.game if args.game.is_dir() else args.game.parent
    cfg = json.loads((game / "hostgen.json").read_text())
    source = args.source or ROOT / cfg["source"]
    if not (source / "config").is_dir():
        print(f"release_rac2: no decompilation at {source}: run tools/sources.py init", file=sys.stderr)
        return 1
    problems = check(game, source)
    for p in problems:
        print(p)
    print(f"release_rac2: {game.name}: {len(problems)} hook name(s) of the release's own C that differ"
          + ("; run release.py build again" if problems else ""), file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
