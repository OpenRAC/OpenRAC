#!/usr/bin/env python3
"""
Fail if this tree matches less than the last commit.

  python3 tools/check_regressions.py

It compares with HEAD: every report's match number, and every function's status in each version's map. A path in a map
is ignored, because moving a file changes it. Run it after tools/check_all.sh has refreshed the reports.
"""
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RANK = {"match": 2}


def at_head(path):
    run = subprocess.run(["git", "show", f"HEAD:{path}"], cwd=ROOT, capture_output=True, text=True)
    return run.stdout if run.returncode == 0 else None


def report_drops(old_text, new_text):
    """(old, new) if the match number fell."""
    old = json.loads(old_text)["measures"]["fuzzy_match_percent"]
    new = json.loads(new_text)["measures"]["fuzzy_match_percent"]
    return (old, new) if new < old else None


def map_rows(text):
    """{function: status}. A map's columns are: function, source path, status, then the rest."""
    rows = (line.split("\t") for line in text.splitlines() if line and not line.startswith("#"))
    return {row[0]: row[2] for row in rows if len(row) > 2}


def map_regressions(old_text, new_text):
    """Functions that were a match and are not now, or are gone."""
    old, new = map_rows(old_text), map_rows(new_text)
    return sorted(name for name, status in old.items() if status == "match" and new.get(name) != "match")


def main():
    bad = 0
    for path in sorted((ROOT / "progress").rglob("report.json")):
        rel = path.relative_to(ROOT).as_posix()
        old = at_head(rel)
        if old is None:
            continue
        drop = report_drops(old, path.read_text())
        if drop:
            print(f"*** {rel}: {drop[0]} -> {drop[1]}")
            bad += 1
    for path in sorted((ROOT / "config/versions").glob("*/map.tsv")):
        rel = path.relative_to(ROOT).as_posix()
        old = at_head(rel)
        if old is None:
            continue
        lost = map_regressions(old, path.read_text())
        if lost:
            print(f"*** {rel}: {len(lost)} no longer match: {', '.join(lost[:6])}")
            bad += 1
    if bad:
        sys.exit(f"*** {bad} check(s) got worse than HEAD")
    print("no report and no function got worse than HEAD")


if __name__ == "__main__":
    main()
