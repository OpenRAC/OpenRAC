# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""What the whole program defines: every function, its signature, its address.

A call is translated against the callee's definition, not against what the
calling file declared (the decompilation's files often declare a callee
differently from its definition: other argument types, another order of
float and integer arguments, fewer arguments). So every unit is read once
first, to know each definition (index_unit), before any is translated.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

import ctype

# How a game's decompilation names code and data by address, as regular
# expressions with an `addr` group (hexadecimal) and, for code in a level's
# program, an `overlay` group (decimal). These are rac1/pal's (func_00123456,
# func_L05_00123456, D_00123456, with an optional _suffix for a variant); a
# game's hostgen.json gives its own under "names".
DEFAULT_CODE_NAMES = [r"^func_(?:L(?P<overlay>\d\d)_)?(?P<addr>[0-9A-Fa-f]{6,8})(?:_\w*)?$"]
DEFAULT_DATA_NAMES = [r"^(?:D|func)_(?:L(?P<overlay>\d\d)_)?(?P<addr>[0-9A-Fa-f]{6,8})(?:_\w*)?$"]

ASM_MARK = "openrac_asm_"

# Helpers whose bodies are the decompilation's inline assembly
# (include/common.h): the runtime has host versions (guest.h).
BUILTINS = {
    "qcopy": "openrac_qcopy",
    "qcopy_nc": "openrac_qcopy",
    "qzero": "openrac_qzero",
}


def cname(symbol: str) -> str:
    """The host C name of a game function. Names the game shares with the C
    library or the host's own program (memcpy, sprintf, main, _start) get a
    prefix; func_ names stay as they are."""
    if symbol.startswith("func_"):
        return symbol
    return f"game_{symbol}"


@dataclass
class Signature:
    ret: ctype.Type
    params: tuple
    variadic: bool = False
    prototyped: bool = True

    @staticmethod
    def of(t: ctype.Type) -> "Signature":
        if not isinstance(t, ctype.Func):
            raise ctype.TypeError_(f"not a function type: {t}")
        return Signature(t.ret, tuple(t.params), t.variadic, t.prototyped)


@dataclass
class Function:
    symbol: str                 # the linker name (asm label if any)
    sig: Signature
    unit: str | None = None     # where it is defined; None: not defined in C
    static: bool = False
    asm: bool = False           # still assembly in the decompilation
    declared: list = field(default_factory=list)  # signatures seen in declarations


