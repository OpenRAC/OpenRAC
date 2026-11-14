#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Interface texture runs (docs/port/TEXTURE_PACKS.md, "Export without playing").

  run.py run --game rac1-ntsc --out FILE [--kinds font,interface] [--exe P --data D --levels L]
             [--jobs N] [--official] [--fresh]
      Plays the game's routes hidden (the regress tool's: every level, the pause pages, the vendor,
      the title), each with --interface-run, and joins their parts into FILE: the run file the
      launcher's export reads. Only the routes the kinds asked for need (KIND_ROUTES), and of those
      only the ones FILE has not played yet: a run made for the font can be given the HUD later.
      Prints one JSON object a line ("event": start, route, done).
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

import card as card_mod  # noqa: E402
import routes as routes_mod  # noqa: E402
import runner  # noqa: E402

FORMAT = 1
CATEGORIES = ("font", "interface", "effects")
# The renderer's passes of the 3D world (OPENRAC_HIDE_PASSES); its picture plays no part in the game.
WORLD_PASSES = "sky,terrain,ties,shrubs,mobys,ratchet shadows"
# Each game's route set: the most of its interface the routes show.
ROUTE_SETS = {"rac1-pal": "wide", "rac1-ntsc": "wide", "rac2-ntsc": "rac2-tour"}
# For each kind, the routes that draw all of it (found by a run that kept each route's part: every
# texture of the kind is in one of them). A game not listed, or a kind, plays every route.
# Both rac1 versions play the same routes. Each kind's routes are the union of what draws all of
# it on NTSC-U and on PAL (a run of each that kept every route's part), the font with a route to spare.
_RAC1_KINDS = {
    "font": ["00-long", "title-new"],
    "interface": ["00-clank", "00-long", "01-long", "01-pause-2", "01-pause-4", "01-pause-5", "03-long", "03-ship",
                  "04-long", "05-long", "06-clank", "07-long", "07-snagglebeast", "08-long", "10-clank", "11-long",
                  "11-swim", "12-long", "13-long", "14-long", "15-giant-clank", "16-hoverboard", "16-long",
                  "17-turret", "18-giant-clank", "title-load-0", "title-load-1", "title-load-2", "title-load-3",
                  "title-load-4", "title-new", "title-options"],
    "effects": ["01-long", "06-clank", "07-snagglebeast", "08-long", "10-clank", "11-jet-fighter", "11-swim",
                "12-long", "13-long", "13-qwark", "14-long", "15-giant-clank", "15-long", "17-turret", "18-drek",
                "18-long"],
}
KIND_ROUTES: dict[str, dict[str, list[str]]] = {"rac1-pal": _RAC1_KINDS, "rac1-ntsc": _RAC1_KINDS}
# The routes that load a saved game play on a card of the run's own, never the player's: a New Game
# on an empty card (the title, New Game, Yes to the save prompt; the same presses on both
# versions) saves the first game, and card.py prepares the five slots from it.
CARD_BOOT = [{"at": 100, "buttons": "cross"}, {"at": 600, "buttons": "start"}, {"at": 700, "buttons": "cross"},
             {"at": 800, "buttons": "circle"}, {"at": 900, "buttons": "circle"},
             *({"at": at, "buttons": "cross"} for at in range(1100, 1600, 100))]


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
            "routes": {k: len(v) for k, v in (doc.get("routes") or {}).items()},
            "kinds": {k: bool(v.get("complete")) for k, v in kinds_of(doc).items()}}


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
    every = routes_mod.set_routes(ROUTE_SETS[game], game, spec)
    kinds = args.kinds or list(CATEGORIES)
    if kinds == ["fresh"]:
        kinds, args.fresh = list(CATEGORIES), True
    needs = kind_routes(game, [r.key for r in every])
    wanted = set().union(*(needs[k] for k in kinds))
    if args.only:
        wanted = set(args.only) & {r.key for r in every}
    if not wanted:
        say("error", message="no route to play: no route of that name")
        return 1
    # The run file there already, of this game: its routes are not played again, and what they gave
    # is kept (--fresh starts over).
    earlier = None if args.fresh else read_part(args.out)
    if earlier is not None and earlier.get("game") != game:
        earlier = None
    before = routes_of(earlier)
    todo = [r for r in every if r.key in wanted and r.key not in before["done"]]
    if earlier is not None and not todo:
        # All played: one short route still is, to see the port names its textures as the earlier
        # run's did (a newer port's conversion makes the earlier run of no use: it is made again).
        todo = [min((r for r in every if r.key in wanted), key=lambda r: (bool(r.card), r.timeout))]
    for r in every:
        r.timeout *= args.timeout_scale
    # The routes' folders go beside the run file unless asked elsewhere: a run the system ended
    # hard (Windows' taskkill) leaves its folder there, and the next run clears it.
    parent = args.work or args.out.resolve().parent / "work"
    parent.mkdir(parents=True, exist_ok=True)
    if not args.work:
        for stale in parent.glob("interface-run-*"):
            shutil.rmtree(stale, ignore_errors=True)
    work = Path(tempfile.mkdtemp(prefix="interface-run-", dir=parent))
    done, failed, skipped, parts = [], [], [], []
    # A small window: the HUD's textures are the game's whatever the window's size, and drawing on
    # the CPU (no graphics card) costs by the pixel.
    settings = {"render_scale": 25, "window_size": "320x240"}
    # The 3D world is not drawn: its textures are not the interface's, and drawing it was most of a
    # route's time without a graphics card. The HUD and the effects are drawn as in the game.
    env = {"OPENRAC_HIDE_PASSES": WORLD_PASSES, **dict(args.env)}
    cards = args.cards
    card_tried = False

    def one(r: routes_mod.Route) -> tuple[routes_mod.Route, str]:
        out = work / r.key
        try:
            res = runner.run_level(game, r, exe, data, levels, out, env, settings,
                                   ["--no-mods", "--interface-run", str(out / "part.json")], cards)
        except (OSError, SystemExit, subprocess.SubprocessError) as e:
            # One route that cannot start (a folder not writable, the program gone) is one failed
            # route, not the end of the run.
            print(f"{r.key}: {e}", file=sys.stderr, flush=True)
            return r, "error"
        return r, res["status"]

    def play(chosen: list[routes_mod.Route], count: int, total: int) -> int:
        """Plays CHOSEN (the routes after COUNT of TOTAL); returns the count after them."""
        nonlocal cards, card_tried
        if any(r.card for r in chosen) and cards is None and not card_tried:
            card_tried = True
            cards = make_card(game, base_route(game), exe, data, levels, work, env, settings)
        if cards is None:
            # Without a card of its own, those routes are left out (not failed).
            left = [r.key for r in chosen if r.card]
            skipped.extend(left)
            chosen = [r for r in chosen if not r.card]
            count += len(left)
        from concurrent.futures import ThreadPoolExecutor, as_completed
        with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
            futures = [pool.submit(one, r) for r in chosen]
            # In the order they end, so the launcher's count moves as routes finish.
            for future in as_completed(futures):
                r, status = future.result()
                part = read_part(work / r.key / "part.json")
                # A route that did not play to its end still gave what it drew, if its part was written.
                ok = status == "ok" and part is not None
                (done if ok else failed).append(r.key)
                if part is not None:
                    parts.append(part)
                count += 1
                say("route", key=r.key, name=r.name, status=status if part is not None else "no-part",
                    done=count, of=total)
        return count

    keep = args.keep
    try:
        say("start", game=game, kinds=kinds, routes=len(todo), earlier=len(before["done"]), work=str(work))
        count = play(todo, 0, len(todo))
        if earlier is not None and parts and earlier.get("converter") != parts[0].get("converter"):
            # The earlier run is of another conversion: none of its textures is this port's. Its
            # routes are played again; the file is made anew.
            earlier, before = None, routes_of(None)
            rest = [r for r in every if r.key in wanted and r.key not in done + failed + skipped]
            say("earlier", message="the earlier run is out of date: its routes are played again", routes=len(rest))
            play(rest, count, count + len(rest))
        if not parts:
            keep = True  # the routes' logs, to see why
            say("error", message="no route wrote its part: see " + str(work))
            return 1
    finally:
        # Stopped (the launcher's Stop ends the run with SIGTERM) or done: the routes' folders go.
        if not keep:
            shutil.rmtree(work, ignore_errors=True)
    try:
        doc = join(parts + ([earlier] if earlier is not None else []), game)
    except ValueError as e:
        say("error", message=str(e))
        return 1
    today = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    played = set(before["done"]) | set(done)
    routes = {"done": sorted(played),
              "failed": sorted((set(before["failed"]) | set(failed)) - played),
              "skipped": sorted((set(before["skipped"]) | set(skipped)) - played)}
    earlier_kinds = kinds_of(earlier)
    doc["made"] = today
    doc["official"] = bool(args.official)
    # Clean: no route it played failed (the routes that need the prepared card are left out, not
    # failed). Each kind is complete when every route it needs has played.
    doc["clean"] = not routes["failed"]
    doc["routes"] = routes
    doc["kinds"] = {k: {"complete": needs[k] <= played,
                        "made": today if k in kinds else earlier_kinds.get(k, {}).get("made")}
                    for k in CATEGORIES}
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


