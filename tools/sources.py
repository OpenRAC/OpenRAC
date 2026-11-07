#!/usr/bin/env python3
"""
The decompilation projects under games/ and OpenRAC's changes to them (docs/SOURCES.md).

Each project is a git submodule at games/<game>/<version>, pinned to the commit its game.json
"source" records. Nothing is committed inside it: OpenRAC's changes live beside it, in
games/<game>/<version>-patches/:

  NNNN-<subject>.patch   one change each, with its reason (git format-patch), applied in order
  files/<path>           OpenRAC's own files, copied to <path> in the project
  README.md              what the folder holds; its list of patches is written by `save`

  python3 tools/sources.py apply [GAME/VERSION...]    put the changes in place (after a clone or a pull)
  python3 tools/sources.py unapply [GAME/VERSION...]  take them out: the project as it is upstream
  python3 tools/sources.py edit GAME/VERSION          turn the patches into commits in the submodule
  python3 tools/sources.py save GAME/VERSION          write those commits back as the patches
  python3 tools/sources.py update GAME/VERSION [REV]  move to REV (default: its branch's tip), patches rebased
  python3 tools/sources.py status                     where each project is, and its patches

A first clone takes `git submodule update --init --depth 1`, then `apply`. To change a patch:
`edit`, then work with git in the submodule (commit, `git commit --amend`, `git rebase -i`: each
commit is one patch, its message the patch's reason), then `save`. A change to the project itself
(a match, a name) goes to its own repository instead (docs/SOURCES.md).
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

from openrac_common import games, paths, proc

ROOT = paths.repo_root()
STATE = "openrac-patches.json"  # in the submodule's git folder: what apply changed, or the edit's base
BRANCH = "openrac-patches"      # the submodule's branch while its patches are commits (edit)
BEGIN, END = "<!-- patches:begin -->", "<!-- patches:end -->"


class Tree:
    """One project: its version key, game manifest, submodule folder and patches folder."""

    def __init__(self, root: Path, game: dict, version: str):
        self.root, self.game, self.version = root, game, version
        self.key = f"{game['id']}/{version}"
        self.rel = games.version_dir(game, version)
        self.dir = root / self.rel
        self.patches = root / f"{self.rel}-patches"
        self.source = game["versions"][version]["source"]

    def git(self, *args, check: bool = True):
        return proc.git(self.dir, *args, check=check)

    def run(self, *args) -> subprocess.CompletedProcess:
        return subprocess.run(["git", "-C", str(self.dir), *args], capture_output=True, text=True)

    def checked_out(self) -> bool:
        return (self.dir / ".git").exists()

    def state_path(self) -> Path:
        path = Path(self.git("rev-parse", "--git-dir"))
        return (path if path.is_absolute() else self.dir / path) / STATE

    def state(self) -> dict | None:
        path = self.state_path()
        return json.loads(path.read_text(encoding="utf-8")) if path.exists() else None

    def write_state(self, state: dict | None) -> None:
        path = self.state_path()
        if state is None:
            path.unlink(missing_ok=True)
        else:
            path.write_text(json.dumps(state, indent=1) + "\n", encoding="utf-8")

    def pinned(self) -> str | None:
        """The commit OpenRAC's index pins the submodule to (None if it is not a submodule)."""
        parts = proc.git(self.root, "ls-files", "-s", "--", self.rel, check=False).split()
        return parts[1] if parts and parts[0] == "160000" else None

    def patch_list(self) -> list[Path]:
        return sorted(self.patches.glob("[0-9][0-9][0-9][0-9]-*.patch"))

    def file_list(self) -> list[str]:
        top = self.patches / "files"
        return sorted(p.relative_to(top).as_posix() for p in top.rglob("*") if p.is_file()) if top.is_dir() else []


def trees(root: Path = ROOT) -> dict[str, Tree]:
    """'game/version' -> Tree for every version with its own source (not one "shared" with another
    version's tree, such as rac2's PAL and v2.00, built from rac2/ntsc)."""
    return {f"{g['id']}/{v}": Tree(root, g, v) for g, v, info in games.versions(root)
            if info.get("source") and not info["source"].get("shared")}


def need(tree: Tree) -> None:
    if not tree.checked_out():
        sys.exit(f"{tree.rel} is not checked out: run `git submodule update --init --depth 1 -- {tree.rel}`")


def in_edit(tree: Tree) -> bool:
    return bool((tree.state() or {}).get("edit"))


def do_unapply(tree: Tree) -> None:
    """Put back the project's own files where apply changed them, and remove the ones it added."""
    state = tree.state()
    if state is None or state.get("edit"):
        return
    files = state["files"]
    ours = [rel for rel in files if tree.git("ls-files", "--", rel, check=False)]
    if ours:
        tree.git("checkout", "--", *ours)
    for rel in files:
        if rel not in ours:
            (tree.dir / rel).unlink(missing_ok=True)
    tree.write_state(None)


