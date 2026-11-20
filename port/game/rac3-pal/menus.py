#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Walks Up Your Arsenal's menus by script and checks every page, on either disc (--version).

Each route starts the port hidden and muted, from the boot screens, with a fresh empty memory card
folder, and presses the pad by script (OPENRAC_PRESS). Every frame the port writes the front end's
state and the strings the font printed (OPENRAC_RAC3_MENU_LOG), and a picture of the window every
few frames (OPENRAC_SHOT). Each step of a route is a press, then a wait; at the end of the wait the
page must:

- draw: at least one string printed, none empty, none "(null)", and the strings the step expects;
- respond: what the page prints, or its picture, changed after the press (for a step that expects
  a change);
- run: the frames still come (no stuck or crashed process).

The picture of the window at the end of each step is kept as OUT/ROUTE/NN-step.png. OUT/report.md
has a table of every step's result, OUT/report.json the same for a program.

    python3 port/game/rac3-pal/menus.py --exe "<the built app's program>" --data <data folder>
        [--version pal|ntsc] [--out build/rac3-menus] [--only ROUTE,...] [--jobs N]

The data folder holds iso_data/rac3/SCES_524.56 (PAL) or SCUS_973.53 (NTSC-U, openrac-rac3-ntsc) and
iso_data/rac3/disc.iso, as for a run by hand (README.md). NTSC-U ticks 60 times a second where PAL
ticks 50, and its pages last the same seconds: every wait is counted in PAL's fiftieths and run as
that many sixtieths on NTSC-U (TICKS). Its pages print other strings in places (NTSC_TEXT). Exit
status 0 when every step passes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

# The pad's buttons as OPENRAC_PRESS masks them (active low bits of the report's second byte pair).
BUTTONS = {
    "select": 0x0001, "l3": 0x0002, "r3": 0x0004, "start": 0x0008,
    "up": 0x0010, "right": 0x0020, "down": 0x0040, "left": 0x0080,
    "l2": 0x0100, "r2": 0x0200, "l1": 0x0400, "r1": 0x0800,
    "triangle": 0x1000, "circle": 0x2000, "cross": 0x4000, "square": 0x8000,
}

# Each version's ticks a second (hostgen.json tick_rate): the routes' waits are PAL's.
TICKS = {"pal": 50, "ntsc": 60}
# What NTSC-U's pages print in place of PAL's strings a step expects (None: the page has no such
# string on NTSC-U, the check is left out).
NTSC_TEXT: dict[str, str | None] = {}

HOLD = 4          # frames a button is held
WINDOW = 8        # the frames before a check whose strings it reads
SHOT_EVERY = 10   # a picture every this many frames (a 640x480 window: they are written in time)


@dataclass
class Step:
    """A press (or none), then `wait` frames; then the checks."""
    name: str
    press: str | None = None
    wait: int = 60
    expect: list[str] = field(default_factory=list)   # strings the page must print (substrings)
    changes: bool = True                              # what is shown must differ from before the press
    allow_empty: bool = False                         # a page that may print nothing (a picture)
    hold: int = HOLD                                  # frames the buttons are held
    known: str = ""                                   # a failure another branch owns
    absent: list[str] = field(default_factory=list)   # strings the page must no longer print


@dataclass
class Route:
    name: str
    steps: list[Step]
    boot_wait: int = 420   # frames to the main menu (it shows at about 330)
    card: bool = True      # an empty memory card folder in slot 1, or no card at all


# Failures another branch owns: shown as known in the report, and not counted in the exit status.
LEVEL_LOAD = ("feat/rac3-programs: the boot program's level load faults (a store at 0x80EFAFxx after "
              "0x003BBDC0 unpacks the level's pack)")

