#!/usr/bin/env python3
"""
The decomps under games/ and OpenRAC's patches to them (docs/SOURCES.md).

Each decomp is a git submodule, pinned to the commit its game.json records:

  games/<game>/decomp/            the game's own decomp, which holds every version of the game
                                  (game.json "decomp"; each version built from it says "shared": "decomp")
  games/rac1/ntsc-decomp/         a third-party decomp of one version (that version's "dir" and "source")

Nothing is committed inside a decomp. OpenRAC's changes to it sit beside it, in <folder>-patches/
(games/<game>/decomp-patches/, games/rac1/ntsc-decomp-patches/):

  NNNN-<what>.patch      one change each, its reason in its message (git format-patch), applied in order
  files/<path>           OpenRAC's own files, copied to <path> in the decomp
  README.md              what the folder holds; its list of patches is written by `save`

A decomp is named by its folder under games/ (rac1/decomp, rac1/ntsc-decomp) or by any version
built from it (rac1/pal is rac1/decomp, rac1/ntsc is rac1/ntsc-decomp):

  python3 tools/sources.py init                  check out every decomp at its pin and apply its patches (after a clone or a pull)
  python3 tools/sources.py apply [DECOMP...]     apply the patches
  python3 tools/sources.py unapply [DECOMP...]   take them out again: the decomp as its repository has it
  python3 tools/sources.py edit DECOMP           turn the patches into commits in the submodule
  python3 tools/sources.py save DECOMP           write those commits back as the patches
  python3 tools/sources.py update DECOMP [REV]   move the pin to REV (default: the tip of its branch), patches rebased
  python3 tools/sources.py status                where each decomp is, and its patches
  python3 tools/sources.py check [--since REV]   every pin matches game.json; a pin moved since REV
                                                 came with the port's tables regenerated (CI)

To change a patch: `edit`, then work with git in the submodule (commit, `git commit --amend`,
`git rebase -i`: each commit is one patch, its message the patch's reason), then `save`. A change
to the decomp itself (a match, a name) goes to its own repository instead (docs/SOURCES.md).
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
    """One decomp: its key (its folder under games/: rac1/decomp, rac1/ntsc-decomp), game
    manifest, submodule folder and patches folder."""

    def __init__(self, root: Path, game: dict, version: str):
        self.root, self.game, self.version = root, game, version
        if version == games.DECOMP:
            self.rel = games.decomp_dir(game)
            self.source = game[games.DECOMP]
        else:
            self.rel = games.version_dir(game, version)
            self.source = game["versions"][version]["source"]
        self.key = Path(self.rel).relative_to("games").as_posix()
        self.dir = root / self.rel
        self.patches = root / f"{self.rel}-patches"

    def sharing(self) -> list[dict]:
        """The "source" of every version built from this tree (game.json "shared": its name)."""
        return [v["source"] for v in self.game["versions"].values()
                if (v.get("source") or {}).get("shared") == self.version]

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


def trees(root: Path | None = None) -> dict[str, Tree]:
    """Every decomp by its key, its folder under games/: '<game>/decomp' for each game's own decomp,
    and the folder of each version with a decomp of its own ('rac1/ntsc-decomp'), not one "shared"
    with the game's (every version of rac2, built from rac2/decomp)."""
    root = root or ROOT
    out = {}
    for game in games.load_all(root):
        found = [Tree(root, game, games.DECOMP)] if game.get(games.DECOMP) else []
        found += [Tree(root, game, v) for v, info in game["versions"].items()
                  if info.get("source") and not info["source"].get("shared")]
        out.update({tree.key: tree for tree in found})
    return out


def need(tree: Tree) -> None:
    if not tree.checked_out():
        sys.exit(f"{tree.rel} is not checked out: run `git submodule update --init --depth 1 -- {tree.rel}`")


def in_edit(tree: Tree) -> bool:
    return bool((tree.state() or {}).get("edit"))


def do_unapply(tree: Tree) -> None:
    """Put back the decomp's own files where apply changed them, and remove the ones it added."""
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


def summary(patch: Path) -> str:
    """The first sentence of the patch's message, else its subject."""
    text = patch.read_text(encoding="utf-8", errors="replace")
    found = re.search(r"^Subject: (?:\[PATCH[^\]]*\] )?([^\n]+(?:\n [^\n]+)*)\n(?:[\w-]+: [^\n]*\n)*\n(.*?)\n---\n", text, re.M | re.S)
    if not found:
        return patch.stem
    body = " ".join(found.group(2).split())
    return body.split(". ")[0].rstrip(".") + "." if body else " ".join(found.group(1).split())


def short_name(patch: Path) -> str:
    """NNNN-<subject in lower case>.patch: short, readable names whatever case the subject has."""
    number, _, rest = patch.stem.partition("-")
    return f"{number}-{re.sub(r'[^a-z0-9]+', '-', rest.lower()).strip('-')}.patch"


