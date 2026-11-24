# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""VU0 macro-mode inline assembly in matched C, written as C on locals.

The decompilations' matched C keeps the vector unit's instructions as GCC inline assembly:
one instruction a statement on C variables of GCC's `j` register class (`__asm__("vadd.xyz %0,
%1, %2" : "=j"(x) : "j"(x), "j"(y))`), or a block on named registers that loads and stores
through its operands (`lqc2 $vf1, 0(%1)` ... `sqc2 $vf1, 0(%0)`). Neither runs on the host. Here
each such statement becomes C (lower.py, Unit.asm_stmt):

- a `%N` operand where an instruction names a vector register is that operand's sixteen bytes in
  a local of four floats, read before the first instruction and written back after the last, as
  GCC's register for it was; `j` constraints reach Clang as `m` (prep.py), so the operand is the
  C variable itself;
- a named register (`$vf1`, ACC, Q, the integer registers `vi`) is a field of the function's own
  `openrac_vu0_state`, a local of the function hostgen writes: what one statement leaves there the
  next one in the function reads, and nothing outside it sees it. No global vector unit is
  involved, so a statement that reads a named register no statement of its function wrote first
  (a value a hand-written routine left for its caller) is not written as C: the function stays
  the assembly's translation;
- the integer registers a block uses (`$at` for an `mfc1` ... `qmtc2` pair) are locals of the
  block; `$4` ... `$11` read before the block writes them are the function's arguments in those
  registers, when the statement is the first of the function;
- the arithmetic is asmc.py's translation of the same instructions (Translator.vu0), so a
  statement's C and the assembly's C of the same instructions compute alike (rounding toward
  zero, vf0, the broadcast fields, asm.vu_flush).

`lower_statement` returns the C, or raises Vu0Unsupported with the reason. NotVu0 means the
statement has no vector-unit instruction (a barrier, `di`, an FPU move): lower.py keeps its
own handling of it.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

_GPR_NAMES = {
    "zero": 0, "at": 1, "v0": 2, "v1": 3, "a0": 4, "a1": 5, "a2": 6, "a3": 7,
    **{f"t{i}": 8 + i for i in range(8)}, **{f"s{i}": 16 + i for i in range(8)},
    "t8": 24, "t9": 25, "k0": 26, "k1": 27, "gp": 28, "sp": 29, "fp": 30, "s8": 30, "ra": 31,
}
_LANES = "xyzw"

# Arithmetic with a vector or broadcast second source: the result to fd, or to ACC (an "a").
_ARITH = ("add", "sub", "mul", "max", "mini", "madd", "msub")
_ARITH_ACC = ("add", "sub", "mul", "madd", "msub")
_MOVES = ("vmove", "vmr32", "vabs", "vitof0", "vitof4", "vitof12", "vitof15",
          "vftoi0", "vftoi4", "vftoi12", "vftoi15")
_NOTHING = ("nop", "sync", "sync.l", "sync.p", "vnop", "vwaitq", "ssnop")
# Instructions of the vector unit (or moving to and from it): a statement with one is a VU0 block.
_VU_OPS = re.compile(r"^(lqc2|sqc2|qmtc2|qmfc2|cfc2|ctc2|v[a-z0-9]+)$")


class NotVu0(Exception):
    """The statement has no vector-unit instruction."""


class Vu0Unsupported(Exception):
    """A VU0 statement this module does not write as C (the reason)."""


@dataclass
class Operand:
    """One operand of the statement, as lower.py has it, in GCC's numbering (outputs first)."""

    k: int
    constraint: str            # the constraint as the prepared copy spells it ("=m", "+m", "r", "0", "f")
    output: bool
    ctype: str = "int"         # lower.py's class of its type: int, ptr, float, rec, arr ...
    host_type: str = "int"     # its type as host C
    lv_text: str | None = None  # an lvalue: its host C
    lv_addr: str | None = None  # an lvalue in game memory: its address as host C
    value: str | None = None    # an input: its value as host C


@dataclass
class FunctionState:
    """What the statements already written in one function left in its named registers."""

    vf: dict[int, int] = field(default_factory=lambda: {0: 15})  # initialized component masks
    acc: int = 0               # initialized accumulator components
    q: bool = False
    vi: set = field(default_factory=lambda: {0})
    used: bool = False         # the function needs its openrac_vu0_state