# The main menu's rows top to bottom: LOAD GAME, NEW GAME, MULTIPLAYER, MULTIPLAYER TUTORIAL,
# OPTIONS. Each route starts from the boot screens.
def routes(version: str = "pal") -> list[Route]:
    main = Step("main menu", None, 1, expect=["NEW GAME", "OPTIONS"], changes=False)
    # NTSC-U's Options page has a PROGRESSIVE SCAN row after the 16:9 one (VIDEO here): one row
    # more to SUBTITLES, SOUND and LANGUAGE.
    scan = [Step("to PROGRESSIVE SCAN", "down", 30)] if version == "ntsc" else []
    back_to_main = ["LOAD GAME", "NEW GAME", "OPTIONS"]

    def row(n: int) -> list[Step]:
        return [Step(f"down {i + 1}", "down", 30) for i in range(n)]

    def pad_checks(moves: bool = True) -> list[Step]:
        return [Step("up", "up", 30, changes=moves), Step("down", "down", 30, changes=moves),
                Step("left", "left", 30, changes=False), Step("right", "right", 30, changes=False),
                Step("start", "start", 30, changes=False)]

    return [
        Route("boot", boot_wait=0, steps=[
            Step("legal notice", None, 60, expect=["Sony Computer Entertainment"], changes=False),
            Step("credit screen", None, 160, expect=["A game created and developed by"], changes=False),
            Step("main menu", None, 200, expect=back_to_main, changes=False),
            Step("down", "down", 30), Step("down again", "down", 30), Step("up", "up", 30),
            Step("left", "left", 30, changes=False), Step("right", "right", 30, changes=False),
            Step("triangle", "triangle", 40, changes=False), Step("start", "start", 60, changes=False),
        ]),
        # No memory card: the boot screens' card check, then the menus without one.
        Route("no-card", boot_wait=0, card=False, steps=[
            Step("no card notice", None, 420, expect=["No Memory Card (PS2) inserted"], changes=False),
            Step("continue to the main menu", "cross", 360, expect=back_to_main),
            Step("open LOAD GAME", "cross", 120, expect=["Please insert a Memory Card"]),
            Step("back", "triangle", 90, expect=back_to_main),
            Step("down", "down", 30), Step("open NEW GAME", "cross", 120, expect=["No Memory Card"]),
            Step("triangle", "triangle", 90, expect=back_to_main),
        ]),
        Route("load-game", [main, Step("open LOAD GAME", "cross", 120), *pad_checks(False),
                            Step("cross", "cross", 60, changes=False),
                            Step("back", "triangle", 90, expect=back_to_main)]),
        Route("new-game-no", [main, *row(1), Step("open NEW GAME", "cross", 120, expect=["save file", "Yes", "No"]),
                              *pad_checks(False),
                              Step("no save file", "triangle", 90, expect=["Continue anyway?"]),
                              Step("continue without saving", "cross", 240, allow_empty=True, known=LEVEL_LOAD)]),
        Route("new-game-yes", [main, *row(1), Step("open NEW GAME", "cross", 120, expect=["save file", "Yes", "No"]),
                               Step("create the save file", "circle", 240, expect=["SELECT", "EMPTY"],
                                    known="feat/rac3-saves (#627): func_003932E8's NTSC-U draft reads NTSC-U's "
                                          "buffer top, and the save's icon is read over memory"),
                               Step("start in slot 1", "cross", 300, allow_empty=True, known=LEVEL_LOAD)]),
        # The New Game slot page on a fresh card: the cursor moves over the four slots (the
        # highlighted slot's box), and the dates read the console's clock (sceCdReadClock).
        Route("slots-move", [main, *row(1), Step("open NEW GAME", "cross", 120, expect=["save file"]),
                             Step("create the save file", "circle", 240, expect=["SELECT", "EMPTY", "00.00.00"]),
                             Step("down", "down", 30), Step("down again", "down", 30),
                             Step("up", "up", 30),
                             Step("back", "triangle", 90, expect=back_to_main)]),
        # Multiplayer is the other executable (I5BOOTN.ELF, host/exec.c): its menus (level 39 of its
        # programs) come up with the network notice; EXIT MULTIPLAYER starts the boot program again,
        # whose boot screens lead back to the main menu.
        Route("multiplayer", [main, *row(2),
                              Step("open MULTIPLAYER", "cross", 450, expect=["LOCAL PLAY", "EXIT MULTIPLAYER"],
                                   absent=["LOAD GAME"]),
                              Step("down", "down", 30, expect=["EXIT MULTIPLAYER"]),
                              Step("down again", "down", 30, expect=["EXIT MULTIPLAYER"]),
                              Step("exit multiplayer", "cross", 420, expect=back_to_main)]),
        # The tutorial is a movie (skipped by --skip-movies); the main menu comes back after it.
        Route("mp-tutorial", [main, *row(3), Step("open MULTIPLAYER TUTORIAL", "cross", 240, expect=back_to_main,
                                                  changes=False),
                              *pad_checks()]),
        # Options: four rows (VIDEO in the 16:9 row's place, SUBTITLES, SOUND, LANGUAGE), the
        # cursor on the first; Cross on VIDEO opens the port's Video page (Triangle back to the
        # game's rows), Cross toggles the others, R1 opens ADJUST SCREEN, Triangle goes back.
        Route("options", [main, *row(4), Step("open OPTIONS", "cross", 120, expect=["VIDEO", "LANGUAGE", "STEREO"]),
                          Step("open the Video page", "cross", 40, expect=["DISPLAY MODE", "PICTURE"]),
                          Step("back to the game's rows", "triangle", 40, expect=["VIDEO", "SUBTITLES"]),
                          *scan, Step("to SUBTITLES", "down", 30), Step("subtitles on", "cross", 40, expect=["ON"]),
                          Step("subtitles off", "cross", 40),
                          Step("to SOUND", "down", 30), Step("sound mono", "cross", 40, expect=["MONO"]),
                          Step("sound stereo", "cross", 40, expect=["STEREO"]),
                          Step("up", "up", 30), Step("down", "down", 30),
                          Step("left", "left", 30, changes=False), Step("right", "right", 30, changes=False),
                          Step("start", "start", 30, changes=False),
                          Step("back", "triangle", 90, expect=back_to_main)]),
        # Each Options row's picture (the preview beside the list), the VIDEO row's (the 16:9
        # row's own) and the Video page's, which keeps it.
        Route("options-pictures", [main, *row(4), Step("open OPTIONS on VIDEO", "cross", 120, expect=["VIDEO"]),
                                   *[Step("PROGRESSIVE SCAN's picture", "down", 90)] * bool(scan),
                                   Step("SUBTITLES' picture", "down", 90), Step("SOUND's picture", "down", 90),
                                   Step("LANGUAGE's picture", "down", 90),
                                   *[Step(f"up {i}", "up", 40) for i in range(1, 3 + len(scan))],
                                   Step("VIDEO's picture", "up", 90),
                                   Step("open the Video page", "cross", 90, expect=["DISPLAY MODE"]),
                                   Step("down the Video page", "down", 60, expect=["DISPLAY MODE"]),
                                   Step("back to the game's rows", "triangle", 90, expect=["VIDEO", "SUBTITLES"])]),
        Route("adjust-screen", [main, *row(4), Step("open OPTIONS", "cross", 120, expect=["LANGUAGE"]),
                                Step("open ADJUST SCREEN", "r1", 90, expect=["to adjust screen position"]),
                                # The d-pad moves the console's picture (the OSD offset); the port
                                # places the picture itself (common/lib/graph.c), so nothing moves.
                                Step("up", "up", 40, changes=False), Step("down", "down", 40, changes=False),
                                Step("left", "left", 40, changes=False), Step("right", "right", 40, changes=False),
                                Step("continue", "cross", 60, expect=["LANGUAGE"]),
                                Step("back to the main menu", "triangle", 90, expect=back_to_main)]),
        # LANGUAGE cycles English, French, German, Spanish, Italian and back to English; on NTSC-U
        # English, French and Spanish (its action 0x003B64D8: the byte 0, 2, 4).
        Route("language", [main, *row(4), Step("open OPTIONS", "cross", 120, expect=["LANGUAGE"]),
                           *[Step(f"down {i}", "down", 30) for i in range(1, 4 + len(scan))],
                           Step("French", "cross", 60, expect=["LANGUE", "FRAN"]),
                           *([] if scan else [Step("German", "cross", 60, expect=["SPRACHE", "DEUTSCH"])]),
                           Step("Spanish", "cross", 60, expect=["IDIOMA"]),
                           *([] if scan else [Step("Italian", "cross", 60, expect=["LINGUA", "ITALIANO"])]),
                           Step("English", "cross", 60, expect=["LANGUAGE", "ENGLISH"]),
                           Step("back", "triangle", 90, expect=back_to_main)]),
        Route("sly2-demo", [main, Step("hold L1 L2 R1 R2", "l1+l2+r1+r2", 60, expect=["Sly 2"], hold=60),
                            Step("the demo starts", None, 300, allow_empty=True, changes=False)]),
        Route("idle", [main, Step("idle on the main menu", None, 6300, allow_empty=True, changes=False),
                       Step("press after the attract", "cross", 300, allow_empty=True, changes=False),
                       Step("back to the main menu", "triangle", 200, allow_empty=True, changes=False)]),
    ]


