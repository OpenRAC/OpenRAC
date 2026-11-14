#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Interface texture runs (docs/port/TEXTURE_PACKS.md, "Export without playing").

  run.py run --game rac1-ntsc --out FILE [--exe P --data D --levels L] [--jobs N] [--official]
      Plays the game's routes hidden (the regress tool's: every level, the pause pages, the vendor,
      the title), each with --interface-run, and joins their parts into FILE: the run file the
      launcher's export reads. Prints one JSON object a line ("event": start, route, done).
  run.py join FILE PART... [--official]
      Joins parts (or runs) into FILE.
  run.py info FILE
      What a run file holds, as one JSON object.

A run file holds no byte of the game: hashes, registers and places on the disc only.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "regress"))

import routes as routes_mod  # noqa: E402
import runner  # noqa: E402

FORMAT = 1
CATEGORIES = ("font", "interface", "effects")
# Each game's route set: the most of its interface the routes show.
# The renderer's passes of the 3D world (OPENRAC_HIDE_PASSES); its picture plays no part in the game.
WORLD_PASSES = "sky,terrain,ties,shrubs,mobys,ratchet shadows"

ROUTE_SETS = {"rac1-pal": "wide", "rac1-ntsc": "wide", "rac2-ntsc": "rac2-tour"}


def say(event: str, **fields) -> None:
    print(json.dumps({"event": event, **fields}), flush=True)


def read_part(path: Path) -> dict | None:
    """A part or run file, or None when it is not one (cut short by a crash, another format)."""
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    if not isinstance(doc, dict) or doc.get("format") != FORMAT or not isinstance(doc.get("textures"), list):
        return None
    return doc


def join(parts: list[dict], game: str | None = None) -> dict:
    """One run of PARTS: every texture once, with the blobs it needs; "missing" keeps the textures no
    part could place on the disc. Parts of another game or converter than the first are left out."""
    parts = [p for p in parts if game is None or p.get("game") == game]
    if not parts:
        raise ValueError("no part of this game to join")
    first = parts[0]
    blobs: dict[str, dict] = {}
    textures: dict[str, dict] = {}
    missing: dict[str, set[str]] = {c: set() for c in CATEGORIES}
    for part in parts:
        if part.get("game") != first.get("game") or part.get("converter") != first.get("converter"):
            continue
        for t in part["textures"]:
            name = t.get("name")
            needs = [u.get("blob") for u in t.get("uploads", [])] if t.get("kind") == "pool" \
                else [t.get("indices"), t.get("palette")]
            if not name or name in textures or any(b not in part.get("blobs", {}) for b in needs):
                continue
            textures[name] = t
            for b in needs:
                blobs.setdefault(b, part["blobs"][b])
        for category, names in (part.get("missing") or {}).items():
            if category in missing and isinstance(names, list):
                missing[category].update(names)
    for names in missing.values():
        names.difference_update(textures)
    counts = {c: sum(1 for t in textures.values() if t.get("category") == c) for c in CATEGORIES}
    ordered = sorted(textures.values(), key=lambda t: (CATEGORIES.index(t["category"])
                                                       if t.get("category") in CATEGORIES else 9, t["name"]))
    return {"format": FORMAT, "game": first.get("game"), "converter": first.get("converter"),
            "counts": counts, "missing": {c: sorted(n) for c, n in missing.items()},
            "blobs": dict(sorted(blobs.items())), "textures": ordered}


def write(doc: dict, out: Path) -> None:
    """OUT, whole or not at all."""
    out.parent.mkdir(parents=True, exist_ok=True)
    partial = out.with_name(out.name + ".partial")
    partial.write_text(json.dumps(doc, indent=1) + "\n", encoding="utf-8")
    os.replace(partial, out)


def info(doc: dict) -> dict:
    counts = doc.get("counts") or {c: sum(1 for t in doc["textures"] if t.get("category") == c) for c in CATEGORIES}
    return {"game": doc.get("game"), "converter": doc.get("converter"), "made": doc.get("made"),
            "official": bool(doc.get("official")), "clean": bool(doc.get("clean")), "counts": counts,
            "missing": {c: len(v) for c, v in (doc.get("missing") or {}).items()},
            "routes": {k: len(v) for k, v in (doc.get("routes") or {}).items()}}