@dataclass
class _Ins:
    op: str                    # asmc's name (asm.py's decoder): vaddbc, vmulabc, vadd, lqc2 ...
    text: str
    dest: int = 15
    fd: int = 0
    fs: int = 0
    ft: int = 0
    bc: int = 0
    ftf: int = 0
    rt: int = 0
    rd: int = 0
    sa: int = 0
    mem: tuple | None = None   # lqc2/sqc2/lq/sq: ("op", k, offset) | ("gpr", n, offset)


def is_vu0(text: str) -> bool:
    """Whether an asm statement's instructions include one of the vector unit's."""
    return any(_VU_OPS.match(op.partition(".")[0]) for op, _ in _lines(text))


def operand_in_register(text: str) -> bool:
    """Whether an operand (%N) of the statement is a vector register: GCC's "j" class, which
    Clang does not know (the statement reaches it with "m", prep.py)."""
    for op, rest in _lines(text):
        args = [a.strip() for a in rest.split(",")] if rest else []
        base = op.partition(".")[0]
        if base in ("lqc2", "sqc2"):
            regs = args[:1]
        elif base in ("qmtc2", "qmfc2"):
            regs = args[1:2]
        elif base.startswith("v"):
            regs = args
        else:
            continue
        if any(re.fullmatch(r"%\d+[xyzw]?", a) for a in regs):
            return True
    return False


def _lines(text: str) -> list[tuple[str, str]]:
    out = []
    for raw in re.split(r"[\n;]", text):
        raw = raw.split("#", 1)[0].strip()
        if not raw or raw.endswith(":"):
            continue
        parts = raw.split(None, 1)
        out.append((parts[0].lower(), parts[1].strip() if len(parts) > 1 else ""))
    return out


def _vf(tok: str, n_ops: int) -> tuple[int, int | None]:
    """A vector register operand: its number (32 + N for operand %N) and a broadcast field."""
    m = re.fullmatch(r"\$?vf(\d+)([xyzw])?", tok)
    if m:
        return int(m.group(1)), (_LANES.index(m.group(2)) if m.group(2) else None)
    m = re.fullmatch(r"%(\d+)([xyzw])?", tok)
    if m and int(m.group(1)) < n_ops:
        return 32 + int(m.group(1)), (_LANES.index(m.group(2)) if m.group(2) else None)
    raise Vu0Unsupported(f"vector register operand {tok!r}")


def _gpr(tok: str, n_ops: int) -> int:
    """An integer register operand: its number (32 + N for operand %N)."""
    m = re.fullmatch(r"\$(\d+)", tok)
    if m and int(m.group(1)) < 32:
        return int(m.group(1))
    m = re.fullmatch(r"\$([a-z0-9]+)", tok)
    if m and m.group(1) in _GPR_NAMES:
        return _GPR_NAMES[m.group(1)]
    m = re.fullmatch(r"%z?(\d+)", tok)
    if m and int(m.group(1)) < n_ops:
        return 32 + int(m.group(1))
    raise Vu0Unsupported(f"integer register operand {tok!r}")


def _mem(tok: str, n_ops: int) -> tuple:
    m = re.fullmatch(r"%(\d+)", tok)
    if m and int(m.group(1)) < n_ops:
        return ("op", int(m.group(1)), 0)
    m = re.fullmatch(r"(-?(?:0[xX][0-9A-Fa-f]+|\d+))?\(\s*([^)]+?)\s*\)", tok)
    if m:
        off = int(m.group(1), 0) if m.group(1) else 0
        base = m.group(2)
        b = re.fullmatch(r"%(\d+)", base)
        if b and int(b.group(1)) < n_ops:
            return ("base", 32 + int(b.group(1)), off)
        return ("base", _gpr(base, n_ops), off)
    raise Vu0Unsupported(f"memory operand {tok!r}")


def _dest(suffix: str) -> int:
    if not suffix:
        return 15
    if not re.fullmatch(r"x?y?z?w?", suffix):
        raise Vu0Unsupported(f"field mask .{suffix}")
    return sum(8 >> _LANES.index(c) for c in suffix)