def transitions(path: Path) -> list[str]:
    """The log's frames where what is shown (state words, strings) changes: for exploring a page."""
    out = []
    prev = None
    for frame, rec in sorted(read_log(path).items()):
        st = rec["state"]
        key = (st.get("mode"), st.get("state"), st.get("boot"), st.get("screen"),
               tuple(t for _, t in rec["texts"]))
        if key != prev and rec["texts"] or prev is None or key[:4] != prev[:4]:
            out.append(f"{frame}: mode {key[0]} state {key[1]} boot {key[2]} screen {key[3]} "
                       f"held {st.get('held')} | " + " / ".join(key[4]))
            prev = key
    return out


def read_log(path: Path) -> dict[int, dict]:
    out: dict[int, dict] = {}
    if not path.exists():
        return out
    for line in path.read_text(errors="replace").split("\n"):
        parts = line.split("\t")
        try:
            frame = int(parts[0])
        except (ValueError, IndexError):
            continue
        state = {}
        texts = []
        for p in parts[1:]:
            if "|" in p and "," in p.split("|", 1)[0]:
                place, text = p.split("|", 1)
                texts.append((place, text))
            elif "=" in p:
                k, v = p.split("=", 1)
                state[k] = v
        out[frame] = {"state": state, "texts": texts}
    return out


