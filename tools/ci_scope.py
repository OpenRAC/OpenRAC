# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Decide which checks a pull request needs, from the files it changes.

Pushes and manual runs check everything. A pull request skips what its files cannot affect:
the launcher's jobs when it changes only port/, the port's jobs when it changes only launcher/,
and builds of the games it does not touch. A release carried from another (a hostgen.json with
"release": rac1-pal1, rac1-jp, rac2-ntsc2, rac2-pal) is built in a pull request only when its
own folder changes. Anything unclear (a workflow, this file, a failed git call) checks everything.
"""

import json
import os
import re
from pathlib import Path
import subprocess

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


def is_docs(path):
    # The native env test checks DEBUGGING.md against the environment registry.
    return path.startswith("docs/") and path.endswith(".md") and path != "docs/port/DEBUGGING.md"


def docs_only(base):
    """True when the pull request changes only documentation under docs/."""
    paths = changed_paths(base)
    return bool(paths) and all(map(is_docs, paths))


def programs(root=ROOT):
    """Each game program's id, and whether it is a release carried from another."""
    found = {}
    for path in sorted((root / "port/game").glob("*/hostgen.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        found[data["id"]] = "release" in data
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


def plan(paths, known, shared=frozenset()):
    """What to run for PATHS (None: everything), with KNOWN from programs() and SHARED from
    shared_game_dirs()."""
    every = sorted(known)
    everything = {"docs_only": False, "launcher": True, "pages": True, "port": True,
                  "enhanced": True, "port_parts": ["common", *every]}
    if paths is None or any(path.startswith(EVERYTHING) for path in paths):
        return everything
    if all(map(is_docs, paths)):
        return {**everything, "docs_only": True, "launcher": False, "pages": False, "port": False,
                "enhanced": False, "port_parts": []}
    # Plain text that no build or test reads.
    quiet = [path for path in paths if path.startswith(PORT_READS) or not (
        is_docs(path) or ("/" not in path and path.endswith(".md")))]
    launcher = any(not path.startswith(("port/", "docs/")) or LAUNCHER_READS.match(path)
                   for path in quiet)
    pages = any(path.startswith("launcher/") or PAGES_READS.match(path) for path in quiet)
    port_paths = [path for path in quiet
                  if path.startswith(PORT_READS) or not path.startswith(("launcher/", "docs/"))]
    parts, enhanced = set(), False
    for path in port_paths:
        bits = path.split("/")
        if bits[:2] == ["port", "game"] and len(bits) > 3 and bits[2] != "common":
            # port/game/<program>/, or port/game/rac1/ (what rac1's programs share).
            mine = [p for p in every if p == bits[2] or p.startswith(bits[2] + "-")]
            if mine:
                parts.update(p for p in mine if p == bits[2] or not known[p])
                if bits[2] in shared:
                    parts.add("common")
                    enhanced = True
                continue
        if bits[0] == "games" and len(bits) > 2:
            parts.update(p for p in every if p.startswith(bits[1] + "-") and not known[p])
            continue
        # Anything else the port reads: the shared part and every game it builds itself.
        parts.update(["common", *(p for p in every if not known[p])])
        enhanced = True
    # Fedora and Windows build rac1-pal with the shared code.
    port = "common" in parts or "rac1-pal" in parts
    ordered = [part for part in ["common", *every] if part in parts]
    return {"docs_only": False, "launcher": launcher, "pages": pages, "port": port,
            "enhanced": enhanced, "port_parts": ordered}


if __name__ == "__main__":
    pull = os.environ.get("GITHUB_EVENT_NAME") == "pull_request"
    known = programs()
    result = plan(changed_paths(os.environ.get("PR_BASE", "")) if pull else None, known,
                  shared_game_dirs())
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        for key, value in result.items():
            print(f"{key}={json.dumps(value) if isinstance(value, list) else str(value).lower()}",
                  file=output)
