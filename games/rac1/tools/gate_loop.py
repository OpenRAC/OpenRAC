#!/usr/bin/env python3
"""
Run the gate on a batch of changes, and take out any file that breaks a match.

  python3 tools/gate_loop.py

It runs tools/check_all.sh. If a function stops matching, in any version, it reverts that function's source file and
runs the gate again. Exit 0 means the whole gate passed. Anything else means stop and read the output: do not commit.

Every revert is listed, because the batch is then smaller than you planned. Only progress/ and config/versions/ are reset
between rounds: config/ratchets.json and other work in config/ stay as you left them.
"""
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOG = ROOT / "build-sn/gate_loop.log"
MAX_ROUNDS = 6


def sh(command):
    return subprocess.run(command, shell=True, cwd=ROOT, capture_output=True, text=True)


def changed_files():
    return set(sh("git diff --name-only -- src").stdout.split())


def shared_file(path):
    """src/versions/<K>/<stem>/func_X.c -> src/<stem>.c"""
    m = re.match(r"src/versions/[^/]+/(.+)/[^/]+\.c$", path)
    return f"src/{m.group(1)}.c" if m else path


def report_functions(text):
    return {f["name"]: f.get("fuzzy_match_percent") for u in json.loads(text)["units"] for f in u.get("functions", [])}


def lost_functions(old_text, new_text):
    """Functions whose match number changed in the report."""
    old, new = report_functions(old_text), report_functions(new_text)
    return [name for name, percent in old.items() if percent != new.get(name)]


def source_of(name, map_text):
    for line in map_text.splitlines():
        columns = line.split("\t")
        if columns[0] == name:
            return shared_file(columns[1])
    return None


def map_status(text):
    rows = (line.split("\t") for line in text.splitlines() if line and not line.startswith("#"))
    return {row[0]: (row[1], row[2]) for row in rows if len(row) > 2}


def version_losses(old_text, new_text):
    """Source paths of functions that stopped matching in this version: a fragment, or a shared file."""
    old, new = map_status(old_text), map_status(new_text)
    return {path for name, (path, status) in old.items()
            if status == "match" and new.get(name, ("", ""))[1] != "match"}


def compile_failures(log):
    """Files named by a compile error, in a shared path or a version's spliced copy."""
    files = {f"src/{m}.c" for m in re.findall(r"build-sn/versions/[^/]+/src/src/(.+?)/src\.c", log)}
    files |= set(re.findall(r"^(src/[\w/.-]+\.c):\d+: (?!warning)", log, re.M))
    files |= {f"src/overlays/{m}.c" for m in re.findall(r"build-sn/overlays/report/(.+?)/src\.c", log)}
    return files


def files_to_blame(log):
    bad = set()
    report = ROOT / "progress/report.json"
    if (ROOT / "build-sn/rac1.elf").exists() and report.exists():
        old_map = sh("git show HEAD:config/versions/us/map.tsv").stdout
        for name in lost_functions(sh("git show HEAD:progress/report.json").stdout, report.read_text()):
            source = source_of(name, old_map)
            print(f"  {name} changed: {source}", flush=True)
            if source:
                bad.add(source)
    for path in sorted((ROOT / "config/versions").glob("*/map.tsv")):
        rel = path.relative_to(ROOT).as_posix()
        bad |= version_losses(sh(f"git show HEAD:{rel}").stdout, path.read_text())
    bad |= compile_failures(log)
    changed = changed_files()
    # A lost fragment with no change of its own points at its shared file.
    blamed = {path if path in changed else shared_file(path) for path in bad}
    return blamed & changed


def fragments_of(path):
    """A shared file's version fragments: its declarations may be what they need."""
    if path.startswith("src/versions/"):
        return set()
    stem = path[len("src/"):-len(".c")]
    return {p.relative_to(ROOT).as_posix() for p in ROOT.glob(f"src/versions/*/{stem}/*.c")}


def main():
    for number in range(1, MAX_ROUNDS + 1):
        sh("rm -f build-sn/rac1.elf")
        gate = sh(f"bash tools/check_all.sh > {LOG} 2>&1")
        log = LOG.read_text(errors="replace")
        make_log = ROOT / "build-sn/make.log"
        if make_log.exists():
            log += "\n" + make_log.read_text(errors="replace")
        print(f"round {number}: gate={gate.returncode}", flush=True)
        if gate.returncode == 0:
            print("PASS", flush=True)
            return 0
        bad = files_to_blame(log)
        if not bad:
            print("STOP: the gate failed, and no source file is to blame. Read this:", flush=True)
            print("\n".join(LOG.read_text(errors="replace").splitlines()[-12:])[:1500], flush=True)
            return 1
        for path in sorted(bad):
            print(f"  REVERTED {path}", flush=True)
            sh(f"git checkout -- {path}")
            for fragment in fragments_of(path) & changed_files():
                print(f"  REVERTED {fragment}", flush=True)
                sh(f"git checkout -- {fragment}")
        sh("git checkout -- config/versions progress")
    print("STOP: too many rounds", flush=True)
    return 1


if __name__ == "__main__":
    sys.exit(main())
