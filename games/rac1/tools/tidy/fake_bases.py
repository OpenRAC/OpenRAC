#!/usr/bin/env python3
"""
Replace fake base addresses with the globals they really are.

  python3 tools/tidy/fake_bases.py                  # dry run: show counts
  python3 tools/tidy/fake_bases.py --write [FILE...]
  python3 tools/tidy/fake_bases.py --list  [FILE...]
  python3 tools/tidy/fake_bases.py --fragments      # the same in src/versions/, using each shared file's declarations
  python3 tools/tidy/fake_bases.py --scalars        # `*(int *)name` becomes `name`, where that is the file's only use

`hero_globals_base + 0xE1D` is the address of g_hero: base plus constant lands exactly on a named global in the
executable's symbol table. The bases are labels at odd addresses, used so GCC keeps the address in a register. Most
functions do not need that, so they get the real name. --write bisects each file with tools/overlay_file_check.py, and
the functions that stop matching keep their old spelling. Run tools/gate_loop.py afterwards: it covers the other versions.
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
BASES = ("hero_globals_base", "help_stats_base", "pad_globals_base", "special_items_base", "screen_globals_base",
         "race_gates_base", "level_render_state_base", "level_available_base", "menu_item_enabled_115")
HERO_TYPES = r"(?:struct\s+)?Hero"


def symbols():
    """{readable or address base name: address}, {address: readable global}"""
    defined = dict(re.findall(r"^#define\s+(\w+)\s+(D_[0-9A-Fa-f]{8})\b", (ROOT / "include/names/executable.h").read_text(errors="replace"), re.M))
    bases = {}
    for name in BASES:
        if name in defined:
            address = int(defined[name][2:], 16)
            bases[name] = address
            bases[defined[name]] = address
    fake = set(BASES) | {defined[b] for b in BASES if b in defined}
    targets = {int(v[2:], 16): k for k, v in defined.items() if k not in fake}
    return bases, targets


def with_local_aliases(text, bases):
    """bases, plus this file's byte-array aliases of them: `extern char hero_globals_base_r[] __asm__("D_0013E633");`."""
    found = dict(bases)
    pattern = r'^\s*extern\s+(?:(?:unsigned\s+)?char|u8)\s+(\w+)\s*\[\s*\]\s*__asm__\s*\(\s*"(\w+)"\s*\)[^;\n]*;'
    for name, symbol in re.findall(pattern, pe.without_comments(text), re.M):
        if symbol in bases:
            found[name] = bases[symbol]
    return found


def drop_unused_aliases(text, bases, fragments=""):
    """Remove the declaration of a local alias that nothing uses any more, here or in a version fragment."""
    code = pe.without_comments(text)
    extra = pe.without_comments(fragments)
    for name in set(with_local_aliases(text, bases)) - set(bases):
        declaration = rf'^[ \t]*extern\s[^;\n]*\b{name}\s*\[\s*\]\s*__asm__[^;\n]*;[ \t]*\n'
        declared = len(re.findall(declaration, code, re.M))
        if len(re.findall(rf"\b{name}\b", code)) == declared and not re.search(rf"\b{name}\b", extra):
            text = re.sub(declaration, "", text, flags=re.M)
    return text


def base_sites(masked, bases):
    """(start, end, global address) of each `base + constant`, in code. The constant must stand alone."""
    names = "|".join(sorted(map(re.escape, bases), key=len, reverse=True))
    for m in re.finditer(rf"(?<![\w.>])({names})\s*\+\s*(0x[0-9A-Fa-f]+|\d+)\b(?!\s*[*/%])", masked):
        yield m.start(), m.end(), bases[m.group(1)] + int(m.group(2), 0)


CHAR_CAST = r"\(\s*(?:(?:unsigned\s+)?char|u8)\s*\*\s*\)\s*"


def spell(text, start, end, name, view):
    """(new start, new end, new text) for one site, widened to its casts."""
    before, after = text[:start], text[end:]
    m = re.search(rf"(?:{CHAR_CAST})?&?\s*$", before)
    if m and m.group(0).strip():
        start = m.start()
        before = text[:start]
    wrapped = re.search(r"\(\s*$", before) and re.match(r"\s*\)", after)
    if wrapped:
        cast = re.search(r"\(\s*((?:const\s+|volatile\s+)*[\w ]+?\s*\*+)\s*\)\s*\(\s*$", before)
        if cast:
            typ = cast.group(1)
            close = end + re.match(r"\s*\)", after).end()
            if view == "object" and re.fullmatch(HERO_TYPES + r"\s*\*", typ):
                outer = re.search(r"\(\s*$", before[:cast.start()])
                tail = re.match(r"\s*\)\s*->", text[close:])
                if outer and tail:
                    return outer.start(), close + tail.end(), f"{name}."
                return cast.start(), close, f"&{name}"
            address = f"&{name}" if view == "object" else name
            return cast.start(), close, f"({typ})({address})" if view == "object" else f"({typ}){address}"
    return start, end, f"(char *)&{name}" if view == "object" else name


