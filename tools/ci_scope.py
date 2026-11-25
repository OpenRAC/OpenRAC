# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Decide which checks a pull request needs, from the files it changes.

Pushes and manual runs check everything. A pull request skips what its files cannot affect:
the launcher's jobs when it changes only port/, the port's jobs when it changes only launcher/,
and builds of the games it does not touch, and any build for Markdown that no build or test reads
(MARKDOWN_READ lists those that are read). A release carried from another (a hostgen.json with
"release": rac1-pal1, rac1-jp, rac2-ntsc2, rac2-pal) is built in a pull request when its own
folder, its game's other port folders or its game's decomp change, not for shared code. Going
Commando's PAL and USA v2.00 (PLAYED) are built whenever its USA disc is, since a change to what
they share can break them alone (an argument their C drops crashed only there). Anything unclear
(a workflow, this file, a failed git call) checks everything.
Two tiers (TIERS): a pull request runs the fast tier, the checks that answer in a few minutes;
the merge queue (merge_group) runs the full tier for the queued changes before they land, so
nothing reaches develop without every check. The full tier adds the Windows jobs, the enhanced
Vulkan build and its tests, and the carried releases.
`ci_scope.py ios` does the same for the iOS jobs, for pushes too.
`ci_scope.py decomp-games` prints which games' decomp tool checks a change needs.
`ci_scope.py launcher-changed` (SINCE: a commit) tells whether develop changed what the launcher's
build reads since the last development build (launcher.yml's push paths).
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
    from carried()."""
    result = full_plan(paths, known, shared, removed)
    full = tier == "full"
    if not full:
        # A carried release in the fast tier only when the change is in its own folder.
        own = {path.split("/")[2] for path in paths or () if path.startswith("port/game/")
               and len(path.split("/")) > 3}
        result["port_parts"] = [part for part in result["port_parts"]
                                if part not in releases or part in own]
        result["enhanced"] = False
    # Windows builds rac1-pal with the shared code, and runs the launcher's Rust tests.
    result["windows"] = full and result["port"]
    result["launcher_systems"] = (["Linux", "Windows"] if full else ["Linux"]) if result["launcher"] else []
    return result


def full_plan(paths, known, shared=frozenset(), removed=frozenset()):
    """Every check PATHS can affect (None: everything); the arguments are plan()'s."""
    every = sorted(known)
    everything = {"docs_only": False, "launcher": True, "pages": True, "port": True,
                  "enhanced": True, "port_parts": ["common", *every]}
    if paths is None or any(path.startswith(EVERYTHING) for path in paths):
        return everything
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
    parts, enhanced = set(), False
    for path in port_paths:
        bits = path.split("/")
        if bits[:2] == ["port", "game"] and len(bits) > 3 and bits[2] != "common":
            # port/game/<program>/, or port/game/rac1/ (what rac1's programs share). A release
            # carries its game's shared host C and its primary's tables, so a change to either
            # builds the game's releases too; a release's own folder builds only itself.
            game = bits[2].split("-")[0]
            mine = [p for p in every if p == bits[2] or p.startswith(game + "-")]
            if mine:
                if bits[2] in known and known[bits[2]]:
                    parts.add(bits[2])
                else:
                    parts.update(p for p in mine if p == bits[2] or p.startswith(bits[2] + "-") or known[p])
                if bits[2] in shared:
                    parts.add("common")
                    enhanced = True
                continue
        if bits[0] == "games" and len(bits) > 2:
            # A decomp change reaches every program of the game, its releases too (PAL v1.00's
            # memory card hang came in with a decomp import).
            parts.update(p for p in every if p.startswith(bits[1] + "-"))
            continue
        # Anything else the port reads: the shared part and every game it builds itself.
        parts.update(["common", *(p for p in every if not known[p])])
        enhanced = True
    # Windows builds rac1-pal with the shared code.
    port = "common" in parts or "rac1-pal" in parts
    ordered = [part for part in ["common", *every] if part in parts]
    return {"docs_only": False, "launcher": launcher, "pages": pages, "port": port,
            "enhanced": enhanced, "port_parts": ordered}


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


LAUNCHER_WORKFLOW = ".github/workflows/launcher.yml"


def launcher_paths(root=ROOT):
    """The path filters of launcher.yml's push trigger, read from the workflow itself."""
    lines = (root / LAUNCHER_WORKFLOW).read_text(encoding="utf-8").splitlines()
    start = next(i for i, line in enumerate(lines) if line.strip() == "paths:")
    found = []
    for line in lines[start + 1:]:
        text = line.strip()
        if not text or text.startswith("#"):
            continue
        if not text.startswith("- "):
            break
        found.append(text[2:].strip().strip('"\''))
    return found


def glob_regex(pattern):
    """A GitHub Actions path filter as a regular expression: * stays in a folder, ** does not."""
    out, i = "", 0
    while i < len(pattern):
        if pattern.startswith("**", i):
            out, i = out + ".*", i + 2
        elif pattern[i] == "*":
            out, i = out + "[^/]*", i + 1
        else:
            out, i = out + re.escape(pattern[i]), i + 1
    return re.compile(out + "$")


def launcher_changed(paths, filters):
    """Whether PATHS (None: unknown) touch any of launcher.yml's push FILTERS."""
    if paths is None:
        return True
    patterns = [glob_regex(f) for f in filters]
    return any(p.match(path) for p in patterns for path in paths)


def event_paths():
    """The files this run checks and its tier: a pull request's merge (fast tier), a merge queue
    group's changes over develop (full tier), or None for everything (a push or a manual run)."""
    event = os.environ.get("GITHUB_EVENT_NAME")
    if event == "pull_request":
        return changed_paths(os.environ.get("PR_BASE", "")), "fast"
    if event == "merge_group" and os.environ.get("MERGE_BASE"):
        return pushed_paths(os.environ["MERGE_BASE"]), "full"
    return None, "full"


if __name__ == "__main__":
    paths, tier = event_paths()
    if sys.argv[1:] == ["ios"]:
        # PUSH_BEFORE: the branch's commit before the push.
        if paths is None and os.environ.get("GITHUB_EVENT_NAME") == "push" and os.environ.get("PUSH_BEFORE"):
            paths = pushed_paths(os.environ["PUSH_BEFORE"])
        result = ios_plan(paths, shared_game_dirs())
    elif sys.argv[1:] == ["launcher-changed"]:
        since = os.environ.get("SINCE")
        # Unlike pushed_paths, an empty diff is an answer here: nothing new, no build.
        diff = subprocess.run(["git", "diff", "--no-renames", "--name-only", "-z", since or "", "HEAD", "--"],
                              capture_output=True, check=False) if since else None
        paths = (None if diff is None or diff.returncode else
                 [path.decode("utf-8", "surrogateescape") for path in diff.stdout.split(b"\0")[:-1]])
        result = {"build": launcher_changed(paths, launcher_paths())}
    elif sys.argv[1:] == ["decomp-games"]:
        result = {"games": decomp_games(paths if tier == "fast" else None)}
    else:
        removed = frozenset(path for path in paths or () if not (ROOT / path).exists())
        result = plan(paths, programs(), shared_game_dirs(), removed, tier, carried())
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        for key, value in result.items():
            print(f"{key}={json.dumps(value) if isinstance(value, list) else str(value).lower()}",
                  file=output)
