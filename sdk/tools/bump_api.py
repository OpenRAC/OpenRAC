#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""The SDK's version: worked out from the releases, never chosen by hand.

    python3 sdk/tools/bump_api.py                               set the version (and what follows it)
    python3 sdk/tools/bump_api.py minor --note "..."            add a note for the open minor, then the same
    python3 sdk/tools/bump_api.py --check                       write nothing; fail when something is off
    python3 sdk/tools/bump_api.py --release-version [--stable]  the version a release of this tree gets
    python3 sdk/tools/bump_api.py --versions                    each SDK and launcher release's interface

Releases are the tags sdk-v<major>.M.P of OpenRAC/OpenRAC, made by CI (.github/workflows/sdk.yml):

    <major>.M.0   a stable release, made for a stable launcher (a launcher-v* tag)
    <major>.M.P   a dev release (P > 0), made for each release merged to master

The first release of a major is <major>.0.0. Develop has one open minor, from the last stable M:

    M + 1   when this tree adds to the interface since that tag: a new table member, or a new row
            in types.lock or macros.lock from openrac_mod.h or keys.h, or a note in
            sdk/changes/<major>.<M+1>/
    M       otherwise (rows from games/*.h, hook points, are not a minor)

Every pull request that adds something joins the open minor, so two of them pick the same number
and never renumber each other. Dev releases carry it as patches of M (their headers already say
M + 1); the next stable release closes it as <major>.<M+1>.0.

The minor is in one place, openrac_mod.h's OPENRAC_MOD_API_MINOR: the CMake package and the
launcher read it there, and package_sdk.py writes the release's version into the archive. Run with
no arguments, the tool sets it, the `since` of the members new since the stable tag
(sdk/api/members.tsv), their "Since <major>.N." in the header, the `api_minor >= N` of the mod.hpp
checks that read them and the port's stubs, and moves notes from a released minor's folder, then
runs gen_api_lock.py and gen_reference.py. It prints the lines added since the tag that still name
the closed minor, to be looked at by hand.

Notes: each change of the interface adds sdk/changes/<major>.<open minor>/<name>.md, one paragraph
that names the new members; the changelog (gen_reference.py) puts them together. The first minor of
a major, <major>.0, is the whole table: it needs no note and no Since of its own.

Tags come from `git ls-remote` of OPENRAC_SDK_REPO (default https://github.com/OpenRAC/OpenRAC);
OPENRAC_SDK_OFFLINE=1 uses the local tags only. Stdlib only.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

import package_sdk

ROOT = Path(__file__).resolve().parents[2]
HEADER = Path("sdk/include/openrac/openrac_mod.h")
KEYS = Path("sdk/include/openrac/keys.h")
CPP = Path("sdk/include/openrac/mod.hpp")
MEMBERS = Path("sdk/api/members.tsv")
API_LOCKS = ("openrac_api.lock", "types.lock", "macros.lock")
CHANGES = Path("sdk/changes")
REPO = "https://github.com/OpenRAC/OpenRAC"
TAG = re.compile(r"^sdk-v(\d+)\.(\d+)\.(\d+)$")
LAUNCHER_TAG = re.compile(r"^launcher-v(\d+)\.(\d+)\.(\d+)$")
NOTE_NAME = re.compile(r"^[a-z0-9][a-z0-9._-]*$")


# ---- Reading the tree ----

def read(root: Path, path: Path) -> str:
    return (root / path).read_text(encoding="utf-8")


def write(root: Path, path: Path, text: str) -> None:
    put(root / path, text)


def put(path: Path, text: str) -> None:
    """Written with \\n line ends on every system, as git has the files."""
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def found(pattern: str, text: str, where: Path) -> re.Match:
    m = re.search(pattern, text, re.M)
    if not m:
        raise SystemExit(f"{where}: no {pattern!r}")
    return m


def header_api(root: Path) -> tuple[int, int]:
    text = read(root, HEADER)
    return (int(found(r"^#define OPENRAC_MOD_API_MAJOR (\d+)$", text, HEADER).group(1)),
            int(found(r"^#define OPENRAC_MOD_API_MINOR (\d+)$", text, HEADER).group(1)))


def problems(root: Path, minor: int) -> list[str]:
    """Whether the header says the minor it should (the one place that holds it: the CMake package,
    the launcher and the archive read it there)."""
    major, have = header_api(root)
    return [] if have == minor else [f"{HEADER} says {major}.{have}, the interface is {major}.{minor}"]


def lock_rows(text: str) -> list[str]:
    return [l for l in text.splitlines() if l and not l.startswith("#")]


def lock_key(lock: str, row: str) -> str:
    """A row's name, which never changes (its value may only for a FIXED row or a moving macro)."""
    c = row.split("\t")
    if lock == "openrac_api.lock":
        return c[1]
    if lock == "types.lock":
        return f"{c[0]}\t{c[1]}"
    return c[0]


def core_names(root: Path) -> tuple[set[str], set[str]]:
    """The macros openrac_mod.h and keys.h define, and every word in them (a type of theirs is one).
    A games/*.h type that a core comment names counts as core: that errs toward a minor."""
    text = read(root, HEADER) + "\n" + ((root / KEYS).read_text(encoding="utf-8") if (root / KEYS).exists() else "")
    defined = set(re.findall(r"^\s*#\s*define\s+(OPENRAC_\w+)", text, re.M))
    return defined, set(re.findall(r"\b\w+\b", text))


def is_core(lock: str, key: str, core: tuple[set[str], set[str]]) -> bool:
    if lock == "openrac_api.lock":
        return True
    if lock == "macros.lock":
        return key in core[0]
    return key.split("\t")[0] in core[1]


def notes(root: Path, minor: int) -> dict[str, str]:
    """The notes of a minor: sdk/changes/<major>.<minor>/<name>.md, by path."""
    name = f"{header_api(root)[0]}.{minor}"
    folder = root / CHANGES / name
    if not folder.is_dir():
        return {}
    return {(CHANGES / name / p.name).as_posix(): p.read_text(encoding="utf-8").strip()
            for p in sorted(folder.glob("*.md"))}


def note_folders(root: Path) -> dict[int, list[str]]:
    out: dict[int, list[str]] = {}
    folder = root / CHANGES
    for d in sorted(folder.iterdir()) if folder.is_dir() else []:
        m = re.fullmatch(rf"{header_api(root)[0]}\.(\d+)", d.name)
        if d.is_dir() and m:
            out[int(m.group(1))] = [(CHANGES / d.name / p.name).as_posix() for p in sorted(d.glob("*.md"))]
    return out


# ---- git ----

def git(root: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess:
    done = subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True)
    if check and done.returncode != 0:
        raise SystemExit(f"git {' '.join(args[:2])}: {done.stderr.strip() or done.stdout.strip()}")
    return done


def strict() -> bool:
    """In CI a tag list that cannot be read is an error, never a guess."""
    return os.environ.get("GITHUB_ACTIONS") == "true"


def repo_url() -> str:
    return os.environ.get("OPENRAC_SDK_REPO", REPO)


# The canonical repository's tags by pattern, None once it could not be reached (asked once a run).
_remote: dict[tuple[str, str], list[str] | None] = {}
# Whether the tags came from the local clone because the canonical repository could not be reached.
_fell_back = False


def remote_tags(root: Path, pattern: str) -> list[str] | None:
    """The tags of the canonical repository, or None when it cannot be reached."""
    global _fell_back
    if os.environ.get("OPENRAC_SDK_OFFLINE") == "1":
        return None
    key = (repo_url(), pattern)
    if key not in _remote:
        done = git(root, "ls-remote", "--tags", "--refs", repo_url(), pattern, check=False)
        if done.returncode != 0:
            if strict():
                raise SystemExit(f"cannot list the tags of {repo_url()}: {done.stderr.strip()}")
            print(f"warning: cannot list the tags of {repo_url()}; using the local tags", file=sys.stderr)
            _fell_back = True
            _remote[key] = None
        else:
            _remote[key] = [line.split("\trefs/tags/", 1)[1] for line in done.stdout.splitlines()
                            if "\trefs/tags/" in line]
    return _remote[key]


def all_tags(root: Path, pattern: str) -> list[str]:
    remote = remote_tags(root, pattern)
    if remote is not None:
        return remote
    return git(root, "tag", "--list", pattern).stdout.split()


def have(root: Path, ref: str) -> bool:
    return git(root, "rev-parse", "-q", "--verify", f"{ref}^{{commit}}", check=False).returncode == 0


def fetch_tag(root: Path, tag: str) -> str:
    """A ref to the tag's commit, fetched from the canonical repository when it is not here."""
    ref = f"refs/tags/{tag}"
    if have(root, ref):
        return ref
    shallow = git(root, "rev-parse", "--is-shallow-repository", check=False).stdout.strip() == "true"
    # A full clone takes the tag whole (it shares develop's history); only a shallow one gets depth 1,
    # so a contributor's clone never becomes shallow.
    args = ["fetch", "-q", "--no-tags", *(["--depth=1"] if shallow else []), repo_url(), f"+{ref}:{ref}"]
    done = git(root, *args, check=False)
    if done.returncode != 0 or not have(root, ref):
        raise SystemExit(f"cannot fetch {tag} from {repo_url()}: {done.stderr.strip()}"
                         f" (git fetch {repo_url()} tag {tag})")
    return ref


def show(root: Path, ref: str, path: str) -> str | None:
    done = git(root, "show", f"{ref}:{path}", check=False)
    return done.stdout if done.returncode == 0 else None


# ---- The last release and the open minor ----

@dataclass
class Release:
    tag: str
    version: tuple[int, int, int]
    ref: str


def releases(root: Path, major: int) -> list[tuple[tuple[int, int, int], str]]:
    """The sdk-v<major>.*.* tags, lowest first."""
    out = []
    for tag in all_tags(root, "sdk-v*"):
        m = TAG.match(tag)
        if m and int(m.group(1)) == major:
            out.append(((int(m.group(1)), int(m.group(2)), int(m.group(3))), tag))
    return sorted(out)


def last_release(root: Path, major: int, stable: bool = True) -> Release | None:
    """The highest stable release (patch 0, made for a stable launcher), or with stable=False the
    highest of any kind."""
    tags = [t for t in releases(root, major) if t[0][2] == 0 or not stable]
    if not tags:
        return None
    if _fell_back and stable and tags[-1][0][1] + 1 < header_api(root)[1]:
        # The tree is past the open minor of the last release here: this clone's tags are old.
        raise SystemExit(f"the local tags end at {tags[-1][1]}, older than this tree (interface "
                         f"{major}.{header_api(root)[1]}): fetch them (git fetch --tags {repo_url()}) or "
                         "get online")
    return Release(tags[-1][1], tags[-1][0], fetch_tag(root, tags[-1][1]))


def tag_text(release: Release) -> str:
    return f"{release.tag} is the last release" if release.tag else f"no sdk-v{release.version[0]}.* release yet"


@dataclass
class State:
    release: Release
    open_minor: int
    new_members: list[str]                     # table members the tag does not have, in order
    new_rows: dict[str, list[str]]             # by lock: interface rows (core) the tag does not have
    misplaced: list[str] = field(default_factory=list)  # notes in a released minor's folder
    errors: list[str] = field(default_factory=list)


def state(root: Path) -> State:
    major = header_api(root)[0]
    # No release of this major yet: as if one of minor -1 had nothing, so the open minor is 0.
    release = last_release(root, major) or Release("", (major, -1, 0), "")
    rel_minor = release.version[1]
    core = core_names(root)
    errors: list[str] = []
    new_rows: dict[str, list[str]] = {}
    new_members: list[str] = []
    for lock in API_LOCKS:
        old_text = show(root, release.ref, f"sdk/api/{lock}") if release.ref else ""
        if old_text is None:
            raise SystemExit(f"{release.tag} has no sdk/api/{lock}")
        old = {lock_key(lock, r) for r in lock_rows(old_text)}
        cur_rows = lock_rows(read(root, Path("sdk/api") / lock))
        cur = {lock_key(lock, r) for r in cur_rows}
        gone = sorted(old - cur)
        if gone:
            errors.append(f"sdk/api/{lock} lacks rows {release.tag} has ({', '.join(gone[:3])}"
                          f"{', ...' if len(gone) > 3 else ''}): merge develop into this branch first")
        added = [lock_key(lock, r) for r in cur_rows if lock_key(lock, r) not in old]
        new_rows[lock] = [k for k in added if is_core(lock, k, core)]
        if lock == "openrac_api.lock":
            new_members = added
    # Notes in the folder of a minor that is released already, but not in that release: they belong
    # to the open minor (a branch made before the release).
    # A stable release from before sdk/changes/ (sdk-v1.42.0) has none: the old minors' notes, moved
    # there from the header, are those of its first dev release that has notes; until one does, they
    # are not a branch's.
    released_notes: set[str] = set()
    for version, tag in releases(root, major) if release.ref else []:
        if version[:2] == release.version[:2] and not released_notes:
            released_notes = {p for p in git(root, "ls-tree", "-r", "-z", "--name-only", fetch_tag(root, tag), "--",
                                              CHANGES.as_posix(), check=False).stdout.split("\0") if p.endswith(".md")}
    misplaced: list[str] = []
    for minor, paths in note_folders(root).items():
        if minor <= rel_minor and released_notes:
            misplaced += [p for p in paths if p not in released_notes]
        elif minor > rel_minor + 1:
            errors.append(f"{CHANGES}/{major}.{minor}/: the open minor is {major}.{rel_minor + 1} at most "
                          f"({tag_text(release)})")
    grows = any(new_rows.values()) or bool(notes(root, rel_minor + 1)) or bool(misplaced)
    return State(release, rel_minor + 1 if grows else rel_minor, new_members, new_rows, misplaced, errors)


def member_since(root: Path) -> dict[str, str]:
    path = root / MEMBERS
    if not path.exists():
        return {}
    return {c[0]: c[2] for c in (l.split("\t") for l in path.read_text(encoding="utf-8").splitlines()
                                 if l and not l.startswith("#")) if len(c) >= 3}


def member_comment(header: str, name: str) -> tuple[int, int] | None:
    """The span of the comment just above table member `name` in openrac_api."""
    start = header.find("typedef struct openrac_api {")
    end = header.find("} openrac_api;", start)
    if start < 0 or end < 0:
        return None
    m = re.search(rf"\(\s*\*\s*{re.escape(name)}\s*\)\s*\(", header[start:end])
    if m is None:
        m = re.search(rf"\b{re.escape(name)}\s*;", header[start:end])
    if m is None:
        return None
    at = start + m.start()
    close = header.rfind("*/", start, at)
    semi = header.rfind(";", start, at)
    if close < 0 or close < semi:
        return None
    return header.rfind("/*", start, close), close + 2


def header_since(root: Path, names: list[str]) -> dict[str, str | None]:
    """Each member's "Since <major>.N" in its comment (None for no comment or no Since)."""
    header = read(root, HEADER)
    out: dict[str, str | None] = {}
    for name in names:
        span = member_comment(header, name)
        m = re.findall(r"\bSince (\d+\.\d+)\b", header[span[0]:span[1]]) if span else []
        out[name] = m[-1] if m else None
    return out


# `m_api->api_minor >= N && ...offsetof(openrac_api, member)...;` in mod.hpp.
CPP_CHECK = re.compile(r"api_minor\s*>=\s*(\d+)([^;]*?)(?=;)", re.S)


def cpp_checks(root: Path, names: set[str]) -> list[tuple[int, int, str]]:
    """The `api_minor >= N` of the checks that read a member new since the tag: (start, end, N)."""
    text = read(root, CPP) if (root / CPP).exists() else ""
    out = []
    for m in CPP_CHECK.finditer(text):
        read_members = set(re.findall(r"offsetof\(\s*openrac_api\s*,\s*(\w+)\s*\)", m.group(2)))
        if read_members & names:
            out.append((m.start(1), m.end(1), m.group(1)))
    return out


PORT_STUBS = Path("port/game/common/mod_api")


def stub_since(root: Path, names: set[str]) -> list[tuple[Path, int, int, str, str]]:
    """The "// openrac_api.<name>. Since <major>.N." lines new_api.py writes above a member's port side:
    (file, start, end of N's digits, name, N) for the members in `names`."""
    out = []
    folder = root / PORT_STUBS
    major = header_api(root)[0]
    for path in sorted(folder.glob("*.cpp")) if folder.is_dir() else []:
        text = path.read_text(encoding="utf-8")
        for m in re.finditer(rf"// openrac_api\.(\w+)\. Since {major}\.(\d+)\.", text):
            if m.group(1) in names:
                out.append((path.relative_to(root), m.start(2), m.end(2), m.group(1), m.group(2)))
    return out


def check(root: Path, st: State) -> list[str]:
    """Everything that is not as the open minor wants it."""
    out = list(st.errors)
    want = st.open_minor
    M = st.release.version[0]
    in_lock = {lock_key("openrac_api.lock", r) for r in lock_rows(read(root, Path("sdk/api/openrac_api.lock")))}
    unlocked = [n for n in member_since(root) if n not in in_lock]
    if unlocked:
        out.append(f"sdk/api/openrac_api.lock lacks {', '.join(unlocked)} of {MEMBERS}: "
                   "run python3 sdk/tools/bump_api.py (or gen_api_lock.py)")
    out += [f"{p}: run python3 sdk/tools/bump_api.py" for p in problems(root, want)]
    since = member_since(root)
    for name in st.new_members:
        if since.get(name) != f"{M}.{want}":
            out.append(f"{MEMBERS}: {name} is since {since.get(name)}, but it is new in {M}.{want} "
                       f"({tag_text(st.release)}): run python3 sdk/tools/bump_api.py")
    # The first minor of a major is the whole table: only members after it need their own Since.
    for name, s in header_since(root, st.new_members if st.release.ref else []).items():
        if s != f"{M}.{want}":
            out.append(f"{HEADER}: {name}'s comment says {'Since ' + s if s else 'no Since'}, want Since {M}.{want}"
                       + (": run python3 sdk/tools/bump_api.py" if s else ""))
    for _, _, n in cpp_checks(root, set(st.new_members)):
        if int(n) != want:
            out.append(f"{CPP}: a check of a member new in {M}.{want} reads api_minor >= {n}: "
                       "run python3 sdk/tools/bump_api.py")
    for path, _, _, name, n in stub_since(root, set(st.new_members)):
        if int(n) != want:
            out.append(f"{path}: api_{name} says Since {M}.{n}, want {M}.{want}: run python3 sdk/tools/bump_api.py")
    for p in st.misplaced:
        out.append(f"{p}: {p.split('/')[2]} is released ({st.release.tag}), this note is not in it: "
                   f"run python3 sdk/tools/bump_api.py (moves it to {CHANGES}/{M}.{want}/)")
    # The first minor of a major is the whole table: it needs no note.
    if want > st.release.version[1] and st.release.ref:
        text = " ".join([*notes(root, want).values(),
                         *((root / p).read_text(encoding="utf-8") for p in st.misplaced)])
        if not text.strip():
            out.append(f"{M}.{want} adds to the interface ({describe(st)}) but has no note: "
                       f"python3 sdk/tools/bump_api.py minor --note \"what it adds (names)\"")
        else:
            missing = [n for n in st.new_members if not re.search(rf"\b{re.escape(n)}\b", text)]
            if missing:
                out.append(f"{CHANGES}/{M}.{want}/: no note names {', '.join(missing)}: "
                           "add it to your note (python3 sdk/tools/bump_api.py minor --note ...)")
    for path in notes(root, want):
        if not NOTE_NAME.match(Path(path).stem):
            out.append(f"{path}: a note's name is lower case letters, digits, '.', '-' and '_'")
    return out


def describe(st: State) -> str:
    parts = []
    for lock, rows in st.new_rows.items():
        if rows:
            parts.append(f"{len(rows)} new in {lock}: " + ", ".join(r.replace("\t", ".") for r in rows[:4])
                         + (", ..." if len(rows) > 4 else ""))
    return "; ".join(parts) or "a note"


# ---- Setting it right ----

def set_places(root: Path, minor: int) -> list[str]:
    text = read(root, HEADER)
    new = re.sub(r"^#define OPENRAC_MOD_API_MINOR \d+$", f"#define OPENRAC_MOD_API_MINOR {minor}", text, count=1, flags=re.M)
    if new == text:
        return []
    write(root, HEADER, new)
    return [str(HEADER)]


def fix(root: Path, st: State) -> list[str]:
    """Sets every place, the new members' since, their header Since and mod.hpp checks, and moves
    misplaced notes. Returns what it changed."""
    want = st.open_minor
    M = st.release.version[0]
    changed = []
    folder = root / CHANGES / f"{M}.{want}"
    for p in st.misplaced:
        src = root / p
        dst = folder / src.name
        folder.mkdir(parents=True, exist_ok=True)
        if dst.exists():
            put(dst, dst.read_text(encoding="utf-8").rstrip() + "\n\n" + src.read_text(encoding="utf-8"))
            src.unlink()
        else:
            src.rename(dst)
        if not any(src.parent.iterdir()):
            src.parent.rmdir()
        changed.append(f"{p} -> {dst.relative_to(root).as_posix()}")
    changed += set_places(root, want)
    if (root / MEMBERS).exists() and st.new_members:
        new = set(st.new_members)
        lines = read(root, MEMBERS).split("\n")
        for i, line in enumerate(lines):
            c = line.split("\t")
            if line and not line.startswith("#") and len(c) >= 3 and c[0] in new and c[2] != f"{M}.{want}":
                c[2] = f"{M}.{want}"
                lines[i] = "\t".join(c)
        text = "\n".join(lines)
        if text != read(root, MEMBERS):
            write(root, MEMBERS, text)
            changed.append(str(MEMBERS))
    header = read(root, HEADER)
    out = header
    for name in st.new_members:
        span = member_comment(out, name)
        last = None
        for last in re.finditer(rf"\bSince {M}\.(\d+)\b", out[span[0]:span[1]]) if span else ():
            pass
        if last is not None and int(last.group(1)) != want:
            # Only the last "Since" (the member's own; an earlier one may name another item's).
            at = span[0] + last.start(1)
            out = out[:at] + str(want) + out[span[0] + last.end(1):]
    if out != header:
        write(root, HEADER, out)
        changed.append(f"{HEADER} (Since of {len(st.new_members)} new members)")
    spots = [s for s in cpp_checks(root, set(st.new_members)) if int(s[2]) != want]
    if spots:
        text = read(root, CPP)
        for start, end, _ in sorted(spots, reverse=True):
            text = text[:start] + str(want) + text[end:]
        write(root, CPP, text)
        changed.append(f"{CPP} ({len(spots)} api_minor checks)")
    by_file: dict[Path, list[tuple[int, int]]] = {}
    for path, start, end, _, n in stub_since(root, set(st.new_members)):
        if int(n) != want:
            by_file.setdefault(path, []).append((start, end))
    for path, spots in by_file.items():
        text = read(root, path)
        for start, end in sorted(spots, reverse=True):
            text = text[:start] + str(want) + text[end:]
        write(root, path, text)
        changed.append(f"{path} (Since of {len(spots)} port calls)")
    return changed


def stale_lines(root: Path, st: State) -> list[str]:
    """Lines added since the tag that name the minor the tag closed, when this tree is in the next:
    a "Since 1.M" or `api_minor >= M` there is most likely this branch's own, and wrong."""
    closed = st.release.version[1]
    M = st.release.version[0]
    if st.open_minor == closed or not st.release.ref:
        return []
    diff = git(root, "diff", "-U0", "--no-color", st.release.ref, "--", "sdk", "port/game/common",
               "launcher/core/src", check=False).stdout
    out, path, line_no = [], "", 0
    pattern = re.compile(rf"\b{M}\.{closed}\b(?!\.\d)|api_minor\s*(>=|<|>|<=|==)\s*{closed}\b")
    for line in diff.splitlines():
        if line.startswith("+++ "):
            path = line[6:] if line.startswith("+++ b/") else ""
        elif line.startswith("@@"):
            m = re.search(r"\+(\d+)", line)
            line_no = int(m.group(1)) if m else 0
        elif line.startswith("+") and path:
            if pattern.search(line) and not path.startswith(("sdk/docs/reference/", "sdk/api/")):
                out.append(f"{path}:{line_no}: {line[1:].strip()[:120]}")
            line_no += 1
    return out


def add_note(root: Path, minor: int, note: str, name: str) -> Path:
    M = header_api(root)[0]
    if not NOTE_NAME.match(name):
        raise SystemExit(f"--name {name!r}: lower case letters, digits, '.', '-' and '_'")
    text = " ".join(note.split())
    if not text:
        raise SystemExit("--note is empty")
    if not text.endswith("."):
        text += "."
    text = text[0].upper() + text[1:]
    path = root / CHANGES / f"{M}.{minor}" / f"{name}.md"
    path.parent.mkdir(parents=True, exist_ok=True)
    old = path.read_text(encoding="utf-8").strip() if path.exists() else ""
    put(path, (old + "\n\n" if old else "") + text + "\n")
    return path


def branch_name(root: Path) -> str | None:
    done = git(root, "rev-parse", "--abbrev-ref", "HEAD", check=False)
    name = done.stdout.strip()
    if done.returncode != 0 or name in ("", "HEAD", "develop", "master", "main"):
        return None
    name = re.sub(r"[^a-z0-9._-]+", "-", name.rsplit("/", 1)[-1].lower()).strip("-.")
    return name or None


def archived(path: str) -> bool:
    """Whether a file of the tree (a path from its root) goes into the SDK archive (package_sdk.py)."""
    sdk = package_sdk.SDK.relative_to(package_sdk.ROOT).as_posix() + "/"
    extra = {p.relative_to(package_sdk.ROOT).as_posix() for p in package_sdk.EXTRA.values()}
    if path in extra:
        return True
    if not path.startswith(sdk):
        return False
    rel = path[len(sdk):]
    if package_sdk.SKIP.intersection(rel.split("/")):
        return False
    if re.fullmatch(r"bindings/[^/]+\.ids\.lock", rel):
        return True
    return any(rel == part or rel.startswith(part + "/") for part in package_sdk.PARTS)


def release_version(root: Path, st: State, stable: bool = False) -> str | None:
    """The version a release of this tree gets, None when there is nothing to release.

    The first release of a major is <major>.0.0. A stable one (a stable launcher's) is the open
    minor's <major>.<open>.0 when the interface grew since the last stable, else none. A dev one is
    the next patch of the last stable minor when anything in the archive changed since the last
    release of either kind; its headers already say the open minor."""
    major, minor = st.release.version[0], st.release.version[1]
    latest = last_release(root, major, stable=False)
    if latest is None:
        return f"{major}.0.0"
    if not st.release.ref:
        raise SystemExit(f"{latest.tag} is released but no sdk-v{major}.*.0: the first release is {major}.0.0")
    if stable:
        return f"{major}.{st.open_minor}.0" if st.open_minor > minor else None
    changed = git(root, "diff", "--name-only", "-z", latest.ref, "--", "sdk", "docs/port").stdout.split("\0")
    if not any(archived(p) for p in changed if p):
        return None
    patch = max(v[2] for v, _ in releases(root, major) if v[1] == minor) + 1
    return f"{major}.{minor}.{patch}"


def versions(root: Path) -> str:
    """Each release's interface: SDK tags and launcher tags, a Markdown table."""
    def minor_at(ref: str) -> str:
        text = show(root, ref, HEADER.as_posix()) or ""
        a = re.search(r"^#define OPENRAC_MOD_API_MAJOR (\d+)$", text, re.M)
        b = re.search(r"^#define OPENRAC_MOD_API_MINOR (\d+)$", text, re.M)
        return f"{a.group(1)}.{b.group(1)}" if a and b else "-"

    def sort(tags: list[str], pattern: re.Pattern) -> list[str]:
        return sorted((t for t in tags if pattern.match(t)), key=lambda t: tuple(map(int, pattern.match(t).groups())))

    launchers = [(t, minor_at(fetch_tag(root, t))) for t in sort(all_tags(root, "launcher-v*"), LAUNCHER_TAG)]
    out = ["| SDK | Interface | First stable launcher with it |", "| --- | --- | --- |"]
    for tag in reversed(sort(all_tags(root, "sdk-v*"), TAG)):
        m = TAG.match(tag)
        api = (int(m.group(1)), int(m.group(2)))
        if int(m.group(3)) > 0:
            # A dev release's headers may already have the open minor: what they say counts.
            at = minor_at(fetch_tag(root, tag))
            api = tuple(map(int, at.split("."))) if at != "-" else api
        first = next((t for t, a in launchers if a != "-" and tuple(map(int, a.split("."))) >= api), None)
        out.append(f"| {tag[5:]} | {api[0]}.{api[1]} | {first[10:] if first else 'none yet (Development channel)'} |")
    return "\n".join(out) + "\n"


def write_locks(root: Path) -> None:
    """gen_api_lock.py's locks, written before members.tsv agrees with the header (which is what this
    tool sets next); the append-only rule holds as there."""
    import gen_api_lock
    folder = root / "sdk" / "api"
    rows = gen_api_lock.generate(root / "sdk")
    broken = [p for name, r in rows.items() for p in gen_api_lock.compare(name, gen_api_lock.read_lock(folder / name), r)]
    if broken:
        raise SystemExit("\n".join(f"error: {p}" for p in broken))
    for name, r in rows.items():
        text = gen_api_lock.HEADER + "".join(x + "\n" for x in r)
        if read(root, Path("sdk/api") / name) != text:
            write(root, Path("sdk/api") / name, text)
            print(f"wrote sdk/api/{name}")


def regen(root: Path, *tools: str) -> None:
    for tool in tools:
        subprocess.run([sys.executable, str(root / "sdk" / "tools" / tool)], check=True)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("kind", nargs="?", choices=["minor", "patch"],
                    help="minor: add --note for the open minor; patch: nothing to do (the release picks it)")
    ap.add_argument("--note", help="for a minor: what it adds, naming the new members")
    ap.add_argument("--name", help="the note's file name (default: the git branch's last part)")
    ap.add_argument("--check", action="store_true", help="write nothing; fail when something is off")
    ap.add_argument("--release-version", action="store_true", help="print the version a release gets ('' for none)")
    ap.add_argument("--stable", action="store_true", help="with --release-version: a stable launcher's release")
    ap.add_argument("--versions", action="store_true", help="print each release's interface")
    ap.add_argument("--no-regen", action="store_true", help="do not run gen_api_lock.py and gen_reference.py")
    ap.add_argument("--root", type=Path, default=ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    root = args.root
    if args.versions:
        print(versions(root), end="")
        return 0
    if args.kind == "patch":
        if args.note:
            raise SystemExit("--note is for a minor (the changelog lists minors only)")
        print("Nothing to do: a release takes the next patch by itself when the SDK changed.")
        return 0
    if args.kind == "minor" and not args.note:
        raise SystemExit("a minor needs --note: what it adds, naming the new members")
    if args.note and args.kind != "minor":
        raise SystemExit("--note goes with minor")
    if not (args.check or args.release_version or args.no_regen):
        # The locks first: what the headers have now (new_api.py does not write them).
        write_locks(root)
    st = state(root)
    M = st.release.version[0]
    if args.check or args.release_version:
        found_problems = check(root, st)
        for p in found_problems:
            print(f"error: {p}", file=sys.stderr)
        if found_problems:
            return 1
        if args.release_version:
            print(release_version(root, st, args.stable) or "")
        else:
            print(f"SDK interface {M}.{st.open_minor} in every place ({tag_text(st.release)})")
        return 0
    if st.errors:
        for e in st.errors:
            print(f"error: {e}", file=sys.stderr)
        return 1
    if args.kind == "minor":
        name = args.name or branch_name(root)
        if not name:
            raise SystemExit("--name NAME: the note's file name (no branch name to take it from)")
        path = add_note(root, st.release.version[1] + 1, args.note, name)
        print(f"wrote {path.relative_to(root).as_posix()}")
        st = state(root)
    changed = fix(root, st)
    for c in changed:
        print(f"changed {c}")
    if not args.no_regen:
        regen(root, "gen_api_lock.py", "gen_reference.py")
    for line in stale_lines(root, st):
        print(f"look at: {line}  ({M}.{st.release.version[1]} is released: new things are {M}.{st.open_minor})")
    st = state(root)
    print(f"SDK interface {M}.{st.open_minor} ({tag_text(st.release)})")
    left = check(root, st)
    for p in left:
        print(f"still to do: {p}", file=sys.stderr)
    return 1 if left else 0


if __name__ == "__main__":
    sys.exit(main())