@dataclass
class Program:
    functions: dict[str, Function] = field(default_factory=dict)
    statics: dict[tuple[str, str], Function] = field(default_factory=dict)
    places: dict[str, list[tuple[int, int]]] = field(default_factory=dict)  # symbol -> (overlay, addr)
    symbols: dict[str, int] = field(default_factory=dict)  # other named globals -> address
    typedefs: dict[str, set] = field(default_factory=dict)  # name -> desugared type strings
    host: set = field(default_factory=set)  # functions the port writes by hand (port/game/<game>/)
    wrap: set = field(default_factory=set)  # translated as <name>__game, called by a host <name>
    aliases: dict = field(default_factory=dict)  # (unit, name) -> symbol, from labels Clang refused
    by_place: dict = field(default_factory=dict)  # (overlay, address) -> the symbol defined there
    groups: list = field(default_factory=list)  # [{"path", "suffix", "overlay"}]: separate programs
    code_names: list = field(default_factory=lambda: [re.compile(p) for p in DEFAULT_CODE_NAMES])
    data_names: list = field(default_factory=lambda: [re.compile(p) for p in DEFAULT_DATA_NAMES])

    def set_names(self, code: list[str] | None, data: list[str] | None) -> None:
        if code:
            self.code_names = [re.compile(p) for p in code]
        if data:
            self.data_names = [re.compile(p) for p in data]

    def lookup(self, unit: str, symbol: str) -> Function | None:
        return self.statics.get((unit, symbol)) or self.functions.get(symbol)

    def group_of(self, unit: str) -> dict | None:
        """The group (a separate program, such as a menu loaded over part of
        the executable) that a unit belongs to, if any."""
        for g in self.groups:
            if unit == g["path"] or unit.startswith(g["path"].rstrip("/") + "/"):
                return g
        return None

    def grouped(self, unit: str, symbol: str) -> str:
        """The symbol a definition in unit has: its group's own, if in one."""
        g = self.group_of(unit)
        return f"{symbol}__{g['suffix']}" if g else symbol

    def code_places(self, symbol: str) -> list[tuple[int, int]]:
        """Where a function sits: (overlay, address) pairs; overlay -1 is the executable."""
        for g in self.groups:
            tail = "__" + g["suffix"]
            if symbol.endswith(tail):
                return [(g["overlay"], addr) for _, addr in self.code_places(symbol[:-len(tail)])]
        named: list[tuple[int, int]] = []
        for pattern in self.code_names:
            m = pattern.match(symbol)
            if m:
                overlay = m.groupdict().get("overlay")
                named = [(int(overlay) if overlay is not None else -1, int(m.group("addr"), 16))]
                break
        if not named and symbol in self.symbols:
            named = [(-1, self.symbols[symbol])]
        if symbol in self.places:
            # A function of the executable that the levels' programs also
            # carry is in the catalogue with its level places only: it keeps
            # its place in the executable too, first, as its own address.
            exe = [p for p in named if p[0] < 0 and p not in self.places[symbol]]
            return exe + self.places[symbol]
        return named

    def data_address(self, symbol: str) -> int | None:
        if symbol in self.symbols:
            return self.symbols[symbol]
        for pattern in self.data_names + self.code_names:
            m = pattern.match(symbol)
            if m:
                return int(m.group("addr"), 16)
        return None

    def index_places(self) -> None:
        """Every function with C (or the port's own) by where it sits, so that
        two spellings of one address (FUN_0020c828, func_0020C828) meet."""
        self.by_place = {}
        for sym, f in sorted(self.functions.items()):
            if f.unit is not None or sym in self.host:
                for place in self.code_places(sym):
                    self.by_place.setdefault(place, sym)

    def canonical(self, symbol: str) -> str:
        """The symbol with C at the same place as symbol, or symbol."""
        f = self.functions.get(symbol)
        if f is not None and (f.unit is not None or symbol in self.host):
            return symbol
        for place in self.code_places(symbol):
            other = self.by_place.get(place)
            if other is not None:
                return other
        return symbol

    def stubs(self) -> list[Function]:
        """Functions called or declared but defined nowhere in C, nor by the host."""
        return sorted((f for f in self.functions.values() if f.unit is None and f.symbol not in self.host),
                      key=lambda f: f.symbol)


def parse_type(node: dict, desugared: bool = True) -> ctype.Type:
    t = node["type"]
    text = t.get("desugaredQualType", t["qualType"]) if desugared else t["qualType"]
    return ctype.parse(text)


def symbol_of(decl: dict) -> str:
    return decl.get("mangledName") or decl.get("name", "")


def has_body(decl: dict) -> bool:
    return any(c.get("kind") == "CompoundStmt" for c in decl.get("inner", ()))


def index_unit(program: Program, unit: str, ast: dict) -> list[str]:
    """Adds one unit's definitions and declarations. Returns problems found."""
    problems = []
    for decl in ast.get("inner", ()):
        kind = decl.get("kind")
        if kind == "TypedefDecl":
            t = decl["type"]
            text = t.get("desugaredQualType", t["qualType"])
            for c in decl.get("inner", ()):
                owned = c.get("ownedTagDecl")
                if owned and not owned.get("name"):
                    # A typedef of an unnamed enum or record is named after
                    # itself; what matters for a signature is which it is.
                    text = ("enum " if owned.get("kind") == "EnumDecl" else "struct ") + decl["name"]
            program.typedefs.setdefault(decl["name"], set()).add(text)
            continue
        if kind == "VarDecl" and decl.get("name", "").startswith(ASM_MARK):
            name = program.grouped(unit, decl["name"][len(ASM_MARK):])
            f = program.functions.get(name)
            if f is None:
                program.functions[name] = Function(name, Signature(ctype.Base("int"), (), False, False), asm=True)
            else:
                f.asm = True
            continue
        if kind != "FunctionDecl":
            continue
        sym = symbol_of(decl)
        if sym in BUILTINS or sym.startswith("__builtin") or sym == "openrac_hostgen_asm":
            continue
        if decl.get("isImplicit") and not (sym.startswith("func_") or sym in program.functions):
            # Clang's own declaration of a library builtin (fabsf, memcpy): only a
            # function the game itself names is indexed from an implicit declaration.
            if not any(p.match(sym) for p in program.code_names):
                continue
        try:
            sig = Signature.of(parse_type(decl))
        except ctype.TypeError_ as e:
            problems.append(f"{sym}: {e}")
            continue
        static = decl.get("storageClass") == "static"
        if has_body(decl) and not static:
            sym = program.grouped(unit, sym)
        if has_body(decl):
            from_main = decl.get("_file", "").endswith(unit) or not static
            if static:
                if not from_main and not decl.get("_file", "").endswith(".c"):
                    # A static function from a header is translated in every unit that has it.
                    pass
                program.statics[(unit, sym)] = Function(sym, sig, unit, static=True)
                continue
            f = program.functions.get(sym)
            if f is not None and f.unit is not None and f.unit != unit:
                problems.append(f"{sym} is defined in {f.unit} and in {unit}; keeping the first")
                continue
            declared = f.declared if f else []
            program.functions[sym] = Function(sym, sig, unit, asm=f.asm if f else False, declared=declared)
        else:
            f = program.functions.get(sym)
            if f is None:
                f = program.functions[sym] = Function(sym, sig)
            f.declared.append(sig)
            if f.unit is None:
                f.sig = best_declaration(f.declared)
    return problems


