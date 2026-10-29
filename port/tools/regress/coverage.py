# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The OPENRAC_DRAW_COVERAGE files of runs, merged: per version, what the game asked to draw that
the port left undrawn (frontend/draw_coverage.h).

Two kinds of rows: a VU1 program start no renderer of the port reads, by its signature (command,
entry, the microcode's hash and the layout of the data unpacked for it), and a particle type some of
whose particles particle_quads leaves out, by reason. Each is ranked by how many routes saw it, then
by its count, with an example of where its data came from in game memory. A start SETUP names
(a program's set-up, which draws nothing) is left out.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path


# Program starts that draw nothing, by their microcode's hash and entry: what each sets up.
SETUP = {
    ("d859b12145f6f10e", "0x0"): "the effect strip program's set-up (its constants at 0x3A4, BASE and OFFSET)",
}


@dataclass
class Program:
    command: str
    entry: str
    code: str
    layout: str
    routes: set = field(default_factory=set)
    calls: int = 0
    undrawn: int = 0
    example: dict = field(default_factory=dict)

    def as_dict(self) -> dict:
        return {"command": self.command, "entry": self.entry, "code": self.code, "layout": self.layout,
                "routes": sorted(map(str, self.routes)), "calls": self.calls, "undrawn": self.undrawn,
                "example": self.example}


@dataclass
class Particle:
    type: int
    update: str
    routes: set = field(default_factory=set)
    drawn: int = 0
    left_out: dict = field(default_factory=dict)

    def as_dict(self) -> dict:
        return {"type": self.type, "update": self.update, "routes": sorted(map(str, self.routes)),
                "drawn": self.drawn, "left_out": self.left_out}


def merge(files: list[tuple[str, int | str, dict]]) -> dict[str, tuple[list[Program], list[Particle]]]:
    """files: (game, the route, the coverage file's JSON). Returns each game's undrawn rows, ranked."""
    programs: dict[str, dict[tuple, Program]] = {}
    particles: dict[str, dict[tuple, Particle]] = {}
    for game, route, coverage in files:
        for level, entry in coverage.get("levels", {}).items():
            where = f"{route}@{level}" if str(route) != str(level) else str(level)
            for p in entry.get("programs", []):
                undrawn = int(p["calls"]) - int(p["drawn"])
                if undrawn <= 0 or (p["code"], p["entry"]) in SETUP:
                    continue
                key = (p["command"], p["entry"], p["code"], p["layout"])
                row = programs.setdefault(game, {}).setdefault(key, Program(*key))
                row.routes.add(where)
                row.calls += int(p["calls"])
                row.undrawn += undrawn
                if not row.example:
                    row.example = {k: p[k] for k in ("ram", "alpha", "test", "tex0", "sample") if k in p}
                    row.example["level"] = level
            for p in entry.get("particles", []):
                if not p.get("left_out"):
                    continue
                # By the update's name: each level's program has the function at an address of its own.
                key = (int(p["type"]), p.get("update", "?").split(" ", 1)[-1])
                row = particles.setdefault(game, {}).setdefault(key, Particle(*key))
                row.routes.add(where)
                row.drawn += int(p.get("drawn", 0))
                for reason, n in p["left_out"].items():
                    row.left_out[reason] = row.left_out.get(reason, 0) + int(n)
    out = {}
    for game in sorted(set(programs) | set(particles)):
        ps = sorted(programs.get(game, {}).values(), key=lambda r: (-len(r.routes), -r.undrawn, r.entry))
        qs = sorted(particles.get(game, {}).values(), key=lambda r: (-len(r.routes), -sum(r.left_out.values()), r.type))
        out[game] = (ps, qs)
    return out


def load_run(run: Path) -> list[tuple[str, int | str, dict]]:
    """A run's coverage files: (game, the level or the route's name, the file's JSON)."""
    from runner import run_routes  # noqa: PLC0415 (runner imports nothing of this)

    meta = run / "run.json"
    game = json.loads(meta.read_text()).get("game", "?") if meta.exists() else "?"
    out = []
    for key, (_, _, folder) in sorted(run_routes(run).items(), key=lambda kv: str(kv[0])):
        f = folder / "coverage.json"
        if f.exists():
            try:
                out.append((game, key, json.loads(f.read_text())))
            except json.JSONDecodeError:
                continue
    return out


def table(game: str, programs: list[Program], particles: list[Particle], top: int) -> str:
    lines = [f"{game}: {len(programs)} VU1 program signatures left undrawn",
             "  routes   undrawn  command  entry    code              layout"]
    for r in programs[:top]:
        lines.append(f"  {len(r.routes):6d}  {r.undrawn:8d}  {r.command:<7}  {r.entry:<7}  {r.code}  {r.layout}"
                     f"  (level {r.example.get('level')}, data at {r.example.get('ram')})")
    lines.append(f"{game}: {len(particles)} particle types with particles left out")
    lines.append("  routes     drawn  type  left out                         update")
    for r in particles[:top]:
        left = ", ".join(f"{k} {v}" for k, v in sorted(r.left_out.items()))
        lines.append(f"  {len(r.routes):6d}  {r.drawn:8d}  {r.type:4d}  {left:<31}  {r.update}")
    return "\n".join(lines)