def parse(text: str, n_ops: int) -> list[_Ins]:
    """The statement's instructions, with operands as register numbers (operand %N of a vector or
    integer register as 32 + N)."""
    out = []
    for op, rest in _lines(text):
        args = [a.strip() for a in rest.split(",")] if rest else []
        if op == ".set":
            continue  # .set noat, .set noreorder ...
        if op.startswith("."):
            raise Vu0Unsupported(f"{op} {rest}")  # .word: an instruction as its encoding
        if op in _NOTHING:
            continue
        base, _, suffix = op.partition(".")
        if base in ("qmtc2", "qmfc2", "cfc2", "ctc2"):
            # .ni / .i: whether to wait for a microprogram; none runs here.
            if len(args) != 2:
                raise Vu0Unsupported(f"{op} {rest}")
            if base in ("cfc2", "ctc2"):
                m = re.fullmatch(r"\$?vi(\d+)|\$(\d+)", args[1])
                if not m:
                    raise Vu0Unsupported(f"{op} {rest}")
                rd = int(m.group(1) or m.group(2))
                # Only integer registers and Q have tracked values in the function-local state.
                if rd >= 16 and rd != 22:
                    raise Vu0Unsupported(f"{base} of untracked control register {rd}")
            else:
                rd = _vf(args[1], n_ops)[0]
            out.append(_Ins(base, op, rt=_gpr(args[0], n_ops), rd=rd))
            continue
        if base in ("lqc2", "sqc2"):
            if len(args) != 2:
                raise Vu0Unsupported(f"{op} {rest}")
            out.append(_Ins(base, op, ft=_vf(args[0], n_ops)[0], mem=_mem(args[1], n_ops)))
            continue
        if base in ("lq", "sq"):
            if len(args) != 2:
                raise Vu0Unsupported(f"{op} {rest}")
            out.append(_Ins(base, op, rt=_gpr(args[0], n_ops), mem=_mem(args[1], n_ops)))
            continue
        if base in ("mfc1", "mtc1"):
            m = re.fullmatch(r"%(\d+)", args[1]) if len(args) == 2 else None
            if m is None or int(m.group(1)) >= n_ops:
                raise Vu0Unsupported(f"{op} {rest}")
            out.append(_Ins(base, op, rt=_gpr(args[0], n_ops), fs=int(m.group(1))))
            continue
        if base in ("dsll32", "dsrl32", "dsra32", "dsll", "dsrl", "dsra"):
            if len(args) != 3:
                raise Vu0Unsupported(f"{op} {rest}")
            out.append(_Ins(base, op, rd=_gpr(args[0], n_ops), rt=_gpr(args[1], n_ops), sa=int(args[2], 0)))
            continue
        if not base.startswith("v"):
            raise Vu0Unsupported(f"{op}")
        dest = _dest(suffix)
        if base in ("vcallms", "vcallmsr"):
            raise Vu0Unsupported("a VU0 microprogram (vcallms)")
        if base in ("vdiv", "vrsqrt", "vsqrt"):
            regs = [a for a in args if a.upper() != "Q"]
            if base == "vsqrt" and len(regs) == 1:
                ft, ftf = _vf(regs[0], n_ops)
                out.append(_Ins(base, op, ft=ft, ftf=ftf or 0))
                continue
            if len(regs) != 2:
                raise Vu0Unsupported(f"{op} {rest}")
            (fs, fsf), (ft, ftf) = _vf(regs[0], n_ops), _vf(regs[1], n_ops)
            out.append(_Ins(base, op, fs=fs, bc=fsf or 0, ft=ft, ftf=ftf or 0))
            continue
        if base in _MOVES:
            if len(args) != 2:
                raise Vu0Unsupported(f"{op} {rest}")
            out.append(_Ins(base, op, dest=dest, ft=_vf(args[0], n_ops)[0], fs=_vf(args[1], n_ops)[0]))
            continue
        if base == "vclipw":
            if len(args) != 2:
                raise Vu0Unsupported(f"{op} {rest}")
            out.append(_Ins(base, op, dest=dest, fs=_vf(args[0], n_ops)[0], ft=_vf(args[1], n_ops)[0]))
            continue
        if base in ("vopmula", "vopmsub"):
            if len(args) != 3:
                raise Vu0Unsupported(f"{op} {rest}")
            fd = 0 if base == "vopmula" else _vf(args[0], n_ops)[0]
            out.append(_Ins(base, op, dest=dest, fd=fd, fs=_vf(args[1], n_ops)[0], ft=_vf(args[2], n_ops)[0]))
            continue
        m = re.fullmatch(r"v(add|sub|mul|maxi|max|minii|mini|madd|msub)(a?)(x|y|z|w|q|i)?", base)
        if m is None:
            raise Vu0Unsupported(f"{op}")
        kind, to_acc, src = m.groups()
        if kind == "maxi" and not to_acc and src is None:
            kind, src = "max", "i"
        elif kind == "minii":
            kind, src = "mini", "i"
        elif kind == "maxi":
            raise Vu0Unsupported(f"{op}")
        # I is a control-register input, not an implicit zero in a fresh local state.
        if src == "i":
            raise Vu0Unsupported("the untracked immediate scalar I")
        if to_acc and kind not in _ARITH_ACC:
            raise Vu0Unsupported(f"{op}")
        name = f"v{kind}{'a' if to_acc else ''}" + ("bc" if src in tuple(_LANES) else (src or ""))
        if len(args) != 3:
            raise Vu0Unsupported(f"{op} {rest}")
        if to_acc and args[0].upper() != "ACC":
            raise Vu0Unsupported(f"{op} {rest}")
        fd = 0 if to_acc else _vf(args[0], n_ops)[0]
        fs = _vf(args[1], n_ops)[0]
        ins = _Ins(name, op, dest=dest, fd=fd, fs=fs)
        if src in tuple(_LANES):
            ft, b = _vf(args[2], n_ops)
            if b is not None and b != _LANES.index(src):
                raise Vu0Unsupported(f"{op} {rest}")
            ins.ft, ins.bc = ft, _LANES.index(src)
        elif src in ("q", "i"):
            if args[2].upper() != src.upper():
                raise Vu0Unsupported(f"{op} {rest}")
        else:
            ins.ft = _vf(args[2], n_ops)[0]
        out.append(ins)
    return out