def run(args) -> int:
    game = args.game
    if game not in ROUTE_SETS:
        say("error", message=f"{game} has no routes to run yet (known: {', '.join(ROUTE_SETS)})")
        return 1
    exe = args.exe or runner.REPO / "port" / "build" / "release" / f"openrac-{game}{'.exe' if os.name == 'nt' else ''}"
    data, levels = runner.default_inputs(game)
    # Absolute: each route's game runs in a folder of its own.
    exe = Path(exe).resolve()
    data, levels = Path(args.data or data).resolve(), Path(args.levels or levels).resolve()
    for path, what in ((exe, "the game"), (data, "the game's data"), (levels, "the levels")):
        if not path.exists():
            say("error", message=f"{path} is missing: {what} must be set up first")
            return 1
    spec = routes_mod.load()
    chosen = routes_mod.set_routes(ROUTE_SETS[game], game, spec)
    if args.only:
        chosen = [r for r in chosen if r.key in args.only]
    for r in chosen:
        r.timeout *= args.timeout_scale
    skipped = []
    if not (runner.cards_dir() / game / "card").is_dir():
        # A route that needs the prepared card cannot run without it; the player's own is never used.
        skipped = [r.key for r in chosen if r.card]
        chosen = [r for r in chosen if not r.card]
    if args.work:
        args.work.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="interface-run-", dir=args.work)) if args.work else \
        Path(tempfile.mkdtemp(prefix="openrac-interface-run-"))
    say("start", game=game, routes=len(chosen), skipped=skipped, work=str(work))
    done, failed = [], []
    # A small window: the HUD's textures are the game's whatever the window's size, and drawing on
    # the CPU (no graphics card) costs by the pixel.
    settings = {"render_scale": 25, "window_size": "320x240"}
    # The 3D world is not drawn: its textures are not the interface's, and drawing it was most of a
    # route's time without a graphics card. The HUD and the effects are drawn as in the game.
    env = {"OPENRAC_HIDE_PASSES": WORLD_PASSES, **dict(kv.split("=", 1) for kv in args.env)}

    def one(r: routes_mod.Route) -> tuple[routes_mod.Route, dict]:
        out = work / r.key
        res = runner.run_level(game, r, exe, data, levels, out, env, settings,
                               ["--no-mods", "--interface-run", str(out / "part.json")])
        return r, res

    from concurrent.futures import ThreadPoolExecutor
    parts = []
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for i, (r, res) in enumerate(pool.map(one, chosen), 1):
            part = read_part(work / r.key / "part.json")
            # A route that did not play to its end still gave what it drew, if its part was written.
            ok = res["status"] == "ok" and part is not None
            (done if ok else failed).append(r.key)
            if part is not None:
                parts.append(part)
            say("route", key=r.key, name=r.name, status=res["status"] if part is not None else "no-part",
                done=i, of=len(chosen))
    if not parts:
        say("error", message="no route wrote its part: see " + str(work))
        return 1
    try:
        doc = join(parts, game)
    except ValueError as e:
        say("error", message=str(e))
        return 1
    doc["made"] = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    doc["official"] = bool(args.official)
    doc["clean"] = not failed  # the routes that need the prepared card are left out, not failed
    doc["routes"] = {"done": sorted(done), "failed": sorted(failed), "skipped": sorted(skipped)}
    write(doc, args.out)
    if doc["clean"]:
        # The file as the project keeps it (port/game/<id>/interface-run.json), ready to offer in a
        # pull request: the launcher opens its folder and GitHub's upload page.
        write(doc, args.out.parent / "share" / game / "interface-run.json")
    if not args.keep:
        shutil.rmtree(work, ignore_errors=True)
    say("done", file=str(args.out.resolve()), **info(doc))
    print(f"openrac-output: {args.out.resolve()}", flush=True)
    # Written, clean or not: the launcher shows which routes did not play to their end.
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    r = commands.add_parser("run", help="play the routes and write a run file")
    r.add_argument("--game", required=True, choices=sorted(set(routes_mod.GAMES)))
    r.add_argument("--out", type=Path, required=True, help="the run file to write")
    r.add_argument("--exe", type=Path, help="the game program (default: port/build/release's)")
    r.add_argument("--data", type=Path, help="the game's data folder")
    r.add_argument("--levels", type=Path, help="the extracted levels")
    r.add_argument("--jobs", type=int, default=max(1, min(4, (os.cpu_count() or 2) // 2)))
    r.add_argument("--only", action="append", help="only this route (repeatable; for testing)")
    r.add_argument("--env", action="append", default=[], help="KEY=VALUE for every route (OPENRAC_SOFTWARE_GL=1)")
    r.add_argument("--timeout-scale", type=float, default=1.0,
                   help="give each route this many times its time (a slow computer, software drawing)")
    r.add_argument("--official", action="store_true", help="mark it as the project's own run")
    r.add_argument("--work", type=Path, help="where the routes' folders go (a new folder in it)")
    r.add_argument("--keep", action="store_true", help="keep the routes' folders")
    j = commands.add_parser("join", help="join parts or runs into one run file")
    j.add_argument("out", type=Path)
    j.add_argument("parts", type=Path, nargs="+")
    j.add_argument("--official", action="store_true")
    i = commands.add_parser("info", help="what a run file holds")
    i.add_argument("file", type=Path)
    args = parser.parse_args(argv)
    if args.command == "run":
        return run(args)
    if args.command == "join":
        parts = [p for p in (read_part(f) for f in args.parts) if p is not None]
        if not parts:
            say("error", message="none of the files is a part")
            return 1
        doc = join(parts, parts[0].get("game"))
        doc["made"] = max((p.get("made") or "" for p in parts), default="") or \
            datetime.now(timezone.utc).strftime("%Y-%m-%d")
        doc["official"] = bool(args.official)
        doc["clean"] = all(p.get("clean", True) for p in parts)
        routes = {k: sorted({x for p in parts for x in (p.get("routes") or {}).get(k, [])})
                  for k in ("done", "failed", "skipped")}
        routes["failed"] = sorted(set(routes["failed"]) - set(routes["done"]))
        routes["skipped"] = sorted(set(routes["skipped"]) - set(routes["done"]))
        doc["routes"] = routes
        write(doc, args.out)
        say("done", file=str(args.out.resolve()), **info(doc))
        return 0
    doc = read_part(args.file)
    if doc is None:
        say("error", message=f"{args.file} is not a run file")
        return 1
    print(json.dumps(info(doc)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