def best_declaration(sigs: list[Signature]) -> Signature:
    """For a function defined nowhere in C: the declaration that says most."""
    proto = [s for s in sigs if s.prototyped]
    if not proto:
        return sigs[0]
    return max(proto, key=lambda s: len(s.params))


def read_places(path: Path, program: Program) -> int:
    """Each level function's address in each level that has it
    (config/overlays/functions.tsv of rac1/pal: name, kind, size,
    fingerprint, levels, places as level:address,...)."""
    if not path.exists():
        return 0
    count = 0
    for line in path.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        cols = line.split("\t")
        if len(cols) < 6:
            continue
        places = []
        for item in cols[5].split(","):
            level, _, addr = item.partition(":")
            if level and addr:
                places.append((int(level), int(addr, 16)))
        if places:
            program.places[cols[0]] = places
            count += 1
    return count


def merge(into: Program, part: Program) -> list[str]:
    """Adds one unit's index (made in another process) to the program's."""
    problems = []
    for sym, f in part.functions.items():
        have = into.functions.get(sym)
        if have is None:
            into.functions[sym] = f
            continue
        have.asm = have.asm or f.asm
        if f.unit is not None:
            if have.unit is not None and have.unit != f.unit:
                problems.append(f"{sym} is defined in {have.unit} and in {f.unit}; keeping the first")
                continue
            declared = have.declared + f.declared
            into.functions[sym] = Function(sym, f.sig, f.unit, asm=have.asm, declared=declared)
        else:
            have.declared.extend(f.declared)
            if have.unit is None and have.declared:
                have.sig = best_declaration(have.declared)
    into.statics.update(part.statics)
    into.aliases.update(part.aliases)
    if part.groups and not into.groups:
        into.groups = part.groups
    for name, types in part.typedefs.items():
        into.typedefs.setdefault(name, set()).update(types)
    return problems


def canon(t: ctype.Type, typedefs: dict | None = None, depth: int = 0) -> str | None:
    """A type from a definition's signature, as the shared prototypes write it."""
    if isinstance(t, ctype.Ptr):
        return "gaddr"
    if not isinstance(t, ctype.Base):
        return None
    name = t.name
    if name in ("va_list", "__builtin_va_list", "__gnuc_va_list"):
        return "va_list"
    if name.startswith("enum "):
        return "int"
    if name in ctype.INT_NAMES or name in ctype.FLOAT_NAMES or name in ("void", "double", "long double"):
        return name
    if typedefs and name in typedefs and depth < 16:
        seen = set()
        for text in typedefs[name]:
            try:
                seen.add(canon(ctype.parse(text), typedefs, depth + 1))
            except ctype.TypeError_:
                seen.add(None)
        if len(seen) == 1:
            return seen.pop()
    return None


_SPLAT_SYMBOL = re.compile(r"^\s*([A-Za-z_][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")


def read_symbols(path: Path, program: Program) -> int:
    """Named symbols and their addresses from a splat symbol file
    (`name = 0x00123456; // comment`)."""
    if not path.exists():
        return 0
    count = 0
    for line in path.read_text(errors="replace").splitlines():
        m = _SPLAT_SYMBOL.match(line)
        if m:
            program.symbols.setdefault(m.group(1), int(m.group(2), 16))
            count += 1
    return count

