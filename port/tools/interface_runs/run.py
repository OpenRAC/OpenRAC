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
# The renderer's passes of the 3D world (OPENRAC_HIDE_PASSES); its picture plays no part in the game.
WORLD_PASSES = "sky,terrain,ties,shrubs,mobys,ratchet shadows"
# Each game's route set: the most of its interface the routes show.
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
            if not isinstance(t, dict) or t.get("category") not in CATEGORIES \
                    or t.get("kind") not in ("pool", "indexed8"):
                continue
            name = t.get("name")
            needs = [u.get("blob") for u in t.get("uploads") or [] if isinstance(u, dict)] \
                if t.get("kind") == "pool" else [t.get("indices"), t.get("palette")]
            if not name or not needs or name in textures or any(b not in part.get("blobs", {}) for b in needs):
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
    ordered = sorted(textures.values(), key=lambda t: (CATEGORIES.index(t["category"]), t["name"]))
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
    # The checkout's own data unless named (the launcher names both: no git is asked then).
    data, levels = (args.data, args.levels) if args.data and args.levels else runner.default_inputs(game)
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
    if not chosen:
        say("error", message="no route to play: " + ("no route of that name" if args.only
                                                     else "every route needs the prepared memory card"))
        return 1
    # The routes' folders go beside the run file unless asked elsewhere: a run the system ended
    # hard (Windows' taskkill) leaves its folder there, and the next run clears it.
    parent = args.work or args.out.resolve().parent / "work"
    parent.mkdir(parents=True, exist_ok=True)
    if not args.work:
        for stale in parent.glob("interface-run-*"):
            shutil.rmtree(stale, ignore_errors=True)
    work = Path(tempfile.mkdtemp(prefix="interface-run-", dir=parent))
    say("start", game=game, routes=len(chosen), skipped=skipped, work=str(work))
    done, failed = [], []
    # A small window: the HUD's textures are the game's whatever the window's size, and drawing on
    # the CPU (no graphics card) costs by the pixel.
    settings = {"render_scale": 25, "window_size": "320x240"}
    # The 3D world is not drawn: its textures are not the interface's, and drawing it was most of a
    # route's time without a graphics card. The HUD and the effects are drawn as in the game.
    env = {"OPENRAC_HIDE_PASSES": WORLD_PASSES, **dict(args.env)}

    def one(r: routes_mod.Route) -> tuple[routes_mod.Route, str]:
        out = work / r.key
        try:
            res = runner.run_level(game, r, exe, data, levels, out, env, settings,
                                   ["--no-mods", "--interface-run", str(out / "part.json")])
        except (OSError, SystemExit, subprocess.SubprocessError) as e:
            # One route that cannot start (a folder not writable, the program gone) is one failed
            # route, not the end of the run.
            print(f"{r.key}: {e}", file=sys.stderr, flush=True)
            return r, "error"
        return r, res["status"]

    from concurrent.futures import ThreadPoolExecutor, as_completed
    parts = []
    keep = args.keep
    try:
        with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
            futures = [pool.submit(one, r) for r in chosen]
            # In the order they end, so the launcher's count moves as routes finish.
            for i, future in enumerate(as_completed(futures), 1):
                r, status = future.result()
                part = read_part(work / r.key / "part.json")
                # A route that did not play to its end still gave what it drew, if its part was written.
                ok = status == "ok" and part is not None
                (done if ok else failed).append(r.key)
                if part is not None:
                    parts.append(part)
                say("route", key=r.key, name=r.name, status=status if part is not None else "no-part",
                    done=i, of=len(chosen))
        if not parts:
            keep = True  # the routes' logs, to see why
            say("error", message="no route wrote its part: see " + str(work))
            return 1
    finally:
        # Stopped (the launcher's Stop ends the run with SIGTERM) or done: the routes' folders go.
        if not keep:
            shutil.rmtree(work, ignore_errors=True)
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
    # The file as the project keeps it (port/game/<id>/interface-run.json), ready to offer in a
    # pull request: the launcher opens its folder and GitHub's upload page. Only a clean run's: an
    # earlier clean one is not offered for a later run that is not.
    shared = args.out.parent / "share" / game / "interface-run.json"
    if doc["clean"]:
        write(doc, shared)
    else:
        shared.unlink(missing_ok=True)
    say("done", file=str(args.out.resolve()), **info(doc))
    print(f"openrac-output: {args.out.resolve()}", flush=True)
    # Written, clean or not: the launcher shows which routes did not play to their end.
    return 0


def env_pair(text: str) -> tuple[str, str]:
    key, sep, value = text.partition("=")
    if not sep or not key:
        raise argparse.ArgumentTypeError(f"{text!r} is not KEY=VALUE")
    return key, value


def stop_on_sigterm() -> None:
    """SIGTERM (the launcher's Stop) ends the run as Ctrl+C does, so its folders are removed."""
    import signal
    def stop(*_):
        raise KeyboardInterrupt
    try:
        signal.signal(signal.SIGTERM, stop)
    except (ValueError, OSError):
        pass  # not the main thread (a test)


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
    r.add_argument("--env", action="append", default=[], type=env_pair,
                   help="KEY=VALUE for every route (OPENRAC_SOFTWARE_GL=1)")
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
        stop_on_sigterm()
        try:
            return run(args)
        except KeyboardInterrupt:
            say("error", message="stopped")
            return 130
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