def kind_routes(game: str, keys: list[str]) -> dict[str, set[str]]:
    """For each kind, the routes of KEYS a run of it plays (KIND_ROUTES; every route when unlisted)."""
    table = KIND_ROUTES.get(game, {})
    return {k: set(table[k]) & set(keys) if k in table else set(keys) for k in CATEGORIES}


def routes_of(doc: dict | None) -> dict[str, list[str]]:
    """A run file's routes by how they ended (none for no file)."""
    routes = (doc or {}).get("routes")
    routes = routes if isinstance(routes, dict) else {}
    return {k: [x for x in routes.get(k) or [] if isinstance(x, str)] for k in ("done", "failed", "skipped")}


def kinds_of(doc: dict | None) -> dict[str, dict]:
    """A run file's kinds (complete, made); a file made before kinds had them: the file's date, complete
    when it was clean and nothing was left out."""
    if doc is None:
        return {}
    kinds = doc.get("kinds")
    if isinstance(kinds, dict):
        return {k: v for k, v in kinds.items() if k in CATEGORIES and isinstance(v, dict)}
    whole = bool(doc.get("clean")) and not routes_of(doc)["skipped"]
    return {k: {"complete": whole, "made": doc.get("made")} for k in CATEGORIES}

def base_route(game: str) -> routes_mod.Route | None:
    """The route the run's card is made with: the set's New Game, on an empty card, with CARD_BOOT."""
    if game not in card_mod.SERIALS:
        return None
    new = next((r for r in routes_mod.set_routes(ROUTE_SETS[game], game, routes_mod.load()) if r.key == "title-new"),
               None)
    if new is None:
        return None
    return routes_mod.Route(level=new.level, name="New Game on an empty card", game=game, boot=CARD_BOOT,
                            skip=None, steps=[], length=300, max_frames=new.max_frames, timeout=new.timeout * 4,
                            key="card", start="title", card="empty")


