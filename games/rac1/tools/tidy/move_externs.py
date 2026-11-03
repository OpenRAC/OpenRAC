#!/usr/bin/env python3
"""
Move function declarations from .c files into headers.

  python3 tools/tidy/move_externs.py --plan                     # what could move, per header
  python3 tools/tidy/move_externs.py --split                    # one function per extern statement
  python3 tools/tidy/move_externs.py --header asm_functions/executable.h [--top N] [--write]

One symbol gets one prototype, and it moves only if every file accepts that prototype.

  - Pointer parameters all become `void *`: their type has no bytes. Scalars must match exactly, because `short` and
    `int` make different code. `void` and `int` returns make the same code, so a value return wins.
  - A symbol with `__asm__`, `SDATA`, `MACRO_ADDR` or `NOT_SDA` on its own name in any file stays: GCC keeps the first
    declaration of a name, so a plain one in a header would silently cancel that.
  - `()` goes if the file still matches. If not, the line stays: a later `()` keeps the call unprototyped.
  - A declaration inside a function stays. It has its own scope. A prototype without `extern` counts like one with it.

--write compiles every affected file against retail. A file that fails is bisected to the symbols it cannot accept.
Those symbols leave the header and every file keeps its own line. Run tools/format.py and tools/gate_loop.py afterwards:
only the gate covers the other versions.
"""
import collections
import concurrent.futures
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fake_bases as fb  # noqa: E402
import merge_types as mt  # noqa: E402
import move_protos as mp  # noqa: E402
import prune_externs as pe  # noqa: E402

ROOT = pe.ROOT
QUALIFIED = re.compile(r"__asm__|__attribute__|MACRO_ADDR|NOT_SDA|SDATA")
WORKERS = 6
INCLUDE_LINE = re.compile(r'^#include\s+"([^"]+)"', re.M)


# `void f(p)\n    char *p;\n{`: a prototype in a header would change how this definition compiles.
OLD_STYLE = re.compile(r"^(?!extern\b|typedef\b)[A-Za-z_][^;{}=\n]*?\b(\w+)\s*\(\s*\w+(?:\s*,\s*\w+)*\s*\)\s*\n\s*[A-Za-z_][^;{}()]*;", re.M)
INNER_EXTERN = re.compile(r"\bextern\b[^;{}]*;")
PLAIN = re.compile(r"^(?!extern\b|static\b|typedef\b|return\b|else\b|goto\b)[A-Za-z_][\w \t*]*[ \t*]\w+\s*\([^;{}]*\)\s*;", re.M)


def declarations(text):
    """(start, end, statement) of each file-scope declaration: `extern` ones, and prototypes written without it."""
    found = list(pe.find_externs(text))
    masked = pe.mask(text)
    depth, at = 0, 0
    for m in PLAIN.finditer(masked):
        depth += masked.count("{", at, m.start()) - masked.count("}", at, m.start())
        at = m.start()
        if depth == 0:
            found.append((m.start(), m.end(), text[m.start():m.end()]))
    return sorted(found)


def split_prototype(name, statement):
    """(return type, [parameters]), or None if this is not a plain function declaration. No parameters is `()`."""
    text = mp.normal(statement)
    m = re.fullmatch(rf"(?:extern\s+)?(.*?)\b{re.escape(name)}\s*\((.*)\)\s*;", text)
    if not m:
        return None
    return m.group(1).strip(), [p.strip() for p in pe.split_declarators(m.group(2))] if m.group(2).strip() else []


def scalar(kind):
    """A type in one spelling: every pointer is the same, and a scalar keeps its width and sign."""
    if "*" in kind or "[" in kind:
        return "void *"
    if "..." in kind:
        return "..."
    for pattern, spelling in mp.SPELLINGS:
        kind = re.sub(pattern, spelling, kind)
    words = [w for w in re.findall(r"\w+", kind) if w not in ("const", "volatile")]
    types = [w for w in words if w in mp.TYPE_WORDS]
    # A last word that is not a type is the parameter's name. A typedef name alone cannot be judged here.
    if types:
        return " ".join(types)
    return " ".join(words[:1]) if len(words) < 2 else " ".join(words[:-1])


def shape(prototype):
    """What decides the code at a call: parameter types, or None for `()`."""
    _, params = prototype
    if not params:
        return None
    return tuple(scalar(p) for p in params)