def for_version(route: Route, version: str) -> Route:
    """ROUTE as VERSION runs it: its waits in that version's ticks, its strings that version's."""
    if version == "pal":
        return route

    def ticks(n: int) -> int:
        return round(n * TICKS[version] / TICKS["pal"])

    def text(t: list[str]) -> list[str]:
        return [NTSC_TEXT.get(e, e) for e in t if NTSC_TEXT.get(e, e) is not None]
    steps = [Step(s.name, s.press, ticks(s.wait), text(s.expect), s.changes, s.allow_empty, s.hold, s.known,
                  text(s.absent)) for s in route.steps]
    return Route(route.name, steps, ticks(route.boot_wait), route.card)


def press_spec(route: Route) -> tuple[str, list[tuple[Step, int, int]]]:
    """OPENRAC_PRESS for the route, and each step with the frame it presses at and the frame it is
    checked at."""
    at = route.boot_wait
    presses = []
    timeline = []
    for s in route.steps:
        start = at
        if s.press:
            mask = 0
            for b in s.press.split("+"):
                mask |= BUTTONS[b]
            presses.append(f"{at}:{mask:x}:{s.hold}")
        at += s.wait
        timeline.append((s, start, at))
    return ",".join(presses), timeline


def nearest_shot(shots: Path, frame: int) -> Path | None:
    best = None
    for f in range(frame, frame - 2 * SHOT_EVERY, -1):
        p = shots / f"shot{f}.png"
        if p.exists():
            return p
    return best


def image_hash(p: Path | None) -> str | None:
    return hashlib.sha1(p.read_bytes()).hexdigest() if p and p.exists() else None