def make_card(game: str, route: routes_mod.Route | None, exe: Path, data: Path, levels: Path, work: Path,
              env: dict, settings: dict) -> Path | None:
    """The prepared card (a folder holding `card`) the routes that load a game play on, made in WORK;
    None (said) when it cannot be made."""
    if route is None:
        say("card", made=False, message=f"no card can be made for {game} yet")
        return None
    out = work / route.key
    try:
        runner.run_level(game, route, exe, data, levels, out, env, settings, ["--no-mods"])
        card_mod.prepare(game, out / "card", work / "prepared")
    except (OSError, SystemExit, ValueError, subprocess.SubprocessError) as e:
        say("card", made=False, message=f"the memory card could not be made: {e}")
        return None
    say("card", made=True)
    return work / "prepared"


def kinds_arg(text: str) -> list[str]:
    """Kinds by commas; "fresh" (the launcher's Make the run again) is every kind, started over."""
    if text == "fresh":
        return ["fresh"]
    kinds = [k for k in CATEGORIES if k in text.split(",")]
    if not kinds or any(k not in CATEGORIES for k in text.split(",")):
        raise argparse.ArgumentTypeError(f"{text!r} is not kinds of {','.join(CATEGORIES)}")
    return kinds


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
    r.add_argument("--kinds", type=kinds_arg, help="the kinds to make, by commas (font,interface,effects; default all)")
    r.add_argument("--fresh", action="store_true", help="start over: the run file there is not added to")
    r.add_argument("--only", action="append", help="only this route (repeatable; for testing)")
    r.add_argument("--env", action="append", default=[], type=env_pair,
                   help="KEY=VALUE for every route (OPENRAC_SOFTWARE_GL=1)")
    r.add_argument("--timeout-scale", type=float, default=1.0,
                   help="give each route this many times its time (a slow computer, software drawing)")
    r.add_argument("--official", action="store_true", help="mark it as the project's own run")
    r.add_argument("--cards", type=Path, help="a prepared card's folder (regress.py card) instead of the run's own")
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
        game = doc.get("game")
        if game in ROUTE_SETS:
            keys = [r.key for r in routes_mod.set_routes(ROUTE_SETS[game], game, routes_mod.load())]
            needs = kind_routes(game, keys)
            dates = {k: max((kinds_of(p).get(k, {}).get("made") or "" for p in parts), default="") or None
                     for k in CATEGORIES}
            doc["kinds"] = {k: {"complete": needs[k] <= set(routes["done"]), "made": dates[k] or doc["made"]}
                            for k in CATEGORIES}
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
