#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Where on a level a renderer pass shows (the liquids, the sky, the effects...), in one run of a few
minutes: places for proof pictures, found without a recording.

    survey.py --game rac1-ntsc --level 8 --pass liquids [--points 32] [--out DIR]

The level's collision (the extractor's collision.glb) gives the floors a player stands on; the
survey takes up to --points of them spread over the level, every height of a place included, and
holds the hero at each for 80 frames, facing the level's middle. The run hides the pass in every
other 10 frames (OPENRAC_HIDE_PASSES_EVERY), and a picture is shot every 10 frames, so each place is
shot with the pass, without it and with it again once the camera has come to the hero: the pixels
alike in the two pictures with the pass and unlike the one without it are the pass (what moves by
itself, rain or an enemy, differs between the two with it and is left out). The game's logic runs as ever (only the picture
changes), at a quarter of the resolution, without a graphics card.

DIR/report.json lists the places, the most of the picture the pass covers first, each with the
hero's position and yaw (a route's teleport, or pine.py's writes to hero + 0x80 and + 0x98 in
PCSX2), and DIR/best.png shows the best of them with the pass's pixels marked.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import struct
import subprocess
import sys
from pathlib import Path

import runner

HERE = Path(__file__).resolve().parent
HOLD = 80   # frames the hero is held at a place (the camera flies there in about 40)
EVERY = 10  # the pass is hidden in every other EVERY frames, a picture every EVERY frames


def floors(glb: Path) -> list[tuple[float, float, float]]:
    """The centres of the collision's triangles that face up (a floor), from collision.glb."""
    data = glb.read_bytes()
    size = struct.unpack_from("<I", data, 12)[0]
    doc = json.loads(data[20:20 + size])
    at = 20 + size
    blob = data[at + 8:at + 8 + struct.unpack_from("<I", data, at)[0]]

    def read(i: int) -> list:
        a = doc["accessors"][i]
        view = doc["bufferViews"][a["bufferView"]]
        width = {"SCALAR": 1, "VEC3": 3}[a["type"]]
        code = {5126: "f", 5125: "I", 5123: "H"}[a["componentType"]]
        start = view.get("byteOffset", 0) + a.get("byteOffset", 0)
        flat = struct.unpack_from(f"<{a['count'] * width}{code}", blob, start)
        return [flat[k:k + width] for k in range(0, len(flat), width)] if width > 1 else list(flat)

    out = []
    for mesh in doc["meshes"]:
        for prim in mesh["primitives"]:
            pos = read(prim["attributes"]["POSITION"])
            idx = read(prim["indices"]) if "indices" in prim else list(range(len(pos)))
            for k in range(0, len(idx) - 2, 3):
                a, b, c = (pos[idx[k + j]] for j in range(3))
                u = [b[i] - a[i] for i in range(3)]
                v = [c[i] - a[i] for i in range(3)]
                n = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
                length = math.sqrt(sum(x * x for x in n))
                if length > 0 and n[2] / length > 0.9:
                    out.append(tuple((a[i] + b[i] + c[i]) / 3 for i in range(3)))
    return out