def do_apply(tree: Tree) -> None:
    """Apply TREE's patches, then copy its files, on the commit checked out."""
    need(tree)
    if in_edit(tree):
        sys.exit(f"{tree.key} is being edited: finish with `tools/sources.py save {tree.key}`")
    do_unapply(tree)
    if tree.git("status", "--porcelain", "--untracked-files=no"):
        sys.exit(f"{tree.rel} has changes of its own: send them to {tree.source['repo']}, "
                 f"or make them a patch (tools/sources.py edit {tree.key})")
    touched = []
    for patch in tree.patch_list():
        listed = tree.run("apply", "--numstat", str(patch))
        done = tree.run("apply", "--whitespace=nowarn", str(patch))
        if done.returncode:
            tree.write_state({"files": touched})
            sys.exit(f"{tree.key}: {patch.name} does not apply to {tree.git('rev-parse', '--short', 'HEAD')}:\n"
                     f"{done.stderr.strip()}\nIs the submodule at the commit game.json records "
                     f"(git submodule update)? After an update, `tools/sources.py edit {tree.key}` rebases them.")
        touched += [line.split("\t", 2)[2] for line in listed.stdout.splitlines()]
    for rel in tree.file_list():
        dest = tree.dir / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(tree.patches / "files" / rel, dest)
        touched.append(rel)
    tree.write_state({"files": sorted(set(touched))})
    print(f"{tree.key}: {len(tree.patch_list())} patches and {len(tree.file_list())} files applied")


def am(tree: Tree, base: str) -> None:
    """Check out BASE on the edit branch with the patches as commits on top."""
    tree.git("checkout", "-q", "-B", BRANCH, base)
    if tree.patch_list():
        done = tree.run("am", "-q", "--keep-cr", "--3way", *map(str, tree.patch_list()))
        if done.returncode:
            sys.exit(f"{tree.key}: the patches stop at a conflict:\n{done.stdout}{done.stderr}\n"
                     f"Resolve it in {tree.rel} and `git am --continue`, then `tools/sources.py save {tree.key}`.")


def do_edit(tree: Tree) -> None:
    need(tree)
    if in_edit(tree):
        sys.exit(f"{tree.key} is already being edited (branch {BRANCH} in {tree.rel})")
    do_unapply(tree)
    base = tree.git("rev-parse", "HEAD")
    tree.write_state({"edit": base})
    am(tree, base)
    print(f"{tree.key}: the patches are commits on {BRANCH} in {tree.rel}, on {base[:10]}. Change them with git "
          f"(one commit per patch, its message the reason), then `tools/sources.py save {tree.key}`.")


def subject(patch: Path) -> str:
    text = patch.read_text(encoding="utf-8", errors="replace")
    found = re.search(r"^Subject: (?:\[PATCH[^\]]*\] )?(.+(?:\n [^\n]+)*)", text, re.M)
    return " ".join(found.group(1).split()) if found else patch.stem


def write_readme(tree: Tree) -> None:
    """Rewrite the list of patches and files between the markers of the folder's README.md."""
    rows = [f"| [{p.name}]({p.name}) | {subject(p)} |" for p in tree.patch_list()]
    block = [BEGIN, "<!-- Written by tools/sources.py save; do not edit. -->", "| Patch | What |", "|---|---|", *rows]
    if tree.file_list():
        block += ["", "Files under [files/](files) (copied as they are):", ""]
        block += [f"- [`{rel}`](files/{rel})" for rel in tree.file_list()]
    block.append(END)
    readme = tree.patches / "README.md"
    text = readme.read_text(encoding="utf-8") if readme.exists() else (
        f"# OpenRAC's changes to {tree.source['name']}\n\n"
        f"Applied to [{tree.source['name']}]({tree.source['repo']}) (`{tree.rel}`) by "
        f"`python3 tools/sources.py apply`; how to change them: [docs/SOURCES.md]"
        f"({os.path.relpath(tree.root / 'docs/SOURCES.md', tree.patches).replace(os.sep, '/')}).\n\n"
        f"{BEGIN}\n{END}\n")
    head, rest = text.split(BEGIN, 1)
    readme.write_text(head + "\n".join(block) + rest.split(END, 1)[1], encoding="utf-8")


