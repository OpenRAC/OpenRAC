# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""
The games' manifests: games/<game>/game.json, read one way.

  from openrac_common import games
  rac1 = games.load("rac1")                     # one manifest
  for game, name, version in games.versions():  # every version of every game, in game order
      print(game["id"], name, version["serial"], games.version_dir(game, name))

Written from tools/openrac.py's vdir, games and versions, the most complete of the five readers
there were. tools/openrac.py, extractor.py, sources.py and xmap.py, versionmap and the editor
read the manifests through it. A manifest
is returned as the parsed JSON, unchanged: its fields are described in docs/LAYOUT.md and each
game's README, and this module does not check them.

source_checkout is the one rule for where a version's sister project (its matching decomp, synced
into games/) is checked out, written from tools/openrac.py's source_checkout and tools/sources.py's
checkout, which said the same thing twice. tools/fcheck_gate.py finds rac1-decomp by it.
"""
import json
import os
from pathlib import Path

from .paths import repo_root


def games_dir(root: Path | str | None = None) -> Path:
    """The games/ folder under ROOT, the top of OpenRAC (default: the checkout holding this package)."""
    return (Path(root) if root is not None else repo_root()) / "games"


def manifest_path(game: str, root: Path | str | None = None) -> Path:
    """Where GAME's manifest is: games/<game>/game.json under ROOT (default: this checkout)."""
    return games_dir(root) / game / "game.json"


def load(game: str, root: Path | str | None = None) -> dict:
    """GAME's manifest (games/<game>/game.json), parsed. Raises FileNotFoundError for a game
    OpenRAC does not have."""
    return json.loads(manifest_path(game, root).read_text(encoding="utf-8"))


def load_all(root: Path | str | None = None) -> list[dict]:
    """Every game's manifest, in game order (the directories' names sort as the release order)."""
    return [json.loads(p.read_text(encoding="utf-8")) for p in sorted(games_dir(root).glob("*/game.json"))]


def versions(root: Path | str | None = None) -> list[tuple[dict, str, dict]]:
    """(game, version name, version) for every version of every game, in game order and in each
    manifest's own order of versions."""
    return [(game, name, version) for game in load_all(root) for name, version in game["versions"].items()]


def version_dir(game: dict, name: str) -> str:
    """Version NAME's directory, relative to the top of OpenRAC: its "dir" in game.json (a release
    built from another version's tree: rac1's PAL v1.00 from games/rac1/pal), else games/<game>/<version>."""
    return game["versions"][name].get("dir", f"games/{game['id']}/{name}")


def source_variable(game_id: str, version_name: str) -> str:
    """The environment variable that names another checkout of a version's source project, used
    instead of its submodule: OPENRAC_SOURCE_<GAME>_<VERSION> in capitals (OPENRAC_SOURCE_RAC1_PAL)."""
    return f"OPENRAC_SOURCE_{game_id}_{version_name}".upper()


def source_checkout(game_id: str, version_name: str, root: Path | str | None = None) -> Path | None:
    """Where version VERSION_NAME of game GAME_ID has its source project checked out, or None when
    its game.json names no source project: the variable source_variable names, when it is set and
    not empty (a full clone kept elsewhere), else the version's directory, a git submodule of OpenRAC
    (docs/SOURCES.md). A version built from another's tree ("shared": rac2's PAL and v2.00) gives
    that tree. Not checked to exist: the caller says what is missing. Raises FileNotFoundError for a
    game OpenRAC does not have and KeyError for a version it does not have."""
    game = load(game_id, root)
    source = game["versions"][version_name].get("source")
    if not source:
        return None
    if source.get("shared"):
        return source_checkout(game_id, source["shared"], root)
    override = os.environ.get(source_variable(game_id, version_name))
    if override:
        return Path(override)
    return (Path(root) if root is not None else repo_root()) / version_dir(game, version_name)