def canonical(name, prototypes):
    """The one declaration for a symbol, from every file's (return, params). None if it has no usable shape."""
    shapes = collections.Counter(s for s in map(shape, prototypes) if s is not None)
    if not shapes:
        return None
    best = sorted(shapes.items(), key=lambda row: (-row[1], row[0]))[0][0]
    returns = collections.Counter(scalar(p[0]) for p in prototypes if shape(p) == best)
    values = [r for r, _ in sorted(returns.items(), key=lambda row: (-row[1], row[0])) if r != "void"]
    result = values[0] if values else "void"
    if mp.NEEDS_TYPES.search(result) or any(mp.NEEDS_TYPES.search(p) for p in best):
        return None
    space = "" if result.endswith("*") else " "
    return f"extern {result}{space}{name}({', '.join(best)});"


def definition_prototype(name, text):
    """(return, params) of this function's definition, or None. Parameter names are dropped."""
    m = re.search(rf"^(?!extern\b|static\b|typedef\b)([A-Za-z_][\w \t\*]*?)\b{re.escape(name)}\s*\(([^;{{}}()]*)\)\s*\{{",
                  pe.COMMENT_OR_STRING.sub(pe.blank, text), re.M)
    if not m:
        return None
    params = []
    for param in pe.split_declarators(m.group(2)) if m.group(2).strip() else []:
        kind = " ".join(param.split())
        kind = re.sub(r"\s*\b\w+\s*(\[[^\]]*\])?$", lambda w: " *" if w.group(1) else "", kind) if " " in kind or "*" in kind else kind
        params.append(kind.strip())
    return " ".join(m.group(1).split()), params


def is_file_scope(text, start):
    line_start = text.rfind("\n", 0, start) + 1
    return text[line_start:start].strip() == "" and start == line_start


class Sources:
    """Every file's function declarations, grouped by symbol."""

    def __init__(self):
        out = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout.split()
        self.files = [f for f in out if not f.startswith(("src/data/", "src/libgcc/", "src/versions/"))]
        self.text = {f: (ROOT / f).read_text(errors="surrogateescape") for f in self.files}
        self.fragments = {f: pe.fragments_text(f) for f in self.files}
        self.aliases = pe.load_aliases()
        self.lines = collections.defaultdict(list)      # symbol: [(file, name, prototype)]
        self.blocked = {}                               # symbol: why it cannot move
        self.defined = collections.defaultdict(set)
        self.existing = {}                              # symbol: (header, declaration text)
        for path in sorted((ROOT / "include").rglob("*.h")):
            if path.parent.name == "names" or path.name == "names.h":
                continue
            for _, _, statement in pe.find_externs(path.read_text(errors="replace")):
                found = pe.declared_names(statement)
                if found and len(found) == 1:
                    self.existing.setdefault(self.symbol(found[0]), (path.relative_to(ROOT / "include").as_posix(), statement))
        for rel, text in self.text.items():
            for name in mp.DEFINITION.findall(pe.COMMENT_OR_STRING.sub(pe.blank, text)):
                self.defined[self.symbol(name)].add(rel)
            for name in OLD_STYLE.findall(pe.COMMENT_OR_STRING.sub(pe.blank, text)):
                self.blocked[self.symbol(name)] = f"old-style definition in {rel}"
            for start, _, statement in declarations(text):
                if not is_file_scope(text, start):
                    continue
                names = pe.declared_names(statement) or []
                for name in names:
                    self.note(rel, name, statement, len(names))

    def symbol(self, name):
        return min({name} | self.aliases.get(name, set()))

    def note(self, rel, name, statement, count):
        symbol = self.symbol(name)
        if not mp.is_function(name, statement):
            return
        if QUALIFIED.search(statement):
            label = re.search(r'__asm__\s*\(\s*"(\w+)"', statement)
            if not label or self.symbol(label.group(1)) == symbol:
                self.blocked[symbol] = "qualified"
            return
        if count > 1:
            self.blocked[symbol] = "shares a statement"
            return
        prototype = split_prototype(name, statement)
        if prototype is None:
            self.blocked[symbol] = "not parsed"
            return
        self.lines[symbol].append((rel, name, prototype))

    def from_definition(self, symbol, name, homes, header):
        """The definition's own types, when its file takes this header: anything else would clash there."""
        if not homes or not header:
            return None
        home = next(iter(homes))
        if f'"{header}"' not in self.text[home]:
            return None
        found = definition_prototype(name, self.text[home]) or next(
            (definition_prototype(other, self.text[home]) for other in self.aliases.get(name, ())
             if definition_prototype(other, self.text[home])), None)
        if not found or not found[1]:
            return None
        result, params = found
        space = "" if result.endswith("*") else " "
        return f"extern {result}{space}{name}({', '.join(params)});"

    def plan(self, rejected=()):
        """{header: {symbol: (name, declaration)}}"""
        headers = collections.defaultdict(dict)
        for symbol, rows in self.lines.items():
            if symbol in self.blocked or symbol in rejected:
                continue
            outside = [row for row in rows if row[0] not in self.defined.get(symbol, ())]
            if not outside:
                continue
            spelled = collections.Counter(row[1] for row in rows)
            names = {symbol} | self.aliases.get(symbol, set())
            readable = [n for n in names if not re.match(r"(?:func|D)_", n)]
            name = readable[0] if len(readable) == 1 else spelled.most_common(1)[0][0]
            homes = self.defined.get(symbol, set())
            if symbol in self.existing:
                header, statement = self.existing[symbol]
                headers[header][symbol] = (name, None)
                continue
            if len(homes) > 1:
                continue
            header = mp.home_of(name, f"{name}(", homes, self.aliases)
            text = self.from_definition(symbol, name, homes, header) or canonical(name, [row[2] for row in rows])
            if header and text:
                headers[header][symbol] = (name, text)
        return headers