def do_save(tree: Tree) -> None:
    need(tree)
    state = tree.state() or {}
    base = state.get("edit")
    if not base:
        sys.exit(f"{tree.key} is not being edited: `tools/sources.py edit {tree.key}` first")
    if tree.run("status", "--porcelain", "--untracked-files=no").stdout.strip():
        sys.exit(f"{tree.rel} has changes not committed: commit them (each commit is one patch) and save again")
    tree.patches.mkdir(parents=True, exist_ok=True)
    for old in tree.patch_list():
        old.unlink()
    out = tree.run("format-patch", "-q", "--zero-commit", "--no-signature", "--no-numbered", "--keep-subject",
                   "-o", str(tree.patches), f"{base}..HEAD")
    if out.returncode:
        sys.exit(out.stderr)
    write_readme(tree)
    tree.git("checkout", "-q", "--detach", base)
    tree.git("branch", "-q", "-D", BRANCH)
    tree.write_state(None)
    if base != tree.source["commit"]:
        record(tree, base)
    do_apply(tree)
    print(f"{tree.key}: commit {tree.patches.relative_to(ROOT)}/"
          + (f", {tree.rel} and its game.json" if base != (tree.pinned() or "") else ""))


def record(tree: Tree, commit: str) -> None:
    """Write COMMIT into game.json for the tree's version and for the versions sharing its tree."""
    source = tree.source
    source["commit"] = commit
    source["date"] = tree.git("log", "-1", "--format=%cs", commit)
    source["imported"] = time.strftime("%Y-%m-%d")
    for other in tree.game["versions"].values():
        shared = other.get("source") or {}
        if shared.get("shared") == tree.version:
            shared.update({k: source[k] for k in ("commit", "date", "imported")})
    path = games.manifest_path(tree.game["id"], tree.root)
    path.write_text(json.dumps(tree.game, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def do_update(tree: Tree, rev: str | None) -> None:
    need(tree)
    if in_edit(tree):
        sys.exit(f"{tree.key} is being edited: save it first")
    do_unapply(tree)
    old = tree.git("rev-parse", "HEAD")
    tree.git("fetch", "-q", "--depth", "50", "origin", rev or tree.source["branch"])
    new = tree.git("rev-parse", "FETCH_HEAD")
    tree.write_state({"edit": old})
    am(tree, old)
    tree.write_state({"edit": new})
    done = tree.run("rebase", "-q", "--onto", new, old)
    if done.returncode:
        sys.exit(f"{tree.key}: a patch conflicts with {new[:10]}:\n{done.stdout}{done.stderr}\n"
                 f"Resolve it in {tree.rel} and `git rebase --continue`, then `tools/sources.py save {tree.key}`.")
    do_save(tree)
    print(f"{tree.key}: {old[:10]}..{new[:10]}; check files/ against the project's changes, then commit "
          f"{tree.rel}, {tree.rel}-patches/ and games/{tree.game['id']}/game.json as chore(import)")


def do_status() -> None:
    for key, tree in trees().items():
        pinned, recorded = tree.pinned(), tree.source["commit"]
        line = f"{key}: {tree.source['name']} at {recorded[:10]} ({tree.source['date']})"
        if pinned != recorded:
            line += f"; the submodule is pinned to {pinned[:10] if pinned else 'nothing'}"
        if not tree.checked_out():
            print(line + "; not checked out")
            continue
        state = tree.state() or {}
        head = tree.git("rev-parse", "HEAD")
        if state.get("edit"):
            line += f"; being edited on {BRANCH} (base {state['edit'][:10]})"
        elif head != pinned:
            line += f"; checked out at {head[:10]}"
        line += (f"; {len(tree.patch_list())} patches, {len(tree.file_list())} files, "
                 f"{'applied' if state.get('files') is not None else 'not applied'}")
        print(line)


def chosen(keys: list[str]) -> list[Tree]:
    known = trees()
    for key in keys:
        if key not in known:
            sys.exit(f"unknown version {key}: one of {', '.join(known)}")
    return [known[k] for k in keys] if keys else list(known.values())


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("apply", "unapply"):
        sub.add_parser(name).add_argument("versions", nargs="*")
    for name in ("edit", "save"):
        sub.add_parser(name).add_argument("version")
    up = sub.add_parser("update")
    up.add_argument("version")
    up.add_argument("rev", nargs="?")
    sub.add_parser("status")
    args = ap.parse_args()
    if args.cmd == "apply":
        for t in chosen(args.versions):
            do_apply(t)
    elif args.cmd == "unapply":
        for t in chosen(args.versions):
            do_unapply(t)
            print(f"{t.key}: as it is upstream")
    elif args.cmd == "status":
        do_status()
    else:
        tree = chosen([args.version])[0]
        {"edit": do_edit, "save": do_save}.get(args.cmd, lambda t: do_update(t, args.rev))(tree)


if __name__ == "__main__":
    main()
