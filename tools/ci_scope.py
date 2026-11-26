# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Decide which checks a run needs, from the files it changes.

The games (hostgen and each version's build) and Windows build only when everything is checked:
master's merge queue, which a dev release goes through (the workflow gives no paths then), and a
manual run. A game that breaks there is fixed on develop. Pull requests, develop's merge queue and
develop pushes build the port's shared part alone, and
skip what their files cannot affect: the launcher's jobs when they change only port/, the port's
jobs when they change only launcher/, and any build for Markdown that no build or test reads
(MARKDOWN_READ lists those that are read). Anything unclear (a workflow, this file, a failed git
call) checks everything those runs check.
Two tiers (TIERS): a pull request gets the fast tier, of which checks.yml reads only "launcher",
"pages" and "docs_only" (a pull request to develop runs only the lint jobs); the merge queue
(merge_group) and develop pushes (PUSH_BEFORE) run the full tier, which adds the enhanced Vulkan
build and its tests, and the launcher for any change outside port/.
`ci_scope.py ios` does the same for the iOS jobs, for pushes too.
`ci_scope.py decomp-games` prints which games' decomp tool checks a change needs.
"""

import json
import os
import re
from pathlib import Path
import subprocess
import sys

# The checkout: the workflow runs the base branch's copy of this file from elsewhere.
ROOT = Path.cwd()

# Paths whose change checks everything.
EVERYTHING = (".github/", "tools/ci_scope.py", "tools/ci_port_tests.py")


def changed_paths(base):
    """The files the pull request's merge commit changes, or None when that cannot be told."""
    try:
        subprocess.run(["git", "rev-parse", "--verify", "HEAD^2"],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        parent = subprocess.check_output(["git", "rev-parse", "HEAD^1"]).strip()
        if parent != base.encode():
            return None
        diff = subprocess.check_output(
            ["git", "diff", "--no-renames", "--name-only", "-z", "HEAD^1", "HEAD", "--"])
    except (OSError, subprocess.CalledProcessError):
        return None
    paths = [path.decode("utf-8", "surrogateescape") for path in diff.split(b"\0")[:-1]]
    return paths or None


def pushed_paths(before):
    """Files changed since BEFORE, or None on error."""
    try:
        diff = subprocess.check_output(
            ["git", "diff", "--no-renames", "--name-only", "-z", before, "HEAD", "--"])
    except (OSError, subprocess.CalledProcessError):
        return None
    paths = [path.decode("utf-8", "surrogateescape") for path in diff.split(b"\0")[:-1]]
    return paths or None


# Markdown a build or test reads: the native env test checks DEBUGGING.md against its registry.
MARKDOWN_READ = ("docs/port/DEBUGGING.md",)


def is_docs(path):
    return path.startswith("docs/") and path.endswith(".md") and path not in MARKDOWN_READ


def is_text(path, removed=frozenset()):
    """True for Markdown no build or test reads. A removed page still checks as before: the
    launcher's test wants every page actions.json links to exist."""
    return path.endswith(".md") and path not in MARKDOWN_READ and path not in removed


def docs_only(base):
    """True when the pull request changes only documentation under docs/."""
    paths = changed_paths(base)
    return bool(paths) and all(map(is_docs, paths))


# Releases carried from another that players play: built whenever their primary is.
PLAYED = ("rac2-pal", "rac2-ntsc2")


def carried(root=ROOT):
    """The programs built from another program's code (hostgen.json "release"): in the full tier."""
    return {json.loads(path.read_text(encoding="utf-8"))["id"]
            for path in sorted((root / "port/game").glob("*/hostgen.json"))
            if "release" in json.loads(path.read_text(encoding="utf-8"))}


def programs(root=ROOT):
    """Each game program's id, and whether it is a release carried from another (and not PLAYED)."""
    found = {}
    for path in sorted((root / "port/game").glob("*/hostgen.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        found[data["id"]] = "release" in data and data["id"] not in PLAYED
    return found


# Files outside the port that its build reads (the icons, the version).
PORT_READS = ("launcher/src-tauri/icons/", "launcher/src-tauri/tauri.conf.json", "docs/port/DEBUGGING.md")
# Files outside launcher/ that the launcher's Rust side reads (include_str!, its checkout tests).
LAUNCHER_READS = re.compile(r"^port/(game/[^/]+/(differences\.json|icon\.png|hostgen\.json)$"
                            r"|platform/bindings\.(cpp|h)$|enhanced/lighting/profiles/)")
# Files outside launcher/ that the launcher's pages import (src/lib/mock.ts and the tests).
PAGES_READS = re.compile(r"^(games/[^/]+/game\.json|port/game/[^/]+/differences\.json"
                         r"|port/platform/bindings\.(cpp|h)|progress/summary\.json"
                         r"|sdk/include/openrac/keys\.h)$")
# What else the launcher's build reads that a pull request still checks with it (the games'
# descriptions, progress, the build scripts it runs); the rest of the conservative rule above is
# left to the merge queue.
FAST_LAUNCHER_READS = re.compile(r"^(games/[^/]+/game\.json|progress/|sdk/|Cargo\.|tools/(build_|bundle_|openrac\.py|updater_manifest\.py))")
SHARED_SOURCES = ("port/CMakeLists.txt", "port/tests", "port/renderer", "port/runtime", "port/platform",
                  "port/scene", "port/enhanced", "port/viewer", "port/game/common")


def shared_game_dirs(root=ROOT):
    """The port/game/<dir> folders the shared build and its tests compile or read: a change there
    also needs the shared part and the enhanced build."""
    found = set()
    for top in SHARED_SOURCES:
        base = root / top
        files = [base] if base.is_file() else (p for p in base.rglob("*") if p.is_file())
        for path in files:
            if path.suffix not in (".txt", ".cmake", ".c", ".cpp", ".h", ".hpp", ".py", ".json"):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            if path.name == "CMakeLists.txt":
                text = text.split("if(OPENRAC_PORT_GAMES)")[0]
            found.update(re.findall(r"game/([A-Za-z0-9_-]+)/", text))
    found.discard("common")
    return found


TIERS = ("fast", "full")


def plan(paths, known, shared=frozenset(), removed=frozenset(), tier="full", releases=frozenset()):
    """What to run for PATHS (None: everything), with KNOWN from programs(), SHARED from
    shared_game_dirs(), REMOVED the paths the change deletes, TIER one of TIERS and RELEASES
    from carried() (kept for the callers; every release builds when everything is checked)."""
    result = full_plan(paths, known, shared, removed)
    full = tier == "full"
    # Everything (paths None): a dev release or a manual run. The games (hostgen
    # and their builds) and Windows build only then: pull requests to develop, the queue and
    # develop pushes build the shared part alone.
    everything = paths is None
    if not full:
        result["enhanced"] = False
        # The launcher only for what its build reads by name; anything else waits for the queue.
        result["launcher"] = paths is None or any(
            path.startswith(("launcher/", *EVERYTHING)) or LAUNCHER_READS.match(path)
            or FAST_LAUNCHER_READS.match(path) for path in paths)
    # Windows (rac1-pal with the shared code, the launcher's Rust tests) only with everything.
    result["windows"] = full and everything and result["port"]
    result["launcher_systems"] = ((["Linux", "Windows"] if full and everything else ["Linux"])
                                  if result["launcher"] else [])
    return result


def full_plan(paths, known, shared=frozenset(), removed=frozenset()):
    """Every check PATHS can affect (None: everything, every game's build too); the arguments are
    plan()'s. Short of everything, the port builds only its shared part (port_parts ["common"])."""
    every = sorted(known)
    everything = {"docs_only": False, "launcher": True, "pages": True, "port": True,
                  "enhanced": True, "port_parts": ["common", *every]}
    if paths is None:
        return everything
    if any(path.startswith(EVERYTHING) for path in paths):
        return {**everything, "port_parts": ["common"]}
    if all(map(is_docs, paths)):
        return {**everything, "docs_only": True, "launcher": False, "pages": False, "port": False,
                "enhanced": False, "port_parts": []}
    # Markdown that no build or test reads, wherever it is (port/docs, sdk/docs, release notes).
    quiet = [path for path in paths if path.startswith(PORT_READS) or not is_text(path, removed)]
    launcher = any(not path.startswith(("port/", "docs/")) or LAUNCHER_READS.match(path)
                   for path in quiet)
    pages = any(path.startswith("launcher/") or PAGES_READS.match(path) for path in quiet)
    port_paths = [path for path in quiet
                  if path.startswith(PORT_READS) or not path.startswith(("launcher/", "docs/"))]
    common = False
    for path in port_paths:
        bits = path.split("/")
        if bits[:2] == ["port", "game"] and len(bits) > 3 and bits[2] != "common":
            # A game's own folder (port/game/<program>/ or port/game/rac1/): its build is the
            # dev release's; the shared part only when its tests compile that folder.
            game = bits[2].split("-")[0]
            if any(p == bits[2] or p.startswith(game + "-") for p in every):
                common = common or bits[2] in shared
                continue
        if bits[0] == "games" and len(bits) > 2:
            # A decomp: the repository job checks its tools; the games build for a dev release.
            continue
        # Anything else the port reads: the shared part and its tests.
        common = True
    return {"docs_only": False, "launcher": launcher, "pages": pages, "port": common,
            "enhanced": common, "port_parts": ["common"] if common else []}


# The games whose decomp has tool checks of its own in the repository job (checks.yml).
DECOMP_GAMES = ("rac1", "rac2", "rac4", "rac5", "rac6")


def decomp_games(paths):
    """The games of DECOMP_GAMES whose tool checks PATHS (None: everything) needs."""
    if paths is None or any(path.startswith(EVERYTHING) for path in paths):
        return list(DECOMP_GAMES)
    return [game for game in DECOMP_GAMES
            if any(path.startswith(f"games/{game}/") or path == ".gitmodules" for path in paths)]


# A change here runs both iOS jobs.
IOS_EVERYTHING = (".github/workflows/ios.yml", "tools/ci_scope.py")
# Files the iOS launcher job reads (core's tests are not built there).
IOS_LAUNCHER = ("launcher/ios/", "launcher/core/", "launcher/Cargo.toml", "launcher/Cargo.lock",
                "launcher/package.json", "launcher/src/lib/palettes.json",
                "launcher/src-tauri/icons/icon.png", "port/ios/", "port/platform/ios.h",
                ".github/actions/launcher-toolchain/", ".github/actions/yarn-cache/")


def ios_port_reads(path, shared):
    """True if the iOS port job reads PATH."""
    if path.startswith(PORT_READS) or re.fullmatch(r"games/[^/]+/game\.json", path):
        return True
    if not path.startswith("port/") or path.endswith(".md"):
        return False
    bits = path.split("/")
    if bits[1] in ("android", "tools"):
        return False
    if bits[1] == "game" and len(bits) > 3:
        return bits[2] == "common" or bits[2] in shared or bits[3] == "differences.json"
    return True


def ios_plan(paths, shared=frozenset()):
    """Which iOS jobs PATHS need (None: both)."""
    if paths is None or any(path in IOS_EVERYTHING for path in paths):
        return {"ios_port": True, "ios_launcher": True}
    return {"ios_port": any(ios_port_reads(path, shared) for path in paths),
            "ios_launcher": any(path.startswith(IOS_LAUNCHER)
                                or re.fullmatch(r"port/game/[^/]+/differences\.json", path)
                                for path in paths)}


def event_paths():
    """The files this run checks and its tier: a pull request's merge (fast tier), a merge queue
    group's changes over develop (full tier), a develop push's changes (full tier, PUSH_BEFORE),
    or None for everything (another push or a manual run)."""
    event = os.environ.get("GITHUB_EVENT_NAME")
    if event == "pull_request":
        return changed_paths(os.environ.get("PR_BASE", "")), "fast"
    if event == "merge_group" and os.environ.get("MERGE_BASE"):
        return pushed_paths(os.environ["MERGE_BASE"]), "full"
    if event == "push" and os.environ.get("PUSH_BEFORE"):
        return pushed_paths(os.environ["PUSH_BEFORE"]), "full"
    return None, "full"


if __name__ == "__main__":
    paths, tier = event_paths()
    if sys.argv[1:] == ["ios"]:
        result = ios_plan(paths, shared_game_dirs())
    elif sys.argv[1:] == ["decomp-games"]:
        result = {"games": decomp_games(paths if tier == "fast" else None)}
    else:
        removed = frozenset(path for path in paths or () if not (ROOT / path).exists())
        result = plan(paths, programs(), shared_game_dirs(), removed, tier, carried())
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        for key, value in result.items():
            print(f"{key}={json.dumps(value) if isinstance(value, list) else str(value).lower()}",
                  file=output)
