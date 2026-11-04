#!/usr/bin/env python3
"""
Ask Ghidra which functions use each data block with no owner, and give the block to their file.

  python3 tools/tidy/data_owners.py scan      # asks the running Ghidra, writes config/data_xrefs.tsv
  python3 tools/tidy/data_owners.py show      # what would move
  python3 tools/tidy/data_owners.py --write   # move the blocks

Start Ghidra first: bash tools/docker/ghidra_mcp.sh start. It only knows the executable, so a block used only by level code
has no references here. A block moves only if every function that uses it is in one file. Otherwise it has no one owner.
"""
import json
import os
import re
import subprocess
import sys
import urllib.request
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools/tidy"))
import data_in_c as d  # noqa: E402
import prune_externs as pe  # noqa: E402

TABLE = ROOT / "config/data_xrefs.tsv"
HEADER = "# Functions that use each data block with no owner, from Ghidra (tools/tidy/data_owners.py). Addresses only.\n# label\tfunctions\n"
SHARED = "src/data/shared/data.c"
URL = os.environ.get("GHIDRA_URL", "http://127.0.0.1:8089")
FUNCTION = re.compile(r"^[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;{}]*\)\s*\{", re.M)


def token():
    if os.environ.get("GHIDRA_MCP_AUTH_TOKEN"):
        return os.environ["GHIDRA_MCP_AUTH_TOKEN"]
    out = subprocess.run(["podman", "inspect", "ghidra-ps2-mcp", "--format", "{{range .Config.Env}}{{println .}}{{end}}"],
                         capture_output=True, text=True).stdout
    return next(line.split("=", 1)[1] for line in out.splitlines() if line.startswith("GHIDRA_MCP_AUTH_TOKEN="))


def references(label, secret):
    request = urllib.request.Request(f"{URL}/get_xrefs_to?address=0x{d.address(label):08X}&limit=200",
                                     headers={"Authorization": f"Bearer {secret}"})
    with urllib.request.urlopen(request, timeout=60) as reply:
        found = json.load(reply).get("references", [])
    return sorted({normal_name(r["from_function"]) for r in found if r.get("from_function")})


def normal_name(name):
    """FUN_001138a8 -> func_001138A8, as the name table spells it."""
    m = re.fullmatch(r"FUN_([0-9A-Fa-f]+)", name)
    return f"func_{m.group(1).upper()}" if m else name


def scan():
    secret = token()
    labels = [row[0] for row in d.rows() if row[2] == SHARED]
    with ThreadPoolExecutor(8) as pool:
        results = list(pool.map(lambda label: references(label, secret), labels))
    rows = "".join(f"{label}\t{','.join(functions)}\n" for label, functions in zip(labels, results) if functions)
    TABLE.write_text(HEADER + rows)
    print(f"{len(labels)} blocks asked, {sum(map(bool, results))} have references")


def load():
    return {line.split("\t")[0]: line.rstrip("\n").split("\t")[1].split(",")
            for line in TABLE.read_text().splitlines() if line and not line.startswith("#")}


def function_files():
    """{function name: source file}, for every spelling of the name."""
    aliases = pe.load_aliases()
    out = subprocess.run(["git", "ls-files", "src/core/*.c", "src/game/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
    files = {}
    for rel in out.split():
        for name in FUNCTION.findall((ROOT / rel).read_text(errors="replace")):
            files[name] = rel
            for other in aliases.get(name, ()):
                files[other] = rel
    return files


def owner_of(functions, files):
    """The one file all these functions are in, or None. A function with no C file is not evidence for one."""
    owners = {files.get(f) for f in functions}
    return owners.pop() if len(owners) == 1 and None not in owners else None


def plan():
    files = function_files()
    moves, why = {}, Counter()
    for label, functions in load().items():
        owner = owner_of(functions, files)
        if owner:
            moves[label] = "src/data/" + owner[len("src/"):]
            why["one file"] += 1
        else:
            why["several files, or asm"] += 1
    return moves, why


def main():
    command = (sys.argv[1:] or [""])[0]
    if command == "scan":
        scan()
    elif command == "show":
        moves, why = plan()
        print(dict(why))
        print(Counter(moves.values()).most_common(10))
    elif command == "--write":
        moves, why = plan()
        table = [(label, element, moves.get(label, source)) for label, element, source in d.rows()]
        d.write_all(table, d.all_blocks(), d.readable_names())
        print(f"moved {len(moves)} blocks: {dict(why)}")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