def _comps(dest: int) -> list[int]:
    return [c for c in range(4) if dest & (8 >> c)]


def _effects(i: _Ins) -> tuple[set, set, set]:
    """(vector registers read, vector registers written in full, those written in part), the
    accumulator as "acc", Q as "q", the integer registers as ("vi", n), the integer registers of
    the EE as ("r", n)."""
    op, full = i.op, i.dest == 15
    reads, wfull, wpart = set(), set(), set()

    def w(r):
        (wfull if full else wpart).add(r)

    if op in ("lqc2",):
        wfull.add(i.ft)
        if i.mem[0] == "base":
            reads.add(("r", i.mem[1]))
    elif op == "sqc2":
        reads.add(i.ft)
        if i.mem[0] == "base":
            reads.add(("r", i.mem[1]))
    elif op == "lq":
        wfull.add(("r", i.rt))
        if i.mem[0] == "base":
            reads.add(("r", i.mem[1]))
    elif op == "sq":
        reads.add(("r", i.rt))
        if i.mem[0] == "base":
            reads.add(("r", i.mem[1]))
    elif op == "qmtc2":
        reads.add(("r", i.rt))
        wfull.add(i.rd)
    elif op == "qmfc2":
        reads.add(i.rd)
        wfull.add(("r", i.rt))
    elif op == "cfc2":
        if i.rd < 16:
            reads.add(("vi", i.rd))
        elif i.rd == 22:
            reads.add("q")
        wfull.add(("r", i.rt))
    elif op == "ctc2":
        reads.add(("r", i.rt))
        if i.rd < 16:
            wfull.add(("vi", i.rd))
        elif i.rd == 22:
            wfull.add("q")
    elif op == "mfc1":
        wfull.add(("r", i.rt))
    elif op == "mtc1":
        reads.add(("r", i.rt))
    elif op.startswith("ds"):
        reads.add(("r", i.rt))
        wfull.add(("r", i.rd))
    elif op in ("vdiv", "vrsqrt"):
        reads |= {i.fs, i.ft}
        wfull.add("q")
    elif op == "vsqrt":
        reads.add(i.ft)
        wfull.add("q")
    elif op in _MOVES:
        reads.add(i.fs)
        w(i.ft)
    elif op == "vclipw":
        reads |= {i.fs, i.ft}
    elif op == "vopmula":
        reads |= {i.fs, i.ft}
        wpart.add("acc")
    elif op == "vopmsub":
        reads |= {i.fs, i.ft, "acc"}
        w(i.fd)
    else:
        m = re.fullmatch(r"v(add|sub|mul|max|mini|madd|msub)(a?)(bc|q|i)?", op)
        kind, to_acc, src = m.groups()
        reads.add(i.fs)
        if src == "q":
            reads.add("q")
        elif src != "i":
            reads.add(i.ft)
        if kind in ("madd", "msub"):
            reads.add("acc")
        if to_acc:
            (wfull if full else wpart).add("acc")
        else:
            w(i.fd)
    return reads, wfull, wpart