def split_decl(text, name):
    """The file's declarations of name: a list of their text."""
    return [m.group(0).strip() for m in re.finditer(rf"^\s*extern\b[^;\n]*\b{name}\b[^;\n]*;", pe.without_comments(text), re.M)]


def view_of(text, name):
    """(view, declaration to add), or None if the file's declarations of name disagree."""
    found = {re.sub(r"\bstruct\s+", "", d) for d in split_decl(text, name)}
    if len(found) > 1:
        return None
    if found:
        decl = found.pop()
        if "__asm__" in decl:
            return None
        return ("array" if re.search(rf"\b{name}\s*\[", decl) else "object"), decl
    if name == "g_hero":
        return "object", "extern Hero g_hero;"
    return "array", f"extern char {name}[];"


def after_first_includes(text):
    """Where the top block of includes ends. Some files have another #include far below, which is too late."""
    end = 0
    for m in re.finditer(r"^(?:#include .*|\s*)\n", text, re.M):
        if m.start() != end and end:
            break
        end = m.end()
    last = 0
    for m in re.finditer(r"^#include .*\n", text[:end], re.M):
        last = m.end()
    return last


def rewrite(text, bases, targets, keep=(), blocked=(), scope=None, declare=True, declared=None):
    """(new text, sites changed). Functions in keep and globals in blocked stay as they are.

    scope is the text whose declarations decide each view: a fragment has none of its own, so it uses its shared file.
    declare=False adds no declarations, and declared collects the ones the text needs.
    """
    bases = with_local_aliases(scope if scope is not None else text, bases)
    masked = pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    spans = [(m.start(), m.group(1)) for m in mv.FUNC.finditer(masked)]
    sites = []
    views = {}
    for start, end, address in base_sites(masked, bases):
        name = targets.get(address)
        if not name or f"D_{address:08X}" in blocked:
            continue
        owner = next((n for at, n in reversed(spans) if at <= start), None)
        if owner in keep:
            continue
        if name not in views:
            views[name] = view_of(scope if scope is not None else text, name)
        if views[name]:
            sites.append((start, end, name))
    edits = []
    for start, end, name in sites:
        edits.append(spell(text, start, end, name, views[name][0]))
    out, last = [], 0
    for start, end, new in sorted(edits):
        out += [text[last:start], new]
        last = end
    out.append(text[last:])
    new = "".join(out)
    needed = sorted({name for _, _, name in sites})
    decls = "".join(views[n][1] + "\n" for n in needed)
    if decls and declare:
        at = after_first_includes(new)
        if "extern Hero g_hero;" in decls and "structs/hero.h" not in new:
            decls = '#include "structs/hero.h"\n' + decls
        new = new[:at] + decls + new[at:]
    if declared is not None:
        declared.update(views[n][1] for n in needed)
    return new, len(sites)


def check(rel):
    """(names of functions that do not match, symbols that clash, whether the file failed to build)."""
    tool = "tools/overlay_file_check.py" if rel.startswith("src/overlays/") else "tools/file_check.py"
    run = subprocess.run(["bash", "tools/docker/run.sh", "python", tool, rel], cwd=ROOT,
                         capture_output=True, text=True)
    out = run.stdout + run.stderr
    clash = set(re.findall(r"(?:conflicting types for|redeclaration of|previous declaration of) `(\w+)'|`(\w+)' undeclared", out))
    clash = {name for pair in clash for name in pair if name}
    return set(re.findall(r"NOT EXACT [^:]+: (\w+)", out)), clash, "BUILD FAIL" in out


def site_owners(text, bases, targets):
    """Names of the functions that hold a site. None stands for a site outside any function."""
    bases = with_local_aliases(text, bases)
    masked = pe.COMMENT_OR_STRING.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    spans = [(m.start(), m.group(1)) for m in mv.FUNC.finditer(masked)]
    return {next((n for at, n in reversed(spans) if at <= start), None)
            for start, _, address in base_sites(masked, bases) if address in targets}


def accepted_functions(owners, passes):
    """The largest set of owners found by bisecting: passes(set) says whether converting that set still matches."""
    accepted = set()

    def solve(group):
        if not group:
            return
        if passes(accepted | set(group)):
            accepted.update(group)
        elif len(group) > 1:
            middle = len(group) // 2
            solve(group[:middle])
            solve(group[middle:])

    solve(sorted(owners, key=str))
    return accepted