def places(centres: list, count: int, cell: float = 40.0) -> list[list[float]]:
    """Up to count floor places spread over the level: in each cell of the map, one at each height
    that has floor (a gap of 4 units apart), then as far from the others as can be."""
    cells: dict = {}
    for p in centres:
        cells.setdefault((int(p[0] // cell), int(p[1] // cell)), []).append(p)
    candidates = []
    for ps in cells.values():
        ps.sort(key=lambda p: p[2])
        groups, group = [], [ps[0]]
        for p in ps[1:]:
            if p[2] - group[-1][2] > 4.0:
                groups.append(group)
                group = []
            group.append(p)
        groups.append(group)
        for g in groups:
            if len(g) < 4:
                continue
            mid = [sum(p[i] for p in g) / len(g) for i in range(3)]
            best = min(g, key=lambda p: (p[0] - mid[0]) ** 2 + (p[1] - mid[1]) ** 2 + (p[2] - mid[2]) ** 2)
            candidates.append([round(best[0], 2), round(best[1], 2), round(best[2] + 1.0, 2)])
    if not candidates:
        return []
    chosen = [candidates.pop(0)]
    while candidates and len(chosen) < count:
        far = max(candidates, key=lambda c: min((c[0] - o[0]) ** 2 + (c[1] - o[1]) ** 2 + 4 * (c[2] - o[2]) ** 2
                                                for o in chosen))
        candidates.remove(far)
        chosen.append(far)
    return chosen


def routes_file(level: int, points: list, out: Path, jobs: int) -> Path:
    """routes.json with a set "survey": the places in jobs routes, run side by side. The hero faces
    the level's middle, so the picture looks across the level."""
    spec = json.loads((HERE / "routes.json").read_text())
    mid = [sum(p[i] for p in points) / len(points) for i in range(2)]
    routes = {}
    for j in range(jobs):
        steps = [{"at": 5, "health": 8, "hold_health": 100000}]
        at = 10
        for p in points[j::jobs]:
            yaw = math.atan2(mid[1] - p[1], mid[0] - p[0])
            steps.append({"at": at, "teleport": p + [round(yaw, 3)], "hold_place": HOLD})
            at += HOLD
        routes[f"{level:02d}-survey-{j}"] = {"level": level, "name": f"survey {j}", "length": at + 10, "steps": steps}
    wide = spec["sets"]["wide"]["defaults"]
    spec["sets"]["survey"] = {
        "comment": "survey.py: the hero held at floor places, the pass hidden in every other frames",
        "defaults": {**wide, "timeout": 900},
        "routes": routes,
    }
    path = out / "routes.json"
    path.write_text(json.dumps(spec))
    return path


def locate(traces: list[Path], prefixes: list[str], out: Path, threshold: int = 12, best: int = 8) -> list[dict]:
    """Each place's last three pictures, with the pass (frame % 2 EVERY < EVERY), without it and
    with it again, the hero standing at one place in all: the share of the picture the pass covers."""
    pairs = {}
    for trace, prefix in zip(traces, prefixes):
        pairs.update(_pairs(trace, prefix))
    return _report(pairs, out, threshold, best)


def _pairs(trace: Path, prefix: str) -> dict:
    """Each place's last three pictures 2 EVERY frames apart: with the pass, without it, with it."""
    ticks = {}
    for line in trace.read_text().splitlines():
        t = json.loads(line)
        ticks[t["f"]] = t
    pairs = {}
    for f in sorted(ticks):
        if f % (2 * EVERY) != 0 or f + EVERY not in ticks or f + 2 * EVERY not in ticks:
            continue
        a, b, c = ticks[f], ticks[f + EVERY], ticks[f + 2 * EVERY]
        if a.get("hp") is None or any(t.get("hp") != a.get("hp") or t.get("hr") != a.get("hr") for t in (b, c)):
            continue
        settled = ticks.get(f - 5, {})
        cams = [t.get("cam") for t in (settled, a, b, c)]
        if settled.get("hp") != a.get("hp") or None in cams or any(
                max(abs(k[i] - cams[1][i]) for i in range(3)) > 0.5 for k in cams):
            continue  # the camera still flies to the place
        shots = [Path(f"{prefix}{f + i * EVERY}.png") for i in range(3)]
        if all(s.exists() for s in shots):
            pairs[(tuple(a["hp"]), tuple(a["hr"]))] = (f, *shots, a)
    return pairs


def _mask(on: Path, off: Path, again: Path, threshold: int):
    """The pixels the pass covers: alike in both pictures with it, unlike the one without it. What
    moves by itself (rain, an enemy, a flame) differs between the two pictures with the pass, and
    is left out."""
    from PIL import Image, ImageChops

    x, y, z = (Image.open(p).convert("RGB") for p in (on, off, again))
    cut = lambda d, keep: d.convert("L").point(lambda v: 255 if (v > threshold) == keep else 0)
    return ImageChops.multiply(ImageChops.multiply(cut(ImageChops.difference(x, y), True),
                                                   cut(ImageChops.difference(z, y), True)),
                               cut(ImageChops.difference(x, z), False))


def _report(pairs: dict, out: Path, threshold: int, best: int) -> list[dict]:
    from PIL import Image, ImageDraw

    rows = []
    for (hp, hr), (f, on, off, again, t) in pairs.items():
        d = _mask(on, off, again, threshold)
        share = d.histogram()[255] / (d.width * d.height)
        rows.append({"frame": f, "share": round(share, 4), "hero": list(hp), "yaw": hr[2], "camera": t.get("cam"),
                     "forward": t.get("fw"), "on": str(on), "off": str(off), "again": str(again)})
    rows.sort(key=lambda r: -r["share"])
    out.mkdir(parents=True, exist_ok=True)
    (out / "report.json").write_text(json.dumps(rows, indent=1))
    tiles = []
    for r in [r for r in rows if r["share"] > 0][:best]:
        x = Image.open(r["on"]).convert("RGB")
        m = _mask(Path(r["on"]), Path(r["off"]), Path(r["again"]), threshold).point(lambda v: 160 if v else 0)
        x = Image.composite(Image.new("RGB", x.size, (255, 0, 255)), x, m).resize((480, 360))
        ImageDraw.Draw(x).text((8, 8), f"f{r['frame']} {r['share']:.1%} hero {r['hero']} yaw {r['yaw']:.2f}",
                               fill=(255, 255, 0))
        tiles.append(x)
    if tiles:
        sheet = Image.new("RGB", (480 * 4, 360 * ((len(tiles) + 3) // 4)))
        for i, t in enumerate(tiles):
            sheet.paste(t, (480 * (i % 4), 360 * (i // 4)))
        sheet.save(out / "best.png")
    return rows


def main(argv: list[str] | None = None) -> int:
    a = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    a.add_argument("--game", default="rac1-ntsc", choices=("rac1-pal", "rac1-ntsc"))
    a.add_argument("--level", type=int, required=True)
    a.add_argument("--pass", dest="pass_", required=True, help="sky, terrain, ties, liquids, shrubs, mobys, effects, hud")
    a.add_argument("--points", type=int, default=32, help="places: 32 in two runs side by side take about 5 minutes")
    a.add_argument("--jobs", type=int, default=2, help="runs side by side (each takes a share of the places)")
    a.add_argument("--out", help="default: build/regress/survey/<level>-<pass>")
    a.add_argument("--build", default=None)
    o = a.parse_args(argv)
    levels_dir = runner.default_inputs(o.game)[1]
    out = Path(o.out) if o.out else runner.runs_dir().parent / "survey" / f"{o.level:02d}-{o.pass_}"
    out.mkdir(parents=True, exist_ok=True)
    pts = places(floors(levels_dir / f"level_{o.level:02d}" / "collision.glb"), o.points)
    if not pts:
        raise SystemExit("no floors in the level's collision.glb")
    jobs = max(1, min(o.jobs, len(pts)))
    routes = routes_file(o.level, pts, out, jobs)
    run_dir = out / "run"
    shots = out / "shots"
    procs = []
    for j in range(jobs):
        folder = shots / str(j)
        folder.mkdir(parents=True, exist_ok=True)
        for old in folder.glob("*.png"):
            old.unlink()
        # Each run its own process and folder: the shots' prefix is a run's own switch.
        cmd = [sys.executable, str(HERE / "regress.py"), "run", "--game", o.game, "--routes", str(routes),
               "--set", "survey", "--only", f"{o.level:02d}-survey-{j}", "--out", str(run_dir / str(j)),
               "--jobs", "1", "--env", "OPENRAC_SOFTWARE_GL=1", "--env", f"OPENRAC_HIDE_PASSES={o.pass_}",
               "--env", f"OPENRAC_HIDE_PASSES_EVERY={EVERY}", "--env", f"OPENRAC_SHOT={EVERY}:{folder}/s_",
               "--env", "OPENRAC_SHOT_FROM=1100", "--setting", "resolution=640x480", "--setting", "render_scale=50"]
        if o.build:
            cmd += ["--build", o.build]
        procs.append(subprocess.Popen(cmd, env={**os.environ, "SDL_VIDEODRIVER": "offscreen"}))
    print(f"{len(pts)} places in {jobs} runs, {HOLD} frames each")
    for p in procs:
        p.wait()
    rows = locate([run_dir / str(j) / f"{o.level:02d}-survey-{j}" / "trace.jsonl" for j in range(jobs)],
                  [f"{shots / str(j)}/s_" for j in range(jobs)], out)
    for r in rows[:8]:
        print(f"{r['share']:.2%}  hero {r['hero']}  yaw {r['yaw']:.3f}  frame {r['frame']}")
    print(out / "best.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