def write_readme(tree: Tree) -> None:
    """Rewrite the list of patches and files between the markers of the folder's README.md."""
    rows = [f"| [{p.name}]({p.name}) | {summary(p)} |" for p in tree.patch_list()]
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
    gitdir = Path(tree.git("rev-parse", "--absolute-git-dir"))
    if (gitdir / "rebase-apply").exists() or (gitdir / "rebase-merge").exists():
        sys.exit(f"{tree.rel} is in the middle of git am or a rebase: finish it (--continue) first, then save")
    if tree.run("status", "--porcelain", "--untracked-files=no").stdout.strip():
        sys.exit(f"{tree.rel} has changes not committed: commit them (each commit is one patch) and save again")
    tree.patches.mkdir(parents=True, exist_ok=True)
    for old in tree.patch_list():
        old.unlink()
    out = tree.run("format-patch", "-q", "--zero-commit", "--no-signature", "--no-numbered", "--keep-subject",
                   "-o", str(tree.patches), f"{base}..HEAD")
    if out.returncode:
        sys.exit(out.stderr)
    for patch in tree.patch_list():
        patch.rename(patch.with_name(short_name(patch)))
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
    """Write COMMIT into game.json for the tree and for the versions built from it."""
    source = tree.source
    source["commit"] = commit
    source["date"] = tree.git("log", "-1", "--format=%cs", commit)
    source["imported"] = time.strftime("%Y-%m-%d")
    for shared in tree.sharing():
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
    print(f"{tree.key}: {old[:10]}..{new[:10]}; check files/ against the decomp's changes, then commit "
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


def do_init() -> None:
    """Check out every submodule at its pin (LF endings, shallow), then apply every decomp's patches."""
    for tree in trees().values():
        if tree.checked_out():
            do_unapply(tree)
    proc.git(ROOT, "-c", "core.autocrlf=false", "submodule", "update", "--init", "--depth", "1", "--jobs", "8")
    for tree in trees().values():
        do_apply(tree)


def changed_since(rev: str) -> list[str]:
    return proc.git(ROOT, "diff", "--name-only", rev, "HEAD").splitlines()


def pins_at(rev: str, game: str) -> set[str]:
    """The commits REV pins under games/<game>/: a decomp that only moved to another folder (as
    games/<game>/<version> became games/<game>/decomp) is still at one of them."""
    listing = proc.git(ROOT, "ls-tree", "-r", rev, "--", f"games/{game}/", check=False)
    return {cols[2] for cols in (line.split() for line in listing.splitlines()) if cols and cols[0] == "160000"}


def do_check(since: str | None) -> None:
    """The index's pins and game.json agree; and, given REV, a decomp whose pin moved since REV
    comes with its port tables (port/game/<game>*) changed in the same range, since hostgen's tables,
    release names and asm interfaces are made from one commit of the decomp (a game the port does
    not build yet, with no port/game/<game>*/hostgen.json, has none)."""
    problems = []
    for key, tree in trees().items():
        pinned, recorded = tree.pinned(), tree.source["commit"]
        if pinned != recorded:
            problems.append(f"{key}: game.json records {recorded[:10]} but the submodule is pinned to "
                            f"{pinned[:10] if pinned else 'nothing'} (tools/sources.py update {key})")
        for shared in tree.sharing():
            if shared.get("commit") != recorded:
                problems.append(f"{key}: a version built from it records {str(shared.get('commit'))[:10]}, "
                                f"not the tree's {recorded[:10]} (game.json)")
    if since:
        changed = changed_since(since)
        for key, tree in trees().items():
            if tree.rel not in changed or tree.pinned() in pins_at(since, tree.game["id"]):
                continue
            if not any((ROOT / "port/game").glob(f"{tree.game['id']}*/hostgen.json")):
                continue  # the port does not build this game yet: it has no tables to keep in step
            ported = [p for p in changed if p.startswith(f"port/game/{tree.game['id']}")]
            if not ported:
                problems.append(f"{key}: the submodule moved since {since} but nothing under "
                                f"port/game/{tree.game['id']}* changed: regenerate the port's tables "
                                f"(hostgen, release tables, asm interfaces, Windows exports) for the new commit")
    if problems:
        sys.exit("\n".join(problems))
    print("sources: every pin matches game.json" + (f"; pins moved since {since} came with port changes" if since else ""))


def chosen(keys: list[str]) -> list[Tree]:
    known = trees()
    # A version names the decomp it is built from: rac1/pal is rac1/decomp, rac1/ntsc rac1/ntsc-decomp.
    for game in games.load_all(ROOT):
        for v, info in game["versions"].items():
            source = info.get("source") or {}
            if not source or f"{game['id']}/{v}" in known:
                continue
            rel = games.decomp_dir(game) if source.get("shared") == games.DECOMP else games.version_dir(game, v)
            tree = known.get(Path(rel).relative_to("games").as_posix())
            if tree:
                known[f"{game['id']}/{v}"] = tree
    for key in keys:
        if key not in known:
            sys.exit(f"unknown decomp {key}: one of {', '.join(known)}")
    picked = [known[k] for k in keys] if keys else list(trees().values())
    return list({id(t): t for t in picked}.values())


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
    sub.add_parser("init")
    sub.add_parser("check").add_argument("--since")
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
    elif args.cmd == "init":
        do_init()
    elif args.cmd == "check":
        do_check(args.since)
    else:
        tree = chosen([args.version])[0]
        {"edit": do_edit, "save": do_save}.get(args.cmd, lambda t: do_update(t, args.rev))(tree)


if __name__ == "__main__":
    main()