def address_of(sources, symbol):
    names = {symbol} | sources.aliases.get(symbol, set())
    found = [n for n in names if re.fullmatch(r"func_(?:L\d+_)?[0-9A-Fa-f]+", n)]
    return found[0] if found else None


def variant_names(sources, symbol):
    """Every spelling of a symbol goes to one alias, or the code calls a name nothing declares."""
    names = {symbol} | sources.aliases.get(symbol, set())
    readable = [name for name in names if not re.match(r"(?:func|D)_", name)]
    alias = f"{readable[0] if len(readable) == 1 else symbol}_v"
    return {name: alias for name in names}


def use_variant(text, sources, symbol):
    """This file's own view of a symbol, under another C name so it cannot clash with the header."""
    label = address_of(sources, symbol)
    if not label:
        return None
    names = ({symbol} | sources.aliases.get(symbol, set()))
    edits = []
    # A declaration inside a function gets the label too, or the rename leaves it naming nothing.
    inner = [(m.start(), m.end(), text[m.start():m.end()]) for m in INNER_EXTERN.finditer(pe.mask(text))
             if not is_file_scope(text, m.start())]
    for start, end, statement in sorted(declarations(text) + inner):
        found = pe.declared_names(statement) or []
        if len(found) == 1 and found[0] in names and not QUALIFIED.search(statement):
            edits.append((start, end, statement.rstrip(" ;\n\t") + f' __asm__("{label}");'))
    if not edits:
        return None
    for start, end, new in reversed(edits):
        text = text[:start] + new + text[end:]
    return mt.rename_code(text, variant_names(sources, symbol))


def edit_file(text, sources, rel, symbols, header, keep=(), variants=()):
    """The file with these symbols' lines gone and the header included.

    keep: symbols whose line stays. variants: symbols this file reaches through its own alias.
    """
    for symbol in sorted(variants):
        text = use_variant(text, sources, symbol) or text
    spans = []
    for start, end, statement in declarations(text):
        if not is_file_scope(text, start):
            continue
        names = pe.declared_names(statement) or []
        if len(names) != 1:
            continue
        symbol = sources.symbol(names[0])
        # The defining file keeps its forward declarations: its own types may not fit the header.
        if rel in sources.defined.get(symbol, ()):
            continue
        if symbol in symbols and symbol not in keep and not QUALIFIED.search(statement):
            stop = pe.trailing_comment_end(text, end)
            if not pe.PROVENANCE.search(text[start:stop]):
                spans.append((start, stop))
    if not spans:
        return text
    parts, last = [], 0
    for start, stop in spans:
        parts.append(text[last:start])
        last = stop
    parts.append(text[last:])
    new = re.sub(r"\n{3,}", "\n\n", "".join(parts))
    if f'#include "{header}"' in new:
        return new
    first = re.search(r"^#include [^\n]*\n(?:#include [^\n]*\n)*", new, re.M)
    at = first.end() if first else 0
    return new[:at] + f'#include "{header}"\n' + new[at:]