def _component_reads(i: _Ins) -> dict[int | str, int]:
    """The vector and accumulator components actually read, including broadcasts and rotations."""
    reads: dict[int | str, int] = {}

    def add(register: int | str, mask: int) -> None:
        if mask:
            reads[register] = reads.get(register, 0) | mask

    if i.op == "sqc2":
        add(i.ft, 15)
    elif i.op == "qmfc2":
        add(i.rd, 15)
    elif i.op in ("vdiv", "vrsqrt"):
        add(i.fs, 8 >> i.bc)
        add(i.ft, 8 >> i.ftf)
    elif i.op == "vsqrt":
        add(i.ft, 8 >> i.ftf)
    elif i.op in _MOVES:
        mask = i.dest
        if i.op == "vmr32":
            mask = sum(8 >> ((c + 1) % 4) for c in _comps(i.dest))
        add(i.fs, mask)
    elif i.op == "vclipw":
        add(i.fs, 14)
        add(i.ft, 1)
    elif i.op in ("vopmula", "vopmsub"):
        components = [c for c in _comps(i.dest) if c < 3]
        add(i.fs, sum(8 >> ((c + 1) % 3) for c in components))
        add(i.ft, sum(8 >> ((c + 2) % 3) for c in components))
        if i.op == "vopmsub":
            add("acc", i.dest & 14)
    else:
        arithmetic = re.fullmatch(r"v(add|sub|mul|max|mini|madd|msub)(a?)(bc|q|i)?", i.op)
        if arithmetic:
            kind, _, source = arithmetic.groups()
            add(i.fs, i.dest)
            if source == "bc" and i.dest:
                add(i.ft, 8 >> i.bc)
            elif source not in ("q", "i"):
                add(i.ft, i.dest)
            if kind in ("madd", "msub"):
                add("acc", i.dest)
    return reads


class _Emitter:
    """The pieces of asmc.py's Translator that its vu0() reads, on this statement's locals."""

    vu_flags = False
    ps2 = False

    def __init__(self, vu: str, vu_flush: bool):
        import asmc  # lower.py is imported by asmc.py: import it here, once it is loaded

        self.vu = vu
        self.vu_flush = vu_flush
        self._vu0 = asmc.Translator.vu0
        self._f = asmc.Translator._f.__get__(self)
        self._fn = asmc.Translator._fn.__get__(self)

    def vf(self, n: int, c: int) -> str:
        if n == 0:
            return "openrac_ee_f(0x3F800000u)" if c == 3 else "0.0f"
        if n < 32:
            return f"{self.vu}->vf[{n}][{c}]"
        return f"openrac_v{n - 32}_[{c}]"

    @staticmethod
    def R(n: int) -> str:
        if n == 0:
            return "0"
        return f"openrac_r{n}_" if n < 32 else f"openrac_o{n - 32}_"

    @staticmethod
    def H(n: int) -> str:
        if n == 0:
            return "0"
        return f"openrac_h{n}_" if n < 32 else f"openrac_oh{n - 32}_"

    def W(self, n: int, value: str) -> list[str]:
        return [] if n == 0 else [f"{self.R(n)} = {value};"]

    def vu0(self, i: _Ins) -> list[str]:
        import asm

        ins = asm.Insn(i.op, 0, 0, rt=i.rt, rd=i.rd, fs=i.fs, ft=i.ft, fd=i.fd, dest=i.dest, bc=i.bc, ftf=i.ftf)
        return self._vu0(self, ins)