def land(rel, bases, targets, _aliases=None):
    path = ROOT / rel
    original = path.read_text(errors="surrogateescape")
    blocked = set()
    if not rel.startswith("src/overlays/"):
        new, count = rewrite(original, bases, targets)
        path.write_text(new, errors="surrogateescape")
        return count, "unchecked: the gate covers it"
    owners = site_owners(original, bases, targets)

    def passes(chosen):
        while True:
            new, count = rewrite(original, bases, targets, owners - chosen, blocked)
            if not count:
                return True
            path.write_text(new, errors="surrogateescape")
            bad, clash, failed = check(rel)
            if failed and clash - blocked:
                blocked.update(clash)
                continue
            return not bad and not failed

    accepted = accepted_functions(owners, passes)
    new, count = rewrite(original, bases, targets, owners - accepted, blocked)
    fragments = "\n".join(f.read_text(errors="replace") for f in fragments_of(rel))
    path.write_text(drop_unused_aliases(new, bases, fragments), errors="surrogateescape")
    bad, _, failed = check(rel) if count else (set(), set(), False)
    if bad or failed:
        path.write_text(original, errors="surrogateescape")
        return 0, "left as it was: the final check failed"
    kept = len(owners - accepted)
    return count, f"{kept} functions kept, {len(blocked)} globals blocked" if kept or blocked else ""


def fragments_of(rel):
    stem = rel[len("src/"):-len(".c")]
    return sorted(ROOT.glob(f"src/versions/*/{stem}/*.c"))


def land_fragments(rel, bases, targets):
    """Use real names in the file's version fragments. They are spliced into it, so it holds the declarations."""
    path = ROOT / rel
    shared = path.read_text(errors="surrogateescape")
    declared, total = set(), 0
    for fragment in fragments_of(rel):
        text = fragment.read_text(errors="surrogateescape")
        new, count = rewrite(text, bases, targets, scope=shared, declare=False, declared=declared)
        if count:
            fragment.write_text(new, errors="surrogateescape")
            total += count
    missing = [d for d in sorted(declared) if d not in shared]
    if missing:
        at = after_first_includes(shared)
        path.write_text(shared[:at] + "".join(d + "\n" for d in missing) + shared[at:], errors="surrogateescape")
    return total


SMALL = {"char", "u8", "s8", "short", "s16", "u16"}


def scalar_type(text, name):
    """The type T if the file's every use of a char[] global is the read or write *(T *)name, else None."""
    masked = pe.without_comments(text)
    if not re.search(rf"^extern char {name}\[\];$", masked, re.M):
        return None
    types = re.findall(rf"\*\(\s*((?:unsigned\s+)?\w+)\s*\*\s*\)\s*{name}\b(?!\s*[+\[])", masked)
    uses = len(re.findall(rf"\b{name}\b", masked)) - 1
    return types[0] if types and len(types) == uses and len(set(types)) == 1 else None


def give_type(text, name, typ):
    attribute = " NOT_SDA" if typ.split()[-1] in SMALL else ""
    new = re.sub(rf"^extern char {name}\[\];$", f"extern {typ} {name}{attribute};", text, flags=re.M)
    return re.sub(rf"\*\(\s*{re.escape(typ)}\s*\*\s*\)\s*{name}\b", name, new)


def land_scalars(rel, targets):
    """Give each char[] global its one real type, one global at a time, keeping those that still match."""
    path = ROOT / rel
    done = 0
    for name in sorted(set(targets.values())):
        text = path.read_text(errors="surrogateescape")
        typ = scalar_type(text, name)
        if not typ or fragments_use(rel, name):
            continue
        path.write_text(give_type(text, name, typ), errors="surrogateescape")
        bad, _, failed = check(rel)
        if bad or failed:
            path.write_text(text, errors="surrogateescape")
        else:
            done += 1
    return done


def fragments_use(rel, name):
    return any(re.search(rf"\b{name}\b", f.read_text(errors="replace")) for f in fragments_of(rel))


def main():
    args = sys.argv[1:]
    write, listing = "--write" in args, "--list" in args
    if "--fragments" in args or "--scalars" in args:
        bases, targets = symbols()
        out = subprocess.run(["git", "ls-files", "src/overlays/*.c", "src/game/*.c", "src/core/*.c"], cwd=ROOT,
                             capture_output=True, text=True).stdout
        total = 0
        for rel in out.split():
            if rel.startswith("src/versions/"):
                continue
            total += land_fragments(rel, bases, targets) if "--fragments" in args else (
                land_scalars(rel, targets) if rel.startswith("src/overlays/") else 0)
        print(f"changed {total}")
        return
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout
        paths = [p for p in out.split() if not p.startswith(("src/data/", "src/libgcc/", "src/versions/"))]
    bases, targets = symbols()
    totals = collections.Counter()
    for rel in paths:
        text = (ROOT / rel).read_text(errors="surrogateescape")
        _, count = rewrite(text, bases, targets)
        if not count:
            continue
        totals["files"] += 1
        totals["sites"] += count
        if write:
            done, note = land(rel, bases, targets)
            totals["landed"] += done
            print(f"{rel}: {done}/{count} {note}".rstrip(), flush=True)
        elif listing:
            print(f"{rel}: {count}")
    print(dict(totals))


if __name__ == "__main__":
    main()