def write_header(header, rows):
    path = ROOT / "include" / header
    path.parent.mkdir(parents=True, exist_ok=True)
    original = path.read_text() if path.exists() else None
    names = {name: text for name, text in rows.values() if text}
    path.write_text(mp.merge_header(path, header, names))
    return original


def restore_header(header, original):
    path = ROOT / "include" / header
    if original is None:
        path.unlink(missing_ok=True)
    else:
        path.write_text(original)


def includers(sources, header, names):
    """Files that take this header already and use one of these names: the new declaration reaches them."""
    found = []
    for rel, text in sources.text.items():
        if f'"{header}"' in text and set(pe.WORD.findall(pe.without_comments(text))) & names:
            found.append(rel)
    return found


def check_all(files):
    with concurrent.futures.ThreadPoolExecutor(WORKERS) as pool:
        results = list(pool.map(fb.check, files))
    return {rel: result for rel, result in zip(files, results) if result[0] or result[2]}


def land(sources, header, top=None):
    """(symbols moved, lines removed, symbols rejected)."""
    rejected, kept, skipped = set(), collections.defaultdict(set), set()
    variants = collections.defaultdict(set)
    for _ in range(12):
        rows = dict(sources.plan(rejected).get(header, {}))
        if top:
            count = collections.Counter({s: len(sources.lines[s]) for s in rows})
            rows = {s: rows[s] for s, _ in count.most_common(top)}
        symbols = set(rows)
        changed = {}
        for rel in sources.files:
            if rel in skipped:
                continue
            new = edit_file(sources.text[rel], sources, rel, symbols, header, kept[rel], variants[rel] & symbols)
            if new != sources.text[rel]:
                changed[rel] = new
        original = write_header(header, rows)
        for rel, new in changed.items():
            (ROOT / rel).write_text(new, errors="surrogateescape")
        fragment_edits = rename_in_fragments(sources, variants, symbols)
        names = {row[0] for row in rows.values()} | {s for s in symbols}
        affected = sorted(set(changed) | set(includers(sources, header, names)))
        failed = check_all(affected)
        print(f"  {len(symbols)} symbols, {len(changed)} files, {len(affected)} checked, {len(failed)} failed", flush=True)
        if not failed:
            lines = sum(sources.text[r].count("\n") - changed[r].count("\n") for r in changed)
            aliases = sum(len(v & symbols) for v in variants.values())
            return f"{len(symbols)} symbols, {lines} lines gone, {aliases} file aliases, {len(rejected)} rejected: {sorted(rejected)[:12]}"
        for rel in sorted(failed):
            # GCC names a type conflict: no need to bisect.
            clash = {sources.symbol(name) for name in failed[rel][1]} & set(rows)
            if clash:
                for symbol in clash:
                    # The defining file cannot rename its own definition.
                    # So does a file that still clashes with its alias: the clash is somewhere else.
                    if rel in sources.defined.get(symbol, ()) or not address_of(sources, symbol) or symbol in variants[rel]:
                        rejected.add(symbol)
                    else:
                        variants[rel].add(symbol)
                print(f"  {rel}: clash {sorted(clash)[:6]}", flush=True)
                continue
            found = culprits(sources, rel, header, rows, kept[rel], variants[rel])
            if found is None:
                skipped.add(rel)
                print(f"  skip {rel}", flush=True)
                continue
            for symbol in found:
                mine = [row for row in sources.lines[symbol] if row[0] == rel]
                if mine and all(shape(row[2]) is None for row in mine):
                    kept[rel].add(symbol)
                elif mine and address_of(sources, symbol):
                    variants[rel].add(symbol)
                else:
                    rejected.add(symbol)
            print(f"  {rel}: {sorted(found)[:6]}", flush=True)
        for rel in changed:
            (ROOT / rel).write_text(sources.text[rel], errors="surrogateescape")
        for path, old in fragment_edits:
            path.write_text(old, errors="surrogateescape")
        restore_header(header, original)
    raise SystemExit("did not settle")


def without_names(header_text, names):
    """The header without these names' declarations."""
    lines = []
    for line in header_text.split("\n"):
        found = pe.declared_names(line) if line.startswith("extern ") else None
        if not (found and set(found) & names):
            lines.append(line)
    return "\n".join(lines)