def lower_statement(text: str, operands: list[Operand], state: FunctionState, *, vu_flush: bool,
                    arg_register=None, vu: str = "openrac_vu_") -> str:
    """The C of one asm statement whose instructions are VU0's (is_vu0), on the operands lower.py
    read (each with its lvalue or value as host C). STATE is the function's: what earlier statements
    left in the named registers, updated here. ARG_REGISTER(n) gives the function's argument in
    integer register n as host C, or None where the statement cannot take it from there."""
    n_ops = len(operands)
    if not is_vu0(text):
        raise NotVu0()
    insns = parse(text, n_ops)
    ops = {o.k: o for o in operands}
    outputs = [o for o in operands if o.output]

    # An input tied to an output ("0") is that output's register.
    alias: dict[int, int] = {}
    for o in operands:
        if not o.output and re.fullmatch(r"\d+", o.constraint.strip()):
            t = int(o.constraint.strip())
            if t >= len(outputs):
                raise Vu0Unsupported(f"operand {o.k} tied to {t}")
            alias[o.k] = t

    def canon(n: int) -> int:
        return 32 + alias.get(n - 32, n - 32) if n >= 32 else n

    for i in insns:
        for f in ("fs", "ft", "fd", "rt", "rd"):
            v = getattr(i, f)
            if v >= 32 and not (i.op in ("cfc2", "ctc2") and f == "rd"):
                if i.op in ("mfc1", "mtc1") and f == "fs":
                    continue
                setattr(i, f, canon(v))
        if i.mem and i.mem[0] == "base" and i.mem[1] >= 32:
            i.mem = ("base", canon(i.mem[1]), i.mem[2])
        if i.mem and i.mem[0] == "op":
            i.mem = ("op", alias.get(i.mem[1], i.mem[1]), i.mem[2])

    # What each operand is to the instructions: a vector register, an integer register, a float
    # register (mfc1 / mtc1), or a place in memory.
    vec, gpr, fpr, mem = set(), set(), set(), set()
    for i in insns:
        reads, wfull, wpart = _effects(i)
        for r in reads | wfull | wpart:
            if isinstance(r, int) and r >= 32:
                vec.add(r - 32)
            elif isinstance(r, tuple) and r[0] == "r" and r[1] >= 32:
                gpr.add(r[1] - 32)
        if i.op in ("mfc1", "mtc1"):
            fpr.add(alias.get(i.fs, i.fs))
        if i.mem and i.mem[0] == "op":
            mem.add(i.mem[1])
    kinds = [vec, gpr, fpr, mem]
    for k in set().union(*kinds):
        if sum(k in s for s in kinds) > 1:
            raise Vu0Unsupported(f"operand {k} used as two kinds of register")
        if k not in ops:
            raise Vu0Unsupported(f"operand {k}")

    # The named registers: what this statement reads that neither it nor an earlier statement
    # of the function wrote is a value from outside the function.
    written_vf, acc, q, vi = dict(state.vf), state.acc, state.q, set(state.vi)
    gpr_written: set[int] = set()
    gpr_args: dict[int, str] = {}
    for i in insns:
        reads, wfull, wpart = _effects(i)
        # A masked write initializes only its selected components. A read of another component
        # still depends on a value from outside this function, not the local state's zero.
        for register, mask in _component_reads(i).items():
            if isinstance(register, int) and register >= 32:
                continue  # an operand's C variable supplies its value
            available = acc if register == "acc" else written_vf.get(register, 0)
            missing = mask & ~available
            if missing:
                name = "ACC" if register == "acc" else f"$vf{register}"
                components = "".join(_LANES[c] for c in _comps(missing))
                raise Vu0Unsupported(f"{name}.{components} read before the function writes it")
        for r in reads:
            if r == "q" and not q:
                raise Vu0Unsupported("Q read before the function writes it")
            if isinstance(r, tuple) and r[0] == "vi" and r[1] not in vi:
                raise Vu0Unsupported(f"$vi{r[1]} read before the function writes it")
            if isinstance(r, tuple) and r[0] == "r" and r[1] < 32 and r[1] != 0 and r[1] not in gpr_written:
                text_ = arg_register(r[1]) if arg_register is not None and 4 <= r[1] <= 11 else None
                if text_ is None:
                    raise Vu0Unsupported(f"${r[1]} read before the statement writes it")
                gpr_args[r[1]] = text_
                gpr_written.add(r[1])
        for r in wfull | wpart:
            if isinstance(r, int) and r < 32:
                mask = i.dest & 14 if i.op == "vopmsub" else (15 if r in wfull else i.dest)
                written_vf[r] = written_vf.get(r, 0) | mask
            elif r == "acc":
                acc |= i.dest & 14 if i.op == "vopmula" else i.dest
            elif r == "q":
                q = True
            elif isinstance(r, tuple) and r[0] == "vi":
                vi.add(r[1])
            elif isinstance(r, tuple) and r[0] == "r" and r[1] < 32:
                gpr_written.add(r[1])
    em = _Emitter(vu, vu_flush)
    pre: list[str] = []
    body: list[str] = []
    post: list[str] = []

    def ptr_of(o: Operand) -> str:
        if o.lv_addr is not None:
            return f"G(openrac_quadword_address((gaddr)({o.lv_addr})))"
        if o.lv_text is not None:
            return f"(void *)&({o.lv_text})"
        raise Vu0Unsupported(f"operand {o.k} has no place in memory")

    def size_of(o: Operand) -> str:
        if o.lv_addr is not None:
            return "16"
        return f"(sizeof({o.lv_text}) < 16 ? sizeof({o.lv_text}) : 16)"

    # Vector operands: four floats, from the C variable, back to it.
    first_write: dict[int, _Ins] = {}
    for i in insns:
        _, wfull, wpart = _effects(i)
        for r in wfull | wpart:
            if isinstance(r, int) and r >= 32 and (r - 32) not in first_write:
                first_write[r - 32] = i
    fill_from: dict[int, int] = {}  # output -> the register whose fields it starts with
    for k in sorted(vec):
        o = ops[k]
        pre.append(f"openrac_f32 openrac_v{k}_[4] = {{0.0f, 0.0f, 0.0f, 0.0f}};")
        if not o.output:
            pre.append(f"memcpy(openrac_v{k}_, {ptr_of(o)}, {size_of(o)});")
            continue
        pre.append(f"void *const openrac_p{k}_ = {ptr_of(o)};")
        tied = [t for t, to in alias.items() if to == k]
        if tied:
            pre.append(f"memcpy(openrac_v{k}_, {ptr_of(ops[tied[0]])}, {size_of(ops[tied[0]])});")
        else:
            pre.append(f"memcpy(openrac_v{k}_, openrac_p{k}_, {size_of(o)});")
            i = first_write.get(k)
            if (not o.constraint.lstrip().startswith("+") and i is not None and i.dest != 15
                    and i.op not in ("lqc2", "qmtc2") and i.fs != 32 + k):
                # An output GCC gave a register of its own, written in part: the fields left are
                # those of the register the instruction reads first, the one GCC gives an output
                # whose input dies there (vadd.xyz %0, %1, %2 keeps %1's w, as headers/vu0.h's
                # vec_add_ab does).
                fill_from[k] = i.fs
        post.append(f"memcpy(openrac_p{k}_, openrac_v{k}_, {size_of(o)});")

    # Integer register operands: 64 bits and the upper half, as the EE's register holds a value.
    for k in sorted(gpr):
        o = ops[k]
        pre.append(f"openrac_u64 openrac_o{k}_ = 0, openrac_oh{k}_ = 0;")
        src = None
        tied = [t for t, to in alias.items() if to == k]
        if not o.output:
            src = o
        elif tied:
            src = ops[tied[0]]
        elif o.constraint.lstrip().startswith("+"):
            src = o
        if src is not None:
            v = src.value if src.value is not None else src.lv_text
            if v is None:
                raise Vu0Unsupported(f"operand {k} has no value")
            pre.append(f"{{ const __typeof__({v}) openrac_in_ = ({v}); "
                       f"if (sizeof(openrac_in_) > 8) {{ openrac_uti w_ = 0; memcpy(&w_, &openrac_in_, 16); "
                       f"openrac_o{k}_ = (openrac_u64)w_; openrac_oh{k}_ = (openrac_u64)(w_ >> 64); }} "
                       f"else if (sizeof(openrac_in_) > 4) {{ openrac_u64 w_ = 0; memcpy(&w_, &openrac_in_, 8); "
                       f"openrac_o{k}_ = w_; }} "
                       f"else {{ openrac_o{k}_ = (openrac_u64)(openrac_s64)(openrac_s32)(openrac_in_); }} }}")
        if o.output:
            if o.lv_text is None:
                raise Vu0Unsupported(f"output {k} without an lvalue")
            t = o.host_type
            post.append(f"{{ if (sizeof({t}) > 8) {{ openrac_uti w_ = ((openrac_uti)openrac_oh{k}_ << 64) | openrac_o{k}_; "
                        f"memcpy((void *)&({o.lv_text}), &w_, 16); }} "
                        f"else {{ ({o.lv_text}) = ({t})openrac_o{k}_; }} }}")
    # Float operands of mfc1 / mtc1.
    for k in sorted(fpr):
        o = ops[k]
        if o.ctype not in ("float",):
            raise Vu0Unsupported(f"operand {k} of mfc1/mtc1 is not a float")
        pre.append(f"openrac_f32 openrac_f{k}_ = {o.value if not o.output else o.lv_text};")
        if o.output:
            post.append(f"({o.lv_text}) = openrac_f{k}_;")
    # Memory operands: the address, once.
    for k in sorted(mem):
        o = ops[k]
        pre.append(f"void *const openrac_m{k}_ = {ptr_of(o)};")
    # The integer registers the block names.
    regs = sorted({r[1] for i in insns for part in _effects(i) for r in part
                   if isinstance(r, tuple) and r[0] == "r" and 0 < r[1] < 32})
    for n in regs:
        init = gpr_args.get(n)
        val = f"(openrac_u64)(openrac_s64)(openrac_s32)({init})" if init is not None else "0"
        pre.append(f"openrac_u64 openrac_r{n}_ = {val}, openrac_h{n}_ = 0;")

    def address(m: tuple) -> str:
        if m[0] == "op":
            return f"(uint8_t *)openrac_m{m[1]}_"
        reg = em.R(m[1])
        return f"(uint8_t *)G(openrac_quadword_address((gaddr)({reg}) + (gaddr)({m[2]})))"

    for i in insns:
        for k, reg in list(fill_from.items()):
            if _writes(i, 32 + k):
                body += [f"openrac_v{k}_[{c}] = {em.vf(reg, c)};" for c in range(4)]
                del fill_from[k]
        op = i.op
        if op == "lqc2":
            if i.ft != 0:
                body.append(f"memcpy(openrac_t_, {address(i.mem)}, 16);")
                body += [f"{em.vf(i.ft, c)} = openrac_t_[{c}];" for c in range(4)]
        elif op == "sqc2":
            body += [f"openrac_t_[{c}] = {em.vf(i.ft, c)};" for c in range(4)]
            body.append(f"memcpy({address(i.mem)}, openrac_t_, 16);")
        elif op == "lq":
            body.append(f"memcpy(&openrac_q_, {address(i.mem)}, 16);")
            if i.rt != 0:
                body += [f"{em.R(i.rt)} = (openrac_u64)openrac_q_;", f"{em.H(i.rt)} = (openrac_u64)(openrac_q_ >> 64);"]
        elif op == "sq":
            body.append(f"openrac_q_ = ((openrac_uti){em.H(i.rt)} << 64) | (openrac_uti){em.R(i.rt)};")
            body.append(f"memcpy({address(i.mem)}, &openrac_q_, 16);")
        elif op == "mfc1":
            body += em.W(i.rt, f"(openrac_u64)(openrac_s64)(openrac_s32)openrac_ee_bits(openrac_f{alias.get(i.fs, i.fs)}_)")
        elif op == "mtc1":
            body.append(f"openrac_f{alias.get(i.fs, i.fs)}_ = openrac_ee_f((openrac_u32){em.R(i.rt)});")
        elif op.startswith("ds"):
            sa = i.sa + (32 if op.endswith("32") else 0)
            src = em.R(i.rt)
            if op.startswith("dsll"):
                value = f"(openrac_u64){src} << {sa}"
            elif op.startswith("dsrl"):
                value = f"(openrac_u64){src} >> {sa}"
            else:
                value = f"(openrac_u64)((openrac_s64){src} >> {sa})"
            body += em.W(i.rd, value)
        elif op == "qmfc2" and i.rt != 0:
            body += [f"{em.R(i.rt)} = (openrac_u64)openrac_ee_bits({em.vf(i.rd, 0)}) | ((openrac_u64)openrac_ee_bits({em.vf(i.rd, 1)}) << 32);",
                     f"{em.H(i.rt)} = (openrac_u64)openrac_ee_bits({em.vf(i.rd, 2)}) | ((openrac_u64)openrac_ee_bits({em.vf(i.rd, 3)}) << 32);"]
        elif op == "qmfc2":
            pass
        else:
            try:
                body += em.vu0(i)
            except Exception as e:  # asmc.AsmUnsupported, or a field this module did not set
                raise Vu0Unsupported(f"{i.text}: {e}") from e

    state.vf, state.acc, state.q, state.vi = written_vf, acc, q, vi
    if any(f"{vu}->" in b for b in body):
        state.used = True
    inner = ["openrac_f32 v_[4]; openrac_u32 m_ = 0; gaddr t_ = 0; openrac_f32 openrac_t_[4]; "
             "openrac_uti openrac_q_ = 0;", *body,
             "(void)v_; (void)m_; (void)t_; (void)openrac_t_; (void)openrac_q_;"]
    return "GBARRIER(); { " + " ".join(pre) + " { " + " ".join(inner) + " } " + " ".join(post) + " } GBARRIER();"


def _writes(i: _Ins, reg: int) -> bool:
    _, wfull, wpart = _effects(i)
    return reg in wfull or reg in wpart
