#!/usr/bin/env python3
"""
Turn raw offsets into struct fields in bulk, where the source of the pointer makes its type certain.

  python3 tools/tidy/raw_views.py [--write] [--list] [--only=RULE] [--skip=A,B] [FILE...]

A byte pointer converts only when every value it is given matches one rule:
  create_moby   CreateMoby()                 Moby
  hero          g_hero                       Hero
  memcard       memory_card_state            MemoryCardState
  hit           MobyGetHitMessage()          HitMessage
  pvars         the moby's pvars             a new struct, one per function, with unk_XX fields

tools/tidy/rawstruct.py and rawpvars.py do the rewriting. --skip=A,B leaves functions raw, by address or readable name:
use it for functions the gate says change. Run tools/gate_loop.py after --write.
"""
import collections
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import moby_views as mv  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = mv.ROOT
HERE = mv.HERE
RULES = {
    "create_moby": (r"(?:\([^()]*\)\s*)?CreateMoby\s*\(", "Moby", "structs/moby.h"),
    "hero": (r"(?:\([^()]*\)\s*)?&?g_hero(?:_r)?\s*;", "Hero", "structs/hero.h"),
    "memcard": (r"(?:\([^()]*\)\s*)?&?memory_card_state\s*;", "MemoryCardState", "structs/memcard.h"),
    "hit": (r"(?:\([^()]*\)\s*)?MobyGetHitMessage(?:_r)?\s*\(", "struct HitMessage", "structs/hit_message.h"),
}
PVARS = (r"(?:(?:\(+\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?\(*\s*(?:\(\s*(?:struct\s+)?Moby\s*\*\s*\)\s*)?[\w.]+\s*\)*\s*->\s*pvars\s*\)*"
         r"|\*\s*\(\s*(?:unsigned\s+)?char\s*\*\*\s*\)\s*\(\s*\w+\s*\+\s*0x78\s*\))\s*;")


def sole_source(body, var, pattern):
    """True if var is assigned, and every assignment matches the pattern."""
    assigned = re.findall(rf"\b{var}\s*=(?!=)", body)
    matching = re.findall(rf"\b{var}\s*=\s*{pattern}", body)
    return bool(assigned) and len(assigned) == len(matching)


def functions(text):
    clean = mv.COMMENT.sub(" ", text)
    starts = list(mv.FUNC.finditer(clean))
    for i, m in enumerate(starts):
        end = starts[i + 1].start() if i + 1 < len(starts) else len(clean)
        yield m.group(1), m.group(2), clean[m.end():end]


def raw_vars(body):
    """Names used as the base of a byte offset."""
    return set(re.findall(r"\(\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?\b([A-Za-z_]\w*)\s*\+\s*(?:0x[0-9A-Fa-f]+|\d+)\s*\)", body))


def plan(text, only=None):
    """[(rule, function, var)]"""
    found = []
    for name, params, body in functions(text):
        for var in sorted(raw_vars(body)):
            if not mv.declared_as_bytes(params, body, var):
                continue
            if not re.search(mv.RAW.format(v=var), body):
                continue
            for rule, (pattern, _, _) in RULES.items():
                if sole_source(body, var, pattern):
                    found.append((rule, name, var))
                    break
            else:
                if sole_source(body, var, PVARS):
                    found.append(("pvars", name, var))
    return [row for row in found if not only or row[0] == only]


def add_header(text, header):
    line = f'#include "{header}"\n'
    if line in text:
        return text
    m = re.search(r'^#include "common.h"\n', text, re.M)
    return text[:m.end()] + line + text[m.end():] if m else None


def struct_name(function):
    words = re.split(r"_+", re.sub(r"^func_", "", function))
    return "".join(w[:1].upper() + w[1:] for w in words if w) + "Vars"


def definition_start(text, function):
    """Where to put a struct: before the function's comment, if it has one."""
    m = re.search(rf"^[A-Za-z_][^\n;{{}}]*\b{re.escape(function)}\s*\([^;{{}}]*\)\s*\{{", text, re.M)
    if not m:
        return None
    at = m.start()
    before = text[:at].rstrip()
    if before.endswith("*/"):
        at = before.rfind("/*")
        line = text.rfind("\n", 0, at) + 1
        at = line if not text[line:at].strip() else m.start()
    return at


def run(tool, *args):
    return subprocess.run([sys.executable, str(HERE / tool), *args], cwd=ROOT, capture_output=True, text=True)


def convert_pvars(rel, function):
    infer = run("rawpvars.py", "--infer", f"--func={function}", rel)
    draft = infer.stdout
    if infer.returncode or "struct DraftVars {" not in draft:
        return f"no draft: {infer.stderr.strip()[-80:]}"
    path = ROOT / rel
    text = path.read_text(errors="surrogateescape")
    name = struct_name(function)
    if re.search(rf"\b{name}\b", text):
        return f"{name} is taken"
    body = draft[draft.index("struct DraftVars {"):]
    body = re.sub(r"\s*/\* 0x\w+; \d+ uses? \*/", "", body[:body.index("};") + 2]).replace("DraftVars", name)
    body = re.sub(r"(\w+ \*?) +(\w+;)", r"\1 \2", body)
    at = definition_start(text, function)
    if at is None:
        return "no definition"
    path.write_text(text[:at] + body + "\n\n" + text[at:], errors="surrogateescape")
    done = run("rawpvars.py", f"--struct={name}", f"--func={function}", rel)
    if done.returncode:
        path.write_text(text, errors="surrogateescape")
        return f"stopped: {done.stderr.strip()[-80:]}"
    return None


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    only = next((a[7:] for a in args if a.startswith("--only=")), None)
    paths = [a for a in args if not a.startswith("--")]
    skip = set(",".join(a[7:] for a in args if a.startswith("--skip=")).split(",")) - {""}
    for name, others in pe.load_aliases().items():
        if name in skip:
            skip |= set(others)
    if not paths:
        out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
        paths = [p for p in out.split() if not p.startswith(("src/data/", "src/libgcc/", "src/versions/"))]
    counts = collections.Counter()
    for rel in paths:
        path = ROOT / rel
        rows = [r for r in plan(path.read_text(errors="surrogateescape"), only) if r[1] not in skip]
        for rule, function, var in rows:
            counts[rule] += 1
            if listing:
                print(f"{rule}\t{rel}\t{function}\t{var}")
            if not write:
                continue
            if rule == "pvars":
                problem = convert_pvars(rel, function)
            else:
                _, struct, header = RULES[rule]
                new = add_header(path.read_text(errors="surrogateescape"), header)
                if new is None:
                    print(f"{rel}: no common.h")
                    continue
                path.write_text(new, errors="surrogateescape")
                done = run("rawstruct.py", rel, f"--func={function}", f"--var={var}", f"--struct={struct}")
                problem = done.stderr.strip()[-80:] if done.returncode else None
            if problem:
                print(f"{rel}: {function}: {problem}", flush=True)
    print(f"{'converted' if write else 'would convert'}: {dict(counts)}")


if __name__ == "__main__":
    main()