def rename_in_fragments(sources, variants, symbols):
    """A file's fragments are spliced into it, so they follow its aliases. Returns [(path, old text)] to undo."""
    edits = []
    for rel, mine in variants.items():
        if not mine & symbols:
            continue
        for path in fb.fragments_of(rel):
            old = path.read_text(errors="surrogateescape")
            new = old
            for symbol in sorted(mine & symbols):
                # A fragment's own declaration needs the label as well as the new name.
                new = use_variant(new, sources, symbol) or mt.rename_code(new, variant_names(sources, symbol))
            if new != old:
                edits.append((path, old))
                path.write_text(new, errors="surrogateescape")
    return edits


def culprits(sources, rel, header, rows, kept, variants):
    """The symbols this file cannot accept, or None if it fails with none of them moved."""
    path = ROOT / rel
    original_header = (ROOT / "include" / header).read_text()
    mine = {s for s in rows if any(row[0] == rel for row in sources.lines.get(s, ()))} - set(kept) - set(variants)
    # A symbol that was in the header before this run stays there whatever this file does.
    movable = {s for s in mine if rows[s][1]}

    def trial(group):
        keep = movable - set(group)
        reduced = {s: r for s, r in rows.items() if s not in keep and s not in kept}
        path.write_text(edit_file(sources.text[rel], sources, rel, set(reduced), header, kept, set(variants) & set(rows)),
                        errors="surrogateescape")
        (ROOT / "include" / header).write_text(without_names(original_header, {rows[s][0] for s in keep}))
        bad, _, failed = fb.check(rel)
        return not bad and not failed

    try:
        if not trial(set()):
            return None
        return movable - fb.accepted_functions(movable, trial)
    finally:
        (ROOT / "include" / header).write_text(original_header)
        path.write_text(sources.text[rel], errors="surrogateescape")


def split_shared_statements(text):
    """`extern float a(int), b(float);` as one statement each. (new text, statements split)."""
    out, last, count = [], 0, 0
    for start, end, statement in pe.find_externs(text):
        if QUALIFIED.search(statement) or not is_file_scope(text, start):
            continue
        names = pe.declared_names(statement) or []
        if len(names) < 2 or "(" not in statement:
            continue
        m = re.fullmatch(rf"extern\s+([\w \t]*?\s+\*?)\s*{re.escape(names[0])}\b(.*);", statement, re.S)
        parts = pe.split_declarators(mp.normal(statement).rstrip(";")) if m else []
        if not m or len(parts) != len(names) or "*" in m.group(1):
            continue
        head = m.group(1).strip()
        first = re.split(rf"\b{re.escape(names[0])}\b", parts[0], maxsplit=1)[1]
        lines = [f"extern {head} {names[0]}{first};"]
        lines += [f"extern {head} {part.strip()};" for part in parts[1:]]
        out += [text[last:start], "\n".join(lines)]
        last = end
        count += 1
    out.append(text[last:])
    return "".join(out), count


def show_plan(sources):
    headers = sources.plan()
    total = 0
    for header, rows in sorted(headers.items(), key=lambda kv: -sum(len(sources.lines[s]) for s in kv[1])):
        count = sum(len([r for r in sources.lines[s] if r[0] not in sources.defined.get(s, ())]) for s in rows)
        total += count
        print(f"{count:6} lines {len(rows):5} symbols  {header}")
    print(f"{total} lines could move; blocked symbols: {dict(collections.Counter(sources.blocked.values()))}")


def main():
    args = sys.argv[1:]
    if "--split" in args:
        files = subprocess.run(["git", "ls-files", "src/*.c"], cwd=ROOT, capture_output=True, text=True).stdout.split()
        total = 0
        for rel in files:
            path = ROOT / rel
            old = path.read_text(errors="surrogateescape")
            new, count = split_shared_statements(old)
            if count:
                path.write_text(new, errors="surrogateescape")
                total += count
        print(f"split {total} statements")
        return
    sources = Sources()
    if "--plan" in args:
        show_plan(sources)
        return
    header = args[args.index("--header") + 1]
    top = int(args[args.index("--top") + 1]) if "--top" in args else None
    if "--write" not in args:
        rows = sources.plan().get(header, {})
        print(f"{header}: {len(rows)} symbols")
        for symbol, (name, text) in sorted(rows.items(), key=lambda kv: -len(sources.lines[kv[0]]))[:top or 20]:
            print(f"{len(sources.lines[symbol]):5} {text or '(already in the header) ' + name}")
        return
    print(land(sources, header, top))


if __name__ == "__main__":
    main()