def run_route(route: Route, exe: Path, data: Path, out: Path, timeout: float) -> dict:
    rdir = out / route.name
    if rdir.exists():
        shutil.rmtree(rdir)
    (rdir / "shots").mkdir(parents=True)
    (rdir / "card").mkdir()
    card = ["--cards", str(rdir / "card")] if route.card else ["--no-card"]
    press, timeline = press_spec(route)
    end = timeline[-1][2] + 10
    env = {k: v for k, v in os.environ.items() if not k.startswith("OPENRAC_")}
    env.update({
        "OPENRAC_HIDDEN": "1", "SDL_AUDIO_DRIVER": "dummy", "SDL_AUDIODRIVER": "dummy",
        "OPENRAC_PRESS": press, "OPENRAC_RAC3_MENU_LOG": str(rdir / "menu.log"),
        "OPENRAC_SHOT": f"{SHOT_EVERY}:{rdir / 'shots' / 'shot'}",
    })
    # A small window: its pictures are written as fast as they come, so a crash loses none.
    (rdir / "settings.ini").write_text("window_size = 640x480\n")
    cmd = [str(exe), "--data", str(data), *card, "--window", "--skip-movies",
           "--settings", str(rdir / "settings.ini"), "--frames", str(end)]
    started = time.monotonic()
    with (rdir / "run.log").open("w") as log:
        try:
            proc = subprocess.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
            exit_code = proc.returncode
        except subprocess.TimeoutExpired:
            exit_code = "timeout"
    frames = read_log(rdir / "menu.log")
    last = max(frames) if frames else -1
    results = []
    before_texts = None
    before_img = None
    for i, (s, start, check) in enumerate(timeline):
        rec = frames.get(check) or frames.get(check - 1) or {}
        # The strings of the step's last frames together: some pages blink (a frame in eight
        # without its text).
        texts = list(dict.fromkeys(t for f in range(check - WINDOW + 1, check + 1)
                                   for _, t in frames.get(f, {}).get("texts", [])))
        shot = nearest_shot(rdir / "shots", check)
        kept = None
        if shot:
            kept = rdir / f"{i:02d}-{s.name.replace(' ', '-').replace('/', '-')}.png"
            shutil.copyfile(shot, kept)
        problems = []
        if last < check:
            problems.append(f"no frame {check} (the run ended at {last}, exit {exit_code})")
        else:
            if not texts and not s.allow_empty:
                problems.append("nothing printed")
            # The Options page's VIDEO row has no value: its cell prints empty by design.
            if any(t.strip() == "" for t in texts) and "VIDEO" not in texts:
                problems.append("an empty string printed")
            if any("(null)" in t for t in texts):
                problems.append("(null) printed")
            joined = "\n".join(texts)
            for e in s.expect:
                if e not in joined:
                    problems.append(f"'{e}' not printed")
            for e in s.absent:
                if e in joined:
                    problems.append(f"'{e}' still printed")
            img = image_hash(shot)
            if s.changes and s.press and before_texts is not None and texts == before_texts and img == before_img:
                problems.append(f"no change after {s.press}")
            before_img = img
        before_texts = texts
        results.append({
            "step": s.name, "press": s.press, "frame": check, "state": rec.get("state", {}),
            "texts": texts, "picture": str(kept) if kept else None, "problems": problems, "known": s.known,
        })
    shutil.rmtree(rdir / "shots", ignore_errors=True)
    (rdir / "pages.txt").write_text("\n".join(transitions(rdir / "menu.log")) + "\n")
    return {"route": route.name, "exit": exit_code, "last_frame": last,
            "seconds": round(time.monotonic() - started, 1), "steps": results}


def result(s: dict) -> str:
    if not s["problems"]:
        return "ok"
    known = f" (known, {s['known']})" if s["known"] else ""
    return "FAIL" + known + ": " + "; ".join(s["problems"])


def report(results: list[dict], out: Path) -> bool:
    """report.md and report.json; True when every step passes, or fails only as known."""
    ok = True
    lines = ["| route | step | press | result | state | strings |", "|---|---|---|---|---|---|"]
    for r in results:
        for s in r["steps"]:
            good = not s["problems"]
            ok &= good or bool(s["known"])
            st = s["state"]
            state = f"mode {st.get('mode')} state {st.get('state')} screen {st.get('screen')}" if st else ""
            texts = "; ".join(dict.fromkeys(s["texts"]))[:160].replace("|", "/")
            lines.append(f"| {r['route']} | {s['step']} | {s['press'] or ''} | "
                         f"{result(s)} | {state} | {texts} |")
    (out / "report.md").write_text("\n".join(lines) + "\n")
    (out / "report.json").write_text(json.dumps(results, indent=1))
    return ok


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--exe", type=Path, required=True)
    ap.add_argument("--version", choices=sorted(TICKS), default="pal",
                    help="the disc the program is built for: pal (openrac-rac3-pal) or ntsc (openrac-rac3-ntsc)")
    ap.add_argument("--data", type=Path, required=True)
    ap.add_argument("--out", type=Path, default=Path("build/rac3-menus"))
    ap.add_argument("--only", default="")
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--timeout", type=float, default=300)
    ap.add_argument("--list", action="store_true", help="the routes and their steps")
    a = ap.parse_args()
    todo = [for_version(r, a.version) for r in routes(a.version) if not a.only or r.name in a.only.split(",")]
    if a.list:
        for r in todo:
            print(r.name + ": " + ", ".join(s.name for s in r.steps))
        return 0
    a.out.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(a.jobs) as pool:
        results = list(pool.map(lambda r: run_route(r, a.exe.resolve(), a.data.resolve(), a.out.resolve(),
                                                     a.timeout), todo))
    ok = report(results, a.out)
    print((a.out / "report.md").read_text())
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
