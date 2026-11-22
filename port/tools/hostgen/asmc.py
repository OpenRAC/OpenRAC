# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""C from a function's assembly, for the functions the decompilation has no C for.

A function the decompilation still has as assembly (an INCLUDE_ASM stub) would
be a stub in the port that does nothing. Where the game's setup has written its
assembly (asm.py reads it), this writes C that does what the instructions do, in
hostgen's guest model, so the port runs it until the decompilation or a backfill
gives it real C, which always wins (only stubs are translated).

The C keeps the EE's registers as locals (port/runtime/include/openrac/ee.h):

- integer registers as 64-bit values, with a second 64-bit half (hN) where the
  function uses 128-bit loads, stores or MMI; HI and LO (and HI1, LO1); the FPU's
  registers as floats, its condition bit and its accumulator; VU0's registers are
  the program's one vector unit (openrac_vu0), since hand-written routines leave
  results there for the next;
- every instruction is one statement or a few; a branch target is a label; a
  branch's delay slot runs after the condition is taken and before the jump, and a
  likely branch's only when it is taken, as the EE runs them;
- $sp is the game stack (openrac_guest_sp at entry, given back at every return), so
  the function's frame and the addresses of its locals are game memory, and a call
  sees openrac_guest_sp at the caller's $sp; $gp is the game's;
- the function's parameters arrive in $a0-$t3 and $f12-$f19 by its prototype (the one
  hostgen inferred for the stub: the decompilation's declarations, or, for a function
  declared nowhere, the argument registers it reads first); jr $ra returns $v0 or $f0
  as the prototype says;
- jal goes through hostgen's own call path: to the callee's definition by its
  prototype (arguments from the registers by kind, as the EE passes them), through
  the address for a function the catalogue folded, through the level's hook for a
  hook stub; jalr and a jump through a register that is not a jump table go through
  GFN; the result lands in $v0 or $f0. A callee with no C that takes inputs in
  registers no prototype names (a hand-written helper) runs in place instead, back
  through $ra; a jump into code without C (a routine's continuation) is followed;
- an address formed from a symbol (%hi / %lo) or through $gp is relocated as hostgen
  relocates the C's globals (lower.Unit.global_address, code_address).

What it cannot write (system calls, COP0, VU0 microprograms and branches on VU0, the
random-number unit, a function reading a register no caller passes) keeps the stub, and the
report lists the function with the reason. The hardware's semantics it keeps are listed in
docs/port/HARDWARE_SEMANTICS.md.

mode "console" writes the same C for the console's own compiler instead, calls and
globals by retail address: the form rac1-decomp's tools/fcheck.py checks against
retail (tests/asmc_console.h).
"""

from __future__ import annotations

import os
import sys
import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

import asm
import asmext
import ctype
import retail
from lower import FnReport, Unit, UnitReport, Unsupported, _Fn, hexaddr, LEVEL_NAME
from program import Function, Program, Signature, call_copy as program_call_copy, call_copy_expr, canon, cname, inherit_results, prototype

ARG_REGS = list(range(4, 12))     # $a0-$a3, $t0-$t3 (the EE's eight integer argument registers)
FARG_REGS = list(range(12, 20))   # $f12-$f19
INT_TYPES = set(ctype.INT_NAMES) | {"gaddr"}
WIDE = {"long long", "unsigned long long", "long", "unsigned long", "long long int", "long int",
        "unsigned long long int", "unsigned long int"}
# Called with (step, done, total) while functions are found ("find") and translated
# ("translate"), for tools/asm_part.py's progress.
PROGRESS = None
# ei and di only gate the console's interrupts: the port takes none (its handlers run at set
# points, never in the middle of a function), so they do nothing there, as sync does.
SKIP = {"nop", "sync", "pref", "cache", "vnop", "vwaitq", "ssnop", "ei", "di"}
UNSUPPORTED = {
    "syscall": "a system call", "mfc0": "COP0 (mfc0)", "mtc0": "COP0 (mtc0)", "eret": "COP0 (eret)",
    "cop0": "COP0", "vcallms": "a VU0 microprogram (vcallms)",
    "vcallmsr": "a VU0 microprogram (vcallmsr)",
    "bc0f": "COP0 branch", "bc0t": "COP0 branch", "bc0fl": "COP0 branch", "bc0tl": "COP0 branch",
    "bc2f": "VU0 branch", "bc2t": "VU0 branch", "bc2fl": "VU0 branch", "bc2tl": "VU0 branch",
}
# bltzal and bgezal (and their likely forms): a call when the 64-bit register's sign says so; $ra
# is written either way (hand-written code calls its helpers so: bgezal $t0, ...).
COP0_BRANCHES = {"bc0f", "bc0t", "bc0fl", "bc0tl"}
COND_CALLS = {"bltzal", "bgezal", "bltzall", "bgezall"}
# The parallel multiplies and divides on LO and HI (openrac_ee_pmul, ee_ops.h).
PMUL_OPS = {"pmultw", "pmultuw", "pmaddw", "pmadduw", "pmsubw", "pmaddh", "pmsubh", "phmadh", "phmsbh",
            "pdivw", "pdivuw", "pdivbw"}
MMI_OPS = {
    "paddw", "psubw", "pcgtw", "pmaxw", "pminw", "pceqw", "paddh", "psubh", "pcgth", "pmaxh", "pminh",
    "pceqh", "paddb", "psubb", "pcgtb", "pceqb", "pextlw", "pextuw", "pextlh", "pextuh", "pextlb",
    "pextub", "ppacw", "ppach", "ppacb", "pcpyld", "pcpyud", "pcpyh", "pand", "por", "pxor", "pnor",
    "psllw", "psrlw", "psraw", "psllh", "psrlh", "psrah", "pexew", "pexcw", "prot3w", "pexeh", "pexch",
    "prevh", "pabsw", "pabsh", "padduw", "psubuw", "padduh", "psubuh", "paddub", "psubub", "paddsw",
    "psubsw", "paddsh", "psubsh", "paddsb", "psubsb", "plzcw", "pext5", "ppac5", "pinth", "pinteh",
    "psllvw", "psrlvw", "psravw", "padsbh",
}
CONDITIONAL = {"beq", "bne", "blez", "bgtz", "bltz", "bgez", "beql", "bnel", "blezl", "bgtzl", "bltzl",
               "bgezl", "bc1f", "bc1t", "bc1fl", "bc1tl"}


class AsmUnsupported(Exception):
    """An instruction or a shape of code this does not write; the function stays a stub."""


@dataclass
class Result:
    name: str
    text: str | None = None          # the C, when translated
    reason: str = ""                 # why not, when not
    source: str = ""                 # the assembly file, relative to the decompilation
    report: FnReport | None = None


def S32(x: str) -> str:
    return f"OPENRAC_EE_S32({x})"


def hx(v: int) -> str:
    return f"0x{v & 0xFFFFFFFF:08X}u"


def _comps(dest: int) -> list[int]:
    return [c for c in range(4) if dest & (8 >> c)]


def is_control(i: asm.Insn) -> bool:
    return i.op in asm.BRANCHES


def _conditional(i: asm.Insn) -> bool:
    """A branch that may fall through: not j, jr, jal or jalr, nor beq of a register with itself."""
    if i.op in ("j", "jal", "jr", "jalr") or i.op.endswith("al") or i.op.endswith("all"):
        return False
    return not (i.op == "beq" and i.rs == i.rt)


LEVEL_DATUM = re.compile(r"D_L(\d\d)_([0-9A-Fa-f]{8})")


def ps2_float_named(program: Program, symbol: str) -> bool:
    """Whether hostgen.json "ps2_float" names SYMBOL, the function a call-site variant
    (SYMBOL_N) is of, or (as F<hash>_FUN_<address>) the catalogue's name for a func_<address>
    it names."""
    names = getattr(program, "ps2_float", ())
    # The catalogue's name is matched whole first: its address is digits, which the call-site
    # suffix's pattern would take for a suffix (F10bf3d29_FUN_00283098 became F10bf3d29_FUN, and
    # no catalogue variant of a "ps2_float" function was ever computed as the EE does).
    m = re.fullmatch(r"F[0-9a-f]+_FUN_([0-9A-F]{8})(?:_\d+)?", symbol)
    if m is not None:
        return f"func_{m.group(1)}" in names
    base = re.sub(r"_\d+$", "", symbol)
    return symbol in names or base in names


class Translator:
    forward_unprototyped = False  # asm.forward_unprototyped in hostgen.json
    # asm.in_place in hostgen.json: hand-written routines called in their own register convention,
    # whose code a translation runs in place whatever C they have (rac1's collision cell lookup).
    in_place: frozenset = frozenset()
    # asm.dma_stores in hostgen.json: the kernels that start the scratchpad's DMA channels by writing
    # their registers (lui 0x1000), which no DMA controller answers: their word stores go through
    # openrac_ee_sw_dma, which does the transfer (openrac/ee_ops.h).
    dma_stores = False
    # asm.wide_register_calls in hostgen.json: a call through a code address hands on the upper
    # halves of $a0-$a3 as well (openrac_guest_hi_args), and a function's entry takes them in, so a
    # 128-bit argument survives the call (Up Your Arsenal's func_L01_003AB448, two vectors by value).
    wide_register_calls = False
    # asm.fit_declarations in hostgen.json: of the declarations of a function that has no C, the one
    # its code agrees with, not the longest (fitting_declaration).
    fit_declarations = False
    verified_signatures: frozenset = frozenset()  # asm.signatures in hostgen.json
    # asm.register_jumps in hostgen.json: a jump through a register keeps its target in j_, as a VU load
    # in the delay slot uses t_, and a computed jump into the function's own code stays in it (_computed_targets).
    register_jumps = False
    # asm.vu_flush in hostgen.json: an FMAC result too small for VU0 is zero of its sign, as in
    # func_L00_00316F60's joints, where the host's denormal was the only difference from retail.
    vu_flush = False
    # asm.dma_done in hostgen.json: a transfer here is over when the store that starts it returns
    # (asm.dma_stores), so a wait on COP0's condition has nothing to wait for: bc0t is always taken and
    # bc0f never (func_002A2450's copy back from the scratchpad).
    dma_done = False
    # asm.input_idioms in hostgen.json: reads that are no inputs (_no_input): func_002A05A8's
    # `sub.s $f0, $f0, $f0`, func_002A3110 saving $t8 and $t9 to the scratchpad.
    input_idioms = False
    # asm.signature_check in hostgen.json: each translated function's code checked against the
    # signature it is translated and called with (_check_arguments, _check_result): an argument
    # register it reads on some path from its entry that the signature does not pass, and a call
    # after which the code reads $v0 or $f0 that the callee's signature does not hand back. Either
    # runs on whatever the register held before, where the EE's had the value. Each finding goes to
    # `dropped` (name -> what), for hostgen's guard (asm.signature_allow).
    signature_check = False
    # asm.result_flow in hostgen.json: a function declared nowhere that sets $f0 and uses $v0 only
    # as a scratch register returns a float (_v0_reaches_return), not an int.
    result_flow = False
    # asm.live_arguments in hostgen.json: a translated function takes the argument registers it
    # reads on any path from its entry before writing them (_live_arguments), not only those read
    # before its first call; a prototype that leaves some out gets them added, as _entry_reads's.
    live_arguments = False
    # asm.wide_call_results in hostgen.json: a call through a folded copy's own target keeps a
    # 64-bit result its callee's declaration gives (indirect's wide_result), not its low half.
    wide_call_results = False
    # asm.float_parameters_on_calls in hostgen.json: a call through a code address hands on the
    # float parameters the function came in with as well as the float registers it writes, since
    # on the EE they are still in $f12-$f19 when it does not touch them (NTSC-U's
    # func_L01_00432FE8, the help box's frame: its four extents went to the frame drawer as 0).
    float_parameters_on_calls = False
    # asm.stack_arguments in hostgen.json: integer arguments past the eighth, which the caller
    # passes in its frame, count as read where the code loads them (_stack_arguments).
    stack_arguments = False
    # The float arithmetic computed as the EE's (openrac/ps2float.h) rather than in the host's
    # floats: the shadows (translate_shadows), checked bit for bit against the port's own C.
    ps2: bool = False
    def __init__(self, program: Program, index: asm.Index | None, gp: int | None = None, mode: str = "host",
                 source: Path | None = None):
        self.program = program
        self.index = index
        self.gp = gp
        self.gp_addresses = False  # hostgen.json asm.gp_addresses
        self.mode = mode
        self.source = source
        self._decoded: dict = {}
        self.host_text = ""   # the port's own C for the game (host/), which calls game functions by their prototypes
        self.dropped: dict[str, list[str]] = {}  # asm.signature_check's findings, by function

    # ---- What to translate ----

    def overlay_of(self, name: str) -> int:
        for pattern in self.program.code_names:
            m = pattern.match(name)
            if m:
                o = m.groupdict().get("overlay")
                return int(o) if o is not None else -1
        return -1

    def find(self, f: Function) -> tuple[int, int] | None:
        return self.index.find([f.symbol], self.program.code_places(f.symbol), own=f.asm)

    # ---- One function ----

    def translate(self, f: Function, found: tuple[int, int] | None = None) -> Result:
        res = Result(f.symbol)
        found = found or self.find(f)
        if found is None:
            res.reason = "no assembly for it"
            return res
        line_block = self.index.code.get(found)
        if line_block is not None:
            res.source = self._relative(line_block[1].path)
        # hostgen.json "ps2_float" names translated assembly too (a call-site variant by its
        # function's name): its FPU and VU0 arithmetic is then the EE's, as for the shadows.
        shadows = self.ps2
        self.ps2 = shadows or ps2_float_named(self.program, f.symbol)
        try:
            res.text, res.report = self._function(f, found)
        except (AsmUnsupported, Unsupported) as e:
            res.reason = str(e)
            res.text = None
        finally:
            self.ps2 = shadows
        return res

    def _relative(self, path: Path) -> str:
        if self.source is not None:
            try:
                return path.resolve().relative_to(self.source.resolve()).as_posix()
            except ValueError:
                pass
        return path.name

    def _load(self, f: Function, found: tuple[int, int]) -> None:
        """Follows the function's flow from its entry through its program's code."""
        self.f = f
        self.home, self.entry_addr = found
        self._fregs = None
        self.inline_calls: set[int] = set()
        self.returns: set[int] = set()
        self.reach, self.delay_only, self.jtables = self._flow()
        # VU0's MAC and STATUS flags are kept only by a function that reads them (CFC2 $vi16 or
        # $vi17, after its own macro operations: the collision code's tests); elsewhere they would
        # cost every vector operation a few more for nothing.
        self.vu_flags = any((i := self.at(a)) is not None and i.op == "cfc2" and i.rd in (16, 17)
                            for a in self.reach)
        self.starts_dma = self.dma_stores and any((i := self.at(a)) is not None and i.op == "lui"
                                                  and i.imm & 0xFFFF == 0x1000 for a in self.reach)
        self._check_inputs()

    _inline_cache: dict = {}

    def _inline_target(self, target: int) -> bool:
        """Whether a call to target runs the callee's code in place: a function with no C whose
        assembly is here and which reads, before writing, a register no caller passes arguments
        in (on its own it could not be translated)."""
        key = (id(self.index), self.home, target)
        cache = Translator._inline_cache
        if key in cache:
            return bool(cache[key])
        if self.at(target) is not None and self._name_at(target) in self.in_place:
            cache[key] = True
            return True
        if self._is_other_entry(target) or self.at(target) is None:
            cache[key] = False
            return False
        cache[key] = False                   # while it is looked at: a call back to it is a call
        sub = Translator(self.program, self.index, self.gp, self.mode, self.source)
        sub.gp_addresses = self.gp_addresses
        sub.forward_unprototyped = self.forward_unprototyped
        sub.in_place = self.in_place
        sub.dma_stores = self.dma_stores
        sub.wide_register_calls = self.wide_register_calls
        sub.float_parameters_on_calls = self.float_parameters_on_calls
        sub.fit_declarations = self.fit_declarations
        sub.register_jumps = self.register_jumps
        sub.input_idioms = self.input_idioms
        sub.dma_done = self.dma_done
        sub.vu_flush = self.vu_flush
        sub.verified_signatures = self.verified_signatures
        sub.ps2 = self.ps2
        try:
            sub._load(Function(f"sub_{target:08X}", Signature(ctype.Base("int"), (), False, False)),
                      (self.home, target))
            result = False
        except AsmUnsupported as e:
            result = "before writing it" in str(e)
        cache[key] = result
        return result

    # Registers a caller does not pass arguments in and does not keep: a function that reads one
    # before it writes it takes an input C cannot give it (a hand-written routine's own convention).
    SCRATCH = {1, 2, 3, 12, 13, 14, 15, 24, 25}
    FSCRATCH = set(range(0, 12))

    def _check_inputs(self) -> None:
        written_g, written_f = set(), set()
        stop = None
        for a in sorted(x for x in self.reach if x >= self.entry_addr):
            if stop is not None and a > stop:
                break
            ins = self.at(a)
            rg, wg, rf, wf = rw(ins)
            if self.input_idioms:
                rg, rf = self._no_input(ins, rg, rf)
            bad = (rg & self.SCRATCH) - written_g
            fbad = (rf & self.FSCRATCH) - written_f
            if bad or fbad:
                which = ", ".join([f"${r}" for r in sorted(bad)] + [f"$f{r}" for r in sorted(fbad)])
                raise AsmUnsupported(f"it reads {which} at {a:08X} before writing it: an input outside "
                                     "the argument registers")
            written_g |= wg
            written_f |= wf
            if is_control(ins) and stop is None:
                stop = a + 4

    def _no_input(self, ins: asm.Insn, rg: set, rf: set) -> tuple[set, set]:
        """The registers an instruction reads, without those whose value cannot matter: an operand
        taken from itself is zero whatever it held, and a register put away in the scratchpad is
        one a hand-written routine saves to use and puts back."""
        if ins.op in ("sub.s",) and ins.fs == ins.ft:
            return rg, rf - {ins.fs}
        if ins.op in ("subu", "sub", "dsubu", "dsub", "xor") and ins.rs == ins.rt:
            return rg - {ins.rs}, rf
        if ins.op == "sq":
            for k in range(1, 5):
                i = self.at(ins.addr - 4 * k)
                if i is not None and i.op == "lui" and i.rt == ins.rs:
                    if i.imm & 0xFFFF == 0x7000:
                        return rg - {ins.rt}, rf
                    break
        return rg, rf

    def prepare_signature(self, f: Function, found: tuple[int, int]) -> None:
        """Gives f the signature its translation is written with (_signature)."""
        self._load(f, found)
        if self.fit_declarations and f.symbol not in self.verified_signatures:
            f.sig = self._fitting_declaration(f)
        sig = self._signature(f)
        if re.search(r"\b" + re.escape(cname(f.symbol)) + r"\b", self.host_text):
            return  # the port's own C calls it by the signature it has
        if not self.forward_unprototyped and not f.sig.prototyped and self._compatible(sig, f.sig):
            # Declared `int f()`: callers keep calling it so; the definition with its integer
            # parameters is compatible with that declaration.
            return
        # With asm.forward_unprototyped, an old-style declaration keeps the assembly's inferred
        # parameters, so lower._match_args can forward incoming registers omitted from the
        # decompilation's call expression (rac4).
        f.sig = sig

    def _fitting_declaration(self, f: Function) -> Signature:
        """The declaration of a function with no C that its own code agrees with. Files declare a
        callee as each saw it, and the longest of those can be one file's mistake: one `(char *,
        char *, char *, float)` for Vec4Scale gave a 3-argument function a fourth, and all its
        callers a zero in place of the scale register. Registers decide: the declarations that cover
        what the code reads, the fewest extra parameters (a register it never reads is no parameter),
        then the most common. Any declaration's result that the code sets is kept, whatever `void`s
        the other files wrote."""
        proto = [s for s in f.declared if s.prototyped and not s.variadic]
        if not proto or any(s.variadic for s in f.declared) or f.symbol in getattr(self.program, "returns_64", ()):
            return f.sig
        ints, floats, sets_v0, sets_f0 = self._entry_reads(frame_saves=True)
        c = lambda t: canon(t, self.program.typedefs)
        def kinds(s):
            nf = sum(1 for p in s.params if c(p) == "float")
            return len(s.params) - nf, nf
        count = Counter(kinds(s) for s in proto)
        def rank(k):
            short = max(0, ints - k[0]) + max(0, floats - k[1])
            extra = max(0, k[0] - ints) + max(0, k[1] - floats)
            return short, extra, -count[k], k
        shape = min(count, key=rank)
        same = [s for s in proto if kinds(s) == shape]
        params = Counter(tuple(s.params) for s in same).most_common(1)[0][0]
        def result_fits(s):
            r = c(s.ret)
            return r == "void" or (r == "float" and sets_f0) or (r != "float" and sets_v0)
        results = Counter(s.ret for s in proto if result_fits(s) and c(s.ret) != "void")
        if results:
            ret = results.most_common(1)[0][0]  # a caller that ignores it loses nothing; one that reads it gets it
        else:
            ret = next((s.ret for s in proto if c(s.ret) == "void"), same[0].ret)
        return Signature(ret, params, False, True)

    def _compatible(self, sig: Signature, unprototyped: Signature) -> bool:
        """Whether a definition of sig may stand for a declaration `T f()` (unprototyped): the same
        result, no parameters that promotion changes (a float)."""
        c = lambda t: canon(t, self.program.typedefs)
        return (not unprototyped.prototyped and not unprototyped.params and c(sig.ret) == c(unprototyped.ret)
                and all(c(p) not in ("float", "char", "signed char", "unsigned char", "short",
                                     "unsigned short") for p in sig.params))

    def _function(self, f: Function, found: tuple[int, int]):
        self._load(f, found)
        self.report = FnReport(f.symbol, "translated")
        self.unit = Unit(self.program, "<asm>", {"inner": []}, UnitReport("<asm>"))
        self.unit.fn = _Fn(f.symbol, ctype.Func(f.sig.ret, tuple(f.sig.params), f.sig.variadic, f.sig.prototyped),
                           [], report=self.report)
        # The signature the program has for it (prepare_signature widened it where it could), or,
        # for one declared `int f()`, the integer parameters its registers say, which that
        # declaration allows.
        sig = self._signature(f)
        if self.mode == "host" and (f.sig.prototyped or not self._compatible(sig, f.sig)):
            sig = f.sig
        # (The console's form, a candidate of its own, takes every register its code reads.)
        self.sig = sig
        if self.signature_check and self.mode == "host":
            self._check_arguments(f.symbol, sig)
        self.calls_k: dict[str, int] = {}
        body = self._body()
        line_block = self.index.code.get(found)
        self.where = self._relative(line_block[1].path) if line_block else ""
        return self._assemble(f, sig, body), self.report

    def at(self, addr: int) -> asm.Insn | None:
        """The instruction at addr in the function's program, decoded, or None."""
        key = (self.home, addr & 0xFFFFFFFF)
        if key not in self._decoded:
            lb = self.index.code.get(key)
            self._decoded[key] = None if lb is None else asm.decode(lb[0].word, lb[0].addr, lb[0].reloc)
        return self._decoded[key]

    def text_at(self, addr: int) -> str:
        lb = self.index.code.get((self.home, addr))
        return lb[0].text if lb else ""

    def _need(self, addr: int, why: str) -> asm.Insn:
        ins = self.at(addr)
        if ins is None:
            raise AsmUnsupported(f"{why} goes on into code with no assembly here, at {addr:08X} "
                                 "(a fragment of a function with C, or a gap)")
        if ins.word == 0xCDCDCDCD:
            raise AsmUnsupported(f"it reaches padding at {addr:08X}, not code")
        return ins

    def _name_at(self, addr: int) -> str | None:
        """The function the catalogue places at addr, in this program or the executable."""
        sym = getattr(self.program, "by_place", {}).get((self.home, addr))
        if sym is None and self.home >= 0:
            sym = getattr(self.program, "by_place", {}).get((-1, addr))
        return sym

    def _is_other_entry(self, addr: int) -> bool:
        """Whether a jump to addr leaves for another function, a tail call: addr starts a
        function that has C (or the port's own). Other code the assembly has is followed into, as
        the EE runs it with every register as it is: a second entry of the same routine (alabel),
        or a hand-written routine's continuation in the next file, which takes registers no
        prototype says."""
        if addr == self.entry_addr:
            return False
        if self.at(addr) is None:
            return True                      # code with no assembly here: only a call can reach it
        sym = self._name_at(addr)
        f = self.program.functions.get(sym) if sym else None
        has_c = f is not None and (f.unit not in (None, "<asm>") or sym in self.program.host)
        return has_c

    def _table_targets(self, jr_addr: int) -> list[int]:
        """Where a jump through a register at jr_addr may go: the jump tables written beside its
        function, or the labels spimdisasm marked as their targets (jlabel), in its file."""
        lb = self.index.code.get((self.home, jr_addr))
        if lb is None:
            return []
        block = lb[1]
        targets = set(block.jlabels)
        for t in block.tables.values():
            targets.update(t)
        return sorted(a for a in targets if self.at(a) is not None)

    def _constant_before(self, at: int, reg: int, limit: int = 96) -> int | None:
        """The address REG was given by the lui and addiu before `at` in the code's straight line."""
        low = None
        for a in range(at - 4, at - 4 * limit, -4):
            i = self.at(a)
            if i is None:
                return None
            if i.op == "addiu" and i.rt == reg and i.rs == reg and low is None:
                low = i.imm
            elif i.op == "lui" and i.rt == reg:
                return ((i.imm & 0xFFFF) << 16) + (low or 0) & 0xFFFFFFFF
            elif reg in rw(i)[1]:
                return None
        return None

    def _computed_targets(self, at: int) -> list[int]:
        """A jump to a label of the function plus a computed offset (`la t0, L; addu t1, t0, t9; jr t1`):
        into a run of instructions, as func_00280B90 enters the middle of its loads for the vertices
        it has. Every instruction of that run, up to its first jump, is a target; the code's own
        arithmetic says which. Nothing for a jump through a pointer, a call the C would make."""
        rs = self.at(at).rs
        for a in range(at, at - 4 * 96, -4):
            i = self.at(a - 4)
            if i is None:
                return []
            if i.op in ("add", "addu", "dadd", "daddu") and i.rd == rs:
                base = self._constant_before(a - 4, i.rs)
                if base is None:
                    base = self._constant_before(a - 4, i.rt)
                break
            if rs in rw(i)[1]:
                return []
        else:
            return []
        if base is None or not 0 < base - self.entry_addr < 0x10000 or self._is_other_entry(base):
            return []
        # The run goes on past a conditional branch forward inside it (Up Your Arsenal's
        # func_003D37D8 enters a chain of compares, each skipping its two instructions: 0x003D3A4C
        # into 0x003D3A54 + 24 a vector); its delay slot is no entry. It ends at a jump.
        targets = []
        slot = False
        for t in range(base, base + 4 * 256, 4):
            i = self.at(t)
            if i is None:
                return []
            if not slot:
                targets.append(t)
            slot = False
            if is_control(i):
                if not (_conditional(i) and t < i.target <= t + 4 * 64):
                    break
                slot = True
        return targets

    def _flow(self):
        """The instructions reachable from the entry; those reached only as a delay slot; and
        each jump through a register that is a jump table, with its targets."""
        reach: set[int] = set()
        heads: set[int] = set()
        delay: set[int] = set()
        tables: dict[int, list[int]] = {}
        todo = [self.entry_addr]
        while todo:
            a = todo.pop()
            if a in heads:
                continue
            ins = self._need(a, "it")
            heads.add(a)
            reach.add(a)
            done_wait = self.dma_done and ins.op in COP0_BRANCHES
            if ins.op in UNSUPPORTED and self.microprogram(ins) is None and not done_wait:
                raise AsmUnsupported(f"{UNSUPPORTED[ins.op]} at {ins.addr:08X}")
            if not is_control(ins):
                todo.append(a + 4)
                continue
            d = self._need(a + 4, "a delay slot")
            if is_control(d):
                raise AsmUnsupported(f"a jump in the delay slot at {a + 4:08X}")
            reach.add(a + 4)
            delay.add(a + 4)
            op = ins.op
            if done_wait:
                todo.append(ins.target if op.startswith("bc0t") else a + 8)
            elif op in ("jal", *COND_CALLS) and self._inline_target(ins.target):
                # A hand-written routine that takes its inputs in registers no prototype names:
                # run in place, as the EE does, and back to the call through $ra (control).
                self.inline_calls.add(a)
                self.returns.add(a + 8)
                todo += [ins.target, a + 8]
            elif op in ("jal", "jalr", *COND_CALLS):
                todo.append(a + 8)
            elif op == "jr":
                if ins.rs != 31:
                    targets = self._table_targets(a)
                    if not targets and self.register_jumps:
                        targets = self._computed_targets(a)
                    if targets:
                        tables[a] = targets
                        todo.extend(targets)
            elif op == "j":
                if not self._is_other_entry(ins.target):
                    todo.append(ins.target)
            else:
                todo.append(ins.target)
                uncond = op in ("beq", "beql") and ins.rs == ins.rt
                if not uncond:
                    todo.append(a + 8)
        return reach, delay - heads, tables

    # ---- The signature ----

    def _signature(self, f: Function) -> Signature:
        sig = f.sig
        # Verified register-sized results (for example TEX0 descriptors) must
        # survive both the translated return and the call that consumes it.
        if f.symbol in getattr(self.program, "returns_64", ()) and canon(sig.ret, self.program.typedefs) in INT_TYPES:
            sig = Signature(ctype.Base("long long"), sig.params, sig.variadic, sig.prototyped)
        if sig.variadic:
            return sig
        if sig.prototyped or sig.params:
            # The decompilation's prototype, with the argument registers its code reads first that
            # the prototype leaves out (a declaration written for one caller): callers pass them,
            # as the EE's did (hostgen's call matching, passing a caller's own through).
            ints, floats, _, _ = self._entry_reads()
            if self.live_arguments:
                live_i, live_f = self._live_arguments()
                ints, floats = max(ints, live_i), max(floats, live_f)
            if self.stack_arguments and (stacked := self._stack_arguments()):
                ints = max(ints, len(ARG_REGS) + stacked)
            have_i = sum(1 for p in sig.params if canon(p, self.program.typedefs) != "float")
            have_f = len(sig.params) - have_i
            extra = [ctype.Base("int")] * max(0, ints - have_i) + [ctype.Base("float")] * max(0, floats - have_f)
            if not extra:
                return sig
            return Signature(sig.ret, tuple(sig.params) + tuple(extra), False, True)
        # Declared nowhere with parameters: what it reads of the argument registers first.
        ints, floats, sets_v0, sets_f0 = self._entry_reads()
        if self.live_arguments:
            # asm.live_arguments: what it reads on any path before writing it, past its first call
            # too (_live_arguments), and what its first call through a pointer takes and it has not
            # written: level 39's GUI text draw (0x00619B48) hands its $a1, the parent's rect, to
            # its rect getter; the LOADING screen's update (0x006788F8) tests $a0, the load's end,
            # only after its first call.
            live_i, live_f = self._live_arguments()
            ints, floats = max(ints, live_i), max(floats, live_f)
        if self.stack_arguments and (stacked := self._stack_arguments()):
            ints = max(ints, len(ARG_REGS) + stacked)
        params = [ctype.Base("int")] * ints + [ctype.Base("float")] * floats
        ret = sig.ret
        if self.result_flow and sets_v0 and sets_f0:
            sets_v0 = self._v0_reaches_return()
        if not f.declared and not sets_v0 and sets_f0:
            ret = ctype.Base("float")
        return Signature(ret, tuple(params), False, True)

    def _v0_reaches_return(self) -> bool:
        """asm.result_flow: whether some value the code writes to $v0 may reach a jr $ra unread (its
        result). A $v0 the code only uses as a scratch register (an slt and a branch on it: the text
        fit routine of Up Your Arsenal's multiplayer menus, 0x005946B0) is not one, so such a routine
        that sets $f0 returns a float. Flow it cannot follow (a jump through a register, code
        outside the function, a write in a delay slot) counts as reaching."""
        def walk(b: int, seen: set) -> bool:
            while True:
                if b in seen:
                    return False
                seen.add(b)
                ins = self.at(b)
                if ins is None or b not in self.reach:
                    return True
                rg, wg, _, _ = rw(ins)
                if 2 in rg:
                    return False
                if not is_control(ins):
                    if 2 in wg:
                        return False
                    b += 4
                    continue
                delay = self.at(b + 4)
                if delay is not None:
                    drg, dwg, _, _ = rw(delay)
                    if 2 in drg or 2 in dwg:
                        return False
                if ins.op in ("jal", "jalr") or ins.op in COND_CALLS:
                    return False  # the callee's result replaces it
                if ins.op == "jr":
                    return True
                nexts = [ins.target]
                if _conditional(ins):
                    nexts.append(b + 8)
                return any(walk(n, seen) for n in nexts)
        for a in self.reach:
            ins = self.at(a)
            if ins is None:
                continue
            if ins.op in ("jal", "jalr") or ins.op in COND_CALLS:
                if walk(a + 8, set()):  # the callee's result, handed on
                    return True
                continue
            if is_control(ins) or 2 not in rw(ins)[1]:
                continue
            prev = self.at(a - 4)
            if prev is not None and (a - 4) in self.reach and is_control(prev):
                # A delay slot's value goes where its branch goes.
                if prev.op == "jr":
                    return True
                if prev.op in ("jal", "jalr") or prev.op in COND_CALLS:
                    continue  # the callee's result replaces it
                nexts = [prev.target]
                if _conditional(prev) and prev.op not in asm.LIKELY:
                    nexts.append(a + 4)
                if any(walk(n, set()) for n in nexts):
                    return True
                continue
            if walk(a + 4, set()):
                return True
        return False

    def _reloaded_argument(self, save: asm.Insn) -> bool:
        """Whether SAVE keeps an argument register in a frame slot the code loads again: a use of
        the argument, kept for after a call. Going Commando's level loader tests its second
        argument only so. A variadic routine reaches its saved registers through a pointer."""
        if save.op == "swc1":
            if save.ft not in FARG_REGS:
                return False
            loads = ("lwc1", "lw")
        else:
            if save.rt not in ARG_REGS:
                return False
            loads = ("lw", "ld", "lq", "lwc1")
        if any(load.op in loads and load.rs == 29 and load.imm == save.imm
               for load in map(self.at, self.reach)):
            return True
        # The slot's address taken (`addiu rX, $sp, k` with the slot in the 16 bytes from k): the
        # argument is read through it, as libgcc's dp-bit routines hand their doubles, saved to the
        # frame, to unpack_d by address (func_00128F90's dpmul).
        return any(i is not None and i.op in ("addiu", "daddiu") and i.rs == 29 and i.rt != 29
                   and i.imm <= save.imm < i.imm + 16 for i in map(self.at, self.reach))

    def _entry_reads(self, frame_saves: bool = False):
        """The argument registers read before they are written, along the code from the entry up
        to its first call or return, in address order (a parameter is used early). With frame_saves,
        one saved to the frame counts: the function keeps it for after a call."""
        written_g, written_f = set(), set()
        read_g, read_f = set(), set()
        stop = None
        forwarded = None
        for a in sorted(x for x in self.reach if x >= self.entry_addr):
            if stop is not None and a > stop:
                break
            ins = self.at(a)
            rg, wg, rf, wf = rw(ins)
            if ins.op in ("sw", "sd", "sq", "swc1") and ins.rs == 29 and not frame_saves \
                    and not self._reloaded_argument(ins):
                # A register saved to the frame is not a use of it: a variadic routine saves every
                # argument register so, whatever its callers pass.
                rg, rf = rg - {ins.rt}, rf - {ins.ft}
            read_g |= {r for r in rg if r not in written_g}
            read_f |= {r for r in rf if r not in written_f}
            written_g |= wg
            written_f |= wf
            if ins.op in ("jal", "jalr", "jr") and stop is None:
                stop = a + 4   # its delay slot runs first
                if ins.op == "jal":
                    sym = self._callee(ins.target, None)
                    forwarded = self.program.functions.get(sym) if sym else None
        # Passing an incoming register unchanged is a use, too. Account for
        # delay-slot writes before testing which arguments the caller needs.
        if forwarded is not None and forwarded.sig.prototyped:
            ni = nf = 0
            for param in forwarded.sig.params:
                if canon(param, self.program.typedefs) == "float":
                    if nf < len(FARG_REGS) and FARG_REGS[nf] not in written_f:
                        read_f.add(FARG_REGS[nf])
                    nf += 1
                else:
                    if ni < len(ARG_REGS) and ARG_REGS[ni] not in written_g:
                        read_g.add(ARG_REGS[ni])
                    ni += 1
        sets_v0 = sets_f0 = False
        for a in self.reach:
            _, wg, _, wf = rw(self.at(a))
            sets_v0 |= 2 in wg
            sets_f0 |= 0 in wf
        ints = max([k + 1 for k, r in enumerate(ARG_REGS) if r in read_g], default=0)
        floats = max([k + 1 for k, r in enumerate(FARG_REGS) if r in read_f], default=0)
        return ints, floats, sets_v0, sets_f0

    # ---- Pieces of C ----

    @staticmethod
    def R(n: int) -> str:
        return "0" if n == 0 else f"r{n}"

    @staticmethod
    def H(n: int) -> str:
        return "0" if n == 0 else f"h{n}"

    def W(self, n: int, value: str) -> list[str]:
        return [] if n == 0 else [f"r{n} = {value};"]

    def microprogram(self, ins: asm.Insn) -> str | None:
        """The port's C for the VU0 microprogram a vcallms starts (hostgen.json asm.vu0_programs: the
        address in VU0's memory, as a string, to a function working on OPENRAC_VU0), or None."""
        if ins.op != "vcallms":
            return None
        programs = getattr(self.program, "vu0_programs", {})
        return programs.get((ins.word >> 6 & 0x7FFF) * 8)

    def vf(self, n: int, c: int) -> str:
        if n == 0:
            return "openrac_ee_f(0x3F800000u)" if c == 3 else "0.0f"
        return f"OPENRAC_VU0->vf[{n}][{c}]"

    def addr(self, ins: asm.Insn) -> str:
        """The address a load or store reaches."""
        if ins.rs == 28 and self.gp is not None:
            a = (self.gp + ins.imm) & 0xFFFFFFFF
            return f"(openrac_u32)({self._data(f'D_{a:08X}', a)})"
        if ins.rs == 0:
            return hx(ins.imm)
        return f"(openrac_u32)({self.R(ins.rs)} + {ins.imm})"

    def _data(self, sym: str, address: int) -> str:
        if self.mode != "host":
            return hx(address)
        return self.unit.global_address(address, self.unit._code_symbol(sym) is not None, self.program.data_level(sym))

    def _code(self, sym: str, address: int) -> str:
        if self.mode != "host":
            return hx(address)
        try:
            return self.unit.code_address(sym)
        except Unsupported:
            return hx(address)

    def _symbol_address(self, ins: asm.Insn) -> str | None:
        """For lui %hi(S): S's address as this function reaches it, minus S's own address (the
        relocation), or None when it is S's own address."""
        kind, sym, addend = ins.reloc
        address = None
        expr = None
        is_code = sym in self.program.functions or any(p.match(sym) for p in self.program.code_names)
        if sym.startswith("jtbl"):
            m = re.search(r"([0-9A-Fa-f]{8})", sym)
            if m:
                address = int(m.group(1), 16)
                expr = self._data(sym, address)
        elif is_code and not sym.startswith("D_"):
            places = self.program.code_places(self.program.canonical(sym))
            if places:
                own = [a for o, a in places if o == self.home] or [places[0][1]]
                address = own[0]
                expr = self._code(self.program.canonical(sym), address)
        else:
            # This level's own datum, named by retail.py for the address the code forms. The
            # decompilation's build may give the name another value (hostgen.json "derived": Going
            # Commando's, the boot program's address, for C that lands in both), but that is not
            # where this function's copy looks.
            own_name = LEVEL_DATUM.fullmatch(sym)
            if own_name and int(own_name.group(1)) == self.home:
                address = int(own_name.group(2), 16)
            else:
                address = self.program.data_address(sym)
            if address is not None:
                expr = self._data(sym, address)
        if address is None or expr is None or expr == hexaddr(address):
            return None
        return f"(openrac_u32)({expr}) - {hx(address)}"

    # ---- The body ----

    def _body(self) -> list[str]:
        """Every instruction group (an instruction, or a jump with its delay slot) in address order,
        each under its label, with a goto where the next in order is not where it falls through."""
        heads = sorted(self.reach - self.delay_only)
        out: list[str] = []
        if heads[0] != self.entry_addr:
            out.append(f"goto L_{self.entry_addr:08X};")
        for k, a in enumerate(heads):
            ins = self.at(a)
            out.append(f"L_{a:08X}:;")
            if not is_control(ins):
                out += self.stmt(ins)
                falls = a + 4
            else:
                delay = self.stmt(self.at(a + 4))
                out += self.control(ins, delay)
                falls = a + 8 if (ins.op in ("jal", "jalr", *COND_CALLS) or (ins.op in CONDITIONAL and not (
                    ins.op in ("beq", "beql") and ins.rs == ins.rt))) else None
            nxt = heads[k + 1] if k + 1 < len(heads) else None
            if falls is not None and falls != nxt:
                out.append(f"goto L_{falls:08X};")
        # Only the labels something goes to.
        used = set(re.findall(r"goto (L_[0-9A-F]{8});", "\n".join(out)))
        return [s for s in out if not (s.startswith("L_") and s[:10] not in used)]

    def control(self, ins: asm.Insn, delay: list[str]) -> list[str]:
        op = ins.op
        R = self.R
        a = ins.addr
        if op in COP0_BRANCHES:
            if op.startswith("bc0t"):
                return delay + [f"goto L_{ins.target & 0xFFFFFFFF:08X};"]
            return [] if ins.likely else delay
        if op in CONDITIONAL:
            cond = {
                "beq": f"{R(ins.rs)} == {R(ins.rt)}", "bne": f"{R(ins.rs)} != {R(ins.rt)}",
                "blez": f"(openrac_s64){R(ins.rs)} <= 0", "bgtz": f"(openrac_s64){R(ins.rs)} > 0",
                "bltz": f"(openrac_s64){R(ins.rs)} < 0", "bgez": f"(openrac_s64){R(ins.rs)} >= 0",
                "bc1t": "fcc_", "bc1f": "!fcc_",
            }[op[:-1] if ins.likely else op]
            go = f"goto L_{ins.target & 0xFFFFFFFF:08X};"
            if op in ("beq", "beql") and ins.rs == ins.rt:
                return delay + [go]
            if ins.likely:
                return [f"if ({cond}) {{", *["    " + s for s in delay], f"    {go}", "}"]
            return [f"c_ = {cond};", *delay, f"if (c_) {go}"]
        if op in COND_CALLS:
            # The sign of all 64 bits decides; $ra is the return point whether or not it calls.
            cond = f"(openrac_s64){R(ins.rs)} {'<' if op.startswith('bltz') else '>='} 0"
            link = f"r31 = {S32(hx(a + 8))};"
            if a in self.inline_calls:
                taken = [f"goto L_{ins.target:08X};"]
            else:
                taken = self.call(ins.target, self._operand_name(a), a)
                self._check_result(ins.target, self._operand_name(a), a)
            if ins.likely:
                return [f"c_ = {cond};", link, "if (c_) {", *["    " + s for s in delay + taken], "}"]
            return [f"c_ = {cond};", *delay, link, "if (c_) {", *["    " + s for s in taken], "}"]
        if op == "j":
            if not self._is_other_entry(ins.target):
                return delay + [f"goto L_{ins.target:08X};"]
            return delay + self.tail_call(ins.target, self._operand_name(a))
        if op == "jal":
            if a in self.inline_calls:
                return delay + [f"r31 = {S32(hx(a + 8))};", f"goto L_{ins.target:08X};"]
            self._check_result(ins.target, self._operand_name(a), a)
            return delay + [f"r31 = {S32(hx(a + 8))};"] + self.call(ins.target, self._operand_name(a), a)
        if op == "jalr":
            # The target register is read before the delay slot runs.
            return ([f"{self._jt} = (openrac_u32){R(ins.rs)};"] + delay + self.W(ins.rd, S32(hx(a + 8)))
                    + self.indirect(self._jt, a))
        if op == "jr":
            if ins.rs == 31:
                if not self.returns:
                    return delay + self.leave()
                # Back from a routine run in place, to its call; else this function returns.
                lines = [f"{self._jt} = (openrac_u32)r31;", *delay, f"switch ({self._jt}) {{"]
                lines += [f"case {hx(r)}: goto L_{r:08X};" for r in sorted(self.returns)]
                return lines + ["default: break;", "}", *self.leave()]
            if self._holds_return_address(ins.rs):
                return delay + self.leave()
            if a in self.jtables:
                return [f"{self._jt} = (openrac_u32){R(ins.rs)};"] + delay + self.dispatch(self._jt, self.jtables[a])
            return [f"{self._jt} = (openrac_u32){R(ins.rs)};"] + delay + self.indirect(self._jt, a) + self.leave()
        raise AsmUnsupported(f"{op} at {a:08X}")

    def _holds_return_address(self, reg: int) -> bool:
        """Whether a register only ever holds the $ra the function came in with: a leaf that calls
        hand-written helpers keeps it in a temporary (addi $t3, $ra, 0) and ends with jr $t3."""
        writes = [a for a in self.reach if reg in rw(self.at(a))[1]]
        for a in writes:
            ins = self.at(a)
            if ins.op in ("addi", "addiu", "daddi", "daddiu") and ins.rs == 31 and ins.imm == 0:
                continue
            # Or a register move of it (daddu $t9, $ra, $zero: Up Your Arsenal's func_L01_004436F4
            # returns through $t9), which went out as a call to the address it held, 0.
            if ins.op in ("addu", "daddu", "or") and {ins.rs, ins.rt} == {31, 0}:
                continue
            return False
        first_call = min((a for a in self.reach if 31 in rw(self.at(a))[1]), default=None)
        return bool(writes) and (first_call is None or max(writes) < first_call)

    @property
    def _jt(self) -> str:
        return "j_" if self.register_jumps else "t_"

    def _operand_name(self, addr: int) -> str | None:
        parts = self.text_at(addr).split()
        return parts[-1] if len(parts) > 1 and re.match(r"^[A-Za-z_][\w.$]*$", parts[-1]) else None

    def dispatch(self, value: str, targets: list[int]) -> list[str]:
        """A jump through a jump table: to the label at the same offset from the function's entry
        in the copy loaded (a level's copy's tables hold that copy's addresses)."""
        own = hx(self.entry_addr)
        if self.mode == "host" and self.program.code_places(self.f.symbol):
            own = self.unit._own_address()
        lines = [f"switch ((openrac_u32)({value} - (openrac_u32)({own}))) {{"]
        for a in targets:
            lines.append(f"case {hx(a - self.entry_addr)}: goto L_{a:08X};")
        lines.append(f"default: openrac_ee_lost(\"{self.f.symbol}\", {value}); {' '.join(self.leave())}")
        lines.append("}")
        return lines

    # ---- Calls ----

    def _callee(self, target: int, name: str | None) -> str | None:
        p = self.program
        if name and (name in p.functions or any(pt.match(name) for pt in p.code_names)):
            sym = p.canonical(name)
            if sym in p.functions:
                return sym
        for place in ((self.home, target), (-1, target)):
            sym = getattr(p, "by_place", {}).get(place)
            if sym is not None:
                return sym
        # Every function's places, once a program: a level's code calls thousands of places no
        # name states (rac3-pal's levels), and asking each function for each call took hours.
        places = getattr(p, "_callee_places", None)
        if places is None or getattr(p, "_callee_places_count", -1) != len(p.functions):
            places = {}
            for sym in p.functions:
                for place in p.code_places(sym):
                    places.setdefault(tuple(place), sym)
            p._callee_places = places
            p._callee_places_count = len(p.functions)
        return places.get((self.home, target))

    def call(self, target: int, name: str | None, at: int | None = None) -> list[str]:
        sym = self._callee(target, name)
        f = self.program.functions.get(sym) if sym else None
        if f is None or prototype(f, self.program.typedefs) is None:
            return self.indirect(self._code_literal(target), target)
        self.report.calls.add(sym)
        if f.sig.prototyped:
            args = self._args(sym, f.sig)
        else:
            # Declared without its parameters (`int f()`): the integer argument registers, which a
            # definition of that kind reads as far as it has parameters.
            args = [f"(int){self.R(r)}" for r in ARG_REGS]
        ret = canon(f.sig.ret, self.program.typedefs)
        fn_type = self._fn_type(f.sig)
        if self.mode != "host":
            # Retail code, which reads what the EE's registers hold whatever the decompilation
            # declares: every argument register, through a type that puts each where the EE has it.
            # (For the port the call is by the prototype, as C calls; the tests check that mapping.)
            if ret not in INT_TYPES and ret not in WIDE and ret not in ("void", "float"):
                raise AsmUnsupported(f"a call whose result is {ret}")
            # Four doublewords from the function's own $sp after them: the compiler puts parameters
            # past the registers at 0($sp), 8($sp)..., where a callee taking more than eight reads.
            params = ", ".join(["openrac_u64"] * 8 + ["float"] * 8 + ["openrac_u64"] * 4)
            regs = ([self.R(r) for r in ARG_REGS] + [f"f{r}" for r in FARG_REGS]
                    + [f"openrac_ee_ld((openrac_u32)r29 + {8 * k}u)" for k in range(4)])
            return self._invoke(f"(({ret} (*)({params})){hx(target)})({', '.join(regs)})", ret)
        elif sym in getattr(self.program, "level_hooks", set()) and self.unit._level_hook(sym) is not None:
            exe_fn, size, home, home_fn, stub = self.unit._level_hook(sym)
            k = self.calls_k.get(sym, 0)
            self.calls_k[sym] = k + 1
            hook = f"openrac_hook_in_copy({hx(exe_fn)}, {size}u, {home}, {hx(home_fn)}, {hx(stub)}, {k}u)"
            if getattr(self.program, "level_hooks_by_registers", False):
                # A hook stub's own declaration says nothing of what the level's function takes
                # (func_0026F808 is `void (void)`, Going Commando's level 0 fills it with a
                # function of three pointers): the registers, as for a call through an address.
                return self.indirect(hook, target if at is None else at)
            callee = f"GFN({fn_type}, {hook})"
        elif (call_copy := program_call_copy(self.program, self.unit.fn.code, sym, target)) is not None:
            # A jal that differs between this function's places (call_copies.tsv): the running copy's.
            # Through the registers: the copy's own function may take more than the name's.
            self.unit.fn.uses_self = True
            return self.indirect(call_copy_expr(call_copy), target if at is None else at,
                                 wide_result=self.wide_call_results and ret in WIDE)
        elif (wrapped := self._folded_wrapper_call(target, sym)) is not None:
            # A wrapper the catalogue folded across the levels although its copies call different
            # functions (hostgen.json "folded_wrappers"), as the caller or as the callee: what the
            # running copy's own jal calls, with the registers as they are. Up Your Arsenal's
            # func_L01_00451FB8 calls func_L01_004365D8 in Veldin and func_0038DBD8 in Starship
            # Phoenix; translated once, level 3's smoke puffs ran Veldin's callee and faulted on
            # 0xCDCDCDCD. Called by its name, the wrapper ran Veldin's code in every level
            # (func_L01_00447450's copy in Crash Site wraps func_0038DBD8; Veldin's probe read the
            # blended colour 0xAA00xxxx as a moby): reached by its address instead, the runtime
            # follows each place's own jal (guest.cpp, openrac_guest_function).
            self.unit.fn.uses_self = True
            return self.indirect(wrapped, target if at is None else at, ret == "float",
                                 self.wide_call_results and ret in WIDE)
        elif self.unit._folded_places(sym):
            # One name for several places (folded by the catalogue): the place this call goes to.
            callee = f"GFN({fn_type}, {self._code_literal(target)})"
        elif self.gp_addresses and (variant := self.unit._variant_call(sym)) is not None:
            # Code several levels carry, calling a function only some of them list: what this copy's
            # own call reaches (func_L04_00419918 in level 11 calls its own twin of func_L04_0043ABE0,
            # whose level 4 addresses are over the allocator there).
            # Through the registers, as a call copy: the copy's own function may take more than the
            # name's (Florana's copy of func_L01_003B2DC8, a wrapper Veldin's calls func_L01_00459AF8
            # through with no arguments, calls a class update that takes the moby it hands on).
            home, start, size, own = variant
            self.unit.fn.uses_self = True
            expr = f"openrac_call_in_copy({home}, {hx(start)}, {size}u, openrac_self_, {hx(own)})"
            return self.indirect(expr, target if at is None else at, ret == "float",
                                 self.wide_call_results and ret in WIDE)
        else:
            callee = cname(sym)
        if self.forward_unprototyped and not f.sig.prototyped:
            # The same generated unit may already contain this function's
            # definition with fewer integer parameters. Keep the original
            # unprototyped call contract even in that case (extra EE args).
            callee = f"(({fn_type}){callee})"
        return self._invoke(f"{callee}({', '.join(args)})", ret)

    def _folded_wrapper_call(self, target: int, sym: str | None = None) -> str | None:
        """For a jal in a wrapper (64 bytes or less) whose level copies call different functions,
        or a jal of such a wrapper by name (hostgen.json "folded_wrappers"): the running copy's own
        target (guest.h); else None."""
        fn = self.unit.fn
        wrappers = getattr(self.program, "folded_wrappers", ())
        if self.mode != "host" or self.home < 0 or fn is None or not wrappers:
            return None
        size = getattr(self.program, "sizes", {}).get(fn.code)
        if fn.code in wrappers and size and size <= 64:
            pass
        elif sym is None or sym not in wrappers or sym == fn.code:
            return None
        if not size:
            return None
        return f"openrac_call_in_copy({self.home}, {hx(self.entry_addr)}, {size}u, openrac_self_, {hx(target)})"

    def _code_literal(self, target: int) -> str:
        """A code address of the program this assembly is from, as the function reaches it."""
        if self.mode != "host":
            return hx(target)
        if self.home < 0:
            return f"OPENRAC_CODE({hx(target)})" if getattr(self.program, "relocate", None) else hx(target)
        level = self.unit._shared_level()
        if level is not None:
            return f"OPENRAC_LCODE({level}, {hx(target)})"
        return hx(target)

    def _fn_type(self, sig: Signature) -> str:
        ret = canon(sig.ret, self.program.typedefs)
        params = [canon(p, self.program.typedefs) for p in sig.params]
        plist = ", ".join(params) + (", ..." if sig.variadic else "")
        return f"{ret} (*)({plist or ('void' if sig.prototyped else '')})"

    def _args(self, sym: str, sig: Signature) -> list[str]:
        """The call's arguments from the registers, by the callee's prototype: integers from $a0-$t3
        and then the stack (a doubleword each from 0($sp)), floats from $f12-$f19. A variadic
        callee gets the integer registers its fixed parameters leave, as doublewords."""
        out = []
        ni = nf = 0
        for p in sig.params:
            c = canon(p, self.program.typedefs)
            if c == "float":
                if nf >= len(FARG_REGS):
                    raise AsmUnsupported(f"a call to {sym} with more than eight float arguments")
                out.append(f"f{FARG_REGS[nf]}")
                nf += 1
            elif c in INT_TYPES or c in WIDE or c == "double":
                reg = (self.R(ARG_REGS[ni]) if ni < len(ARG_REGS)
                       else f"openrac_ee_ld((openrac_u32)r29 + {8 * (ni - len(ARG_REGS))}u)")
                # A double is a doubleword in an integer register (the EE's soft float).
                out.append(f"openrac_ee_d({reg})" if c == "double" else f"({c}){reg}")
                ni += 1
            else:
                raise AsmUnsupported(f"a call to {sym}, which takes {c or 'a record'}")
        if sig.variadic:
            out += [f"(openrac_u64){self.R(r)}" for r in ARG_REGS[ni:]]
        return out

    def _invoke(self, call: str, ret: str | None) -> list[str]:
        pre = ["OPENRAC_ASM_CALL();", "OPENRAC_VU0_OUT();"]
        post = ["OPENRAC_VU0_IN();"]
        if ret == "void":
            return pre + [f"{call};"] + post
        if ret == "float":
            return pre + [f"f0 = {call};"] + post
        if ret in WIDE:
            return pre + [f"r2 = (openrac_u64)({call});"] + post
        if ret == "double":
            return pre + [f"r2 = openrac_ee_dbits({call});"] + post
        if ret in INT_TYPES:
            return pre + [f"r2 = {S32(f'({ret})({call})')};"] + post
        raise AsmUnsupported(f"a call whose result is {ret}")

    def indirect(self, value: str, at: int, float_result: bool = False, wide_result: bool = False) -> list[str]:
        """A call through a code address: the registers as eight integers (and eight floats when the
        function sets any argument float), the result as an integer, or a float when the code after
        reads $f0 before it writes it (or `float_result`: the callee's declaration returns one).
        `wide_result`: the callee's declaration returns 64 bits, which the result keeps (a TEX0 from
        GetEffectTex, func_L01_00404040, reached through a folded copy's own call: cut to 32 bits,
        its CLUT and height were lost and Annihilation Nation's mirror was drawn as stripes)."""
        floats = any(r in self._float_writes() or r in self._float_parameters() for r in FARG_REGS)
        ret = "float" if float_result or self._reads_f0_after(at) else "unsigned long long" if wide_result else "int"
        params = ["openrac_u64"] * 8 + (["float"] * 8 if floats else [])
        args = [self.R(r) for r in ARG_REGS] + ([f"f{r}" for r in FARG_REGS] if floats else [])
        t = f"{ret} (*)({', '.join(params)})"
        if self.mode != "host":
            return self._invoke(f"(({t})({value}))({', '.join(args)})", ret)
        cast = self._invoke(f"GFN({t}, {value})({', '.join(args)})", ret)
        # Where an argument's position decides its register whatever its type (Windows x64), the
        # cast would put the floats in the wrong place: each function's thunk gives the registers
        # to its parameters instead (guest.h, openrac_guest_call_regs).
        ints = ", ".join(f"(openrac_u64){self.R(r)}" for r in ARG_REGS)
        fl = ", ".join(f"f{r}" if floats else "0.0f" for r in FARG_REGS)
        regs = f"{value}, (const openrac_u64[8]){{{ints}}}, (const float[8]){{{fl}}}, &fr_"
        if ret == "float":
            body = f"{{ float fr_; (void)openrac_guest_call_regs({regs}, 1); f0 = fr_; }}"
        elif ret == "unsigned long long":
            body = f"{{ float fr_; r2 = (openrac_u64)openrac_guest_call_regs({regs}, 0); }}"
        else:
            body = f"{{ float fr_; r2 = {S32(f'(int)openrac_guest_call_regs({regs}, 0)')}; }}"
        thunked = ["OPENRAC_ASM_CALL();", "OPENRAC_VU0_OUT();", body, "OPENRAC_VU0_IN();"]
        if self.wide_register_calls:
            his = " ".join(f"openrac_guest_hi_args[{k}] = {self.H(r)};" for k, r in enumerate(ARG_REGS[:4]))
            before = f"{his} openrac_guest_hi_pending = 1;"
            after = "openrac_guest_hi_pending = 0;"
            return [before, "#if OPENRAC_REGS_THUNKS", *thunked, "#else", *cast, "#endif", after]
        return ["#if OPENRAC_REGS_THUNKS", *thunked, "#else", *cast, "#endif"]

    def _float_parameters(self) -> set:
        """The float argument registers the function's own parameters arrive in, which a call through
        a code address hands on untouched (asm.float_parameters_on_calls); none without the option."""
        if not self.float_parameters_on_calls or getattr(self, "sig", None) is None:
            return set()
        n = sum(1 for t in self.sig.params if canon(t, self.program.typedefs) == "float")
        return set(FARG_REGS[:n])

    def _float_writes(self) -> set:
        """The FPU registers the function writes anywhere."""
        if self._fregs is None:
            self._fregs = set()
            for a in self.reach:
                self._fregs |= rw(self.at(a))[3]
        return self._fregs

    def _reads_f0_after(self, at: int) -> bool:
        """Whether the code after the call at `at` reads $f0 before it writes it (in a straight line)."""
        for a in range(at + 8, at + 8 + 4 * 64, 4):
            ins = self.at(a)
            if ins is None or a not in self.reach:
                return False
            _, _, rf, wf = rw(ins)
            if 0 in rf:
                return True
            if 0 in wf or is_control(ins):
                return False
        return False

    def _reads_after(self, at: int, reg: int, fpu: bool) -> bool:
        """Whether the code after the call at `at` reads `reg` ($f`reg` with fpu) before it writes it
        or leaves the straight line (_reads_f0_after's walk, for either register file)."""
        for a in range(at + 8, at + 8 + 4 * 64, 4):
            ins = self.at(a)
            if ins is None or a not in self.reach:
                return False
            rg, wg, rf, wf = rw(ins)
            reads, writes = (rf, wf) if fpu else (rg, wg)
            if reg in reads:
                return True
            if reg in writes or is_control(ins):
                return False
        return False

    def _check_result(self, target: int, name: str | None, at: int) -> None:
        """asm.signature_check: the call at `at` is followed by a read of $f0 (or $v0) that the
        callee's signature does not set; the translation leaves the register as it was before the
        call (call(), _invoke)."""
        if not self.signature_check or self.mode != "host":
            return
        sym = self._callee(target, name)
        f = self.program.functions.get(sym) if sym else None
        if f is None or prototype(f, self.program.typedefs) is None:
            return  # through the registers (indirect)
        ret = canon(f.sig.ret, self.program.typedefs)
        out = self.dropped.setdefault(self.f.symbol, [])
        if ret != "float" and self._reads_after(at, 0, True):
            out.append(f"{sym}: the code reads $f0 after the call at {at:08X}, its signature returns {ret}")
        if ret in ("void", "float") and self._reads_after(at, 2, False):
            out.append(f"{sym}: the code reads $v0 after the call at {at:08X}, its signature returns {ret}")
        if not out:
            del self.dropped[self.f.symbol]

    def _live_arguments(self) -> tuple[int, int]:
        """The argument registers the code reads on some path from its entry before writing them,
        as (integer, float) counts up to the last read (_entry_reads's measure, by the flow rather
        than in address order up to the first call or return: a parameter used past an early return
        counts). A call reads its callee's parameters and ends every argument register's life; a
        store of one to the frame that is never loaded again is no use (_reloaded_argument)."""
        args_g, args_f = set(ARG_REGS), set(FARG_REGS)
        caller_saved_g = set(range(1, 16)) | {24, 25}
        caller_saved_f = set(range(0, 20))
        nodes: dict = {}   # node -> (reads_g, reads_f, writes_g, writes_f, successors)

        def effect(a: int):
            ins = self.at(a)
            rg, wg, rf, wf = rw(ins)
            if ins.op in ("sw", "sd", "sq", "swc1") and ins.rs == 29 and not self._reloaded_argument(ins):
                rg, rf = rg - {ins.rt}, rf - {ins.ft}
            return set(rg), set(rf), set(wg), set(wf)

        def call_node(a: int, target: int | None, then: list):
            rg: set = set()
            rf: set = set()
            sym = self._callee(target, self._operand_name(a)) if target is not None else None
            f = self.program.functions.get(sym) if sym else None
            if f is not None and f.sig.prototyped and not f.sig.variadic:
                ni = nf = 0
                for p in f.sig.params:
                    if canon(p, self.program.typedefs) == "float":
                        if nf < len(FARG_REGS):
                            rf.add(FARG_REGS[nf])
                        nf += 1
                    else:
                        if ni < len(ARG_REGS):
                            rg.add(ARG_REGS[ni])
                        ni += 1
            elif target is None and self.at(a).op == "jalr":
                rg = self._pointer_call_arguments(a)
            nodes[("call", a)] = (rg, rf, caller_saved_g, caller_saved_f, then)
            return ("call", a)

        for a in self.reach:
            if a in self.delay_only:
                continue
            ins = self.at(a)
            r_g, r_f, w_g, w_f = effect(a)
            if not is_control(ins):
                nodes[a] = (r_g, r_f, w_g, w_f, [a + 4])
                continue
            d = a + 4
            d_g, d_f, dw_g, dw_f = effect(d)
            op = ins.op
            after: list = []
            skip: list = []   # a likely branch not taken skips its delay slot
            if op in ("jal", *COND_CALLS) and a not in self.inline_calls:
                after = [call_node(a, ins.target, [a + 8])]
                if op in COND_CALLS:
                    after.append(a + 8)
                if op in COND_CALLS and ins.likely:
                    after, skip = [after[0]], [a + 8]
            elif op == "jalr":
                after = [call_node(a, None, [a + 8])]
            elif op in ("jal", *COND_CALLS):
                after = [ins.target]
            elif op == "jr":
                after = list(self.jtables.get(a, []))
            elif op == "j":
                after = [call_node(a, ins.target, [])] if self._is_other_entry(ins.target) else [ins.target]
            elif op in COP0_BRANCHES:
                after = [ins.target if op.startswith("bc0t") and self.dma_done else a + 8]
            else:
                uncond = op in ("beq", "beql") and ins.rs == ins.rt
                if ins.likely and not uncond:
                    after, skip = [ins.target], [a + 8]
                else:
                    after = [ins.target] if uncond else [ins.target, a + 8]
            nodes[("delay", a)] = (d_g, d_f, dw_g, dw_f, after)
            nodes[a] = (r_g, r_f, w_g, w_f, [("delay", a)] + skip)
        # Backward liveness on the argument registers alone, by a worklist from each node's
        # predecessors.
        preds: dict = {n: [] for n in nodes}
        for n, (*_, succ) in nodes.items():
            for s2 in succ:
                if s2 in preds:
                    preds[s2].append(n)
        live: dict = {n: (frozenset(), frozenset()) for n in nodes}
        todo = list(nodes)
        queued = set(todo)
        while todo:
            n = todo.pop()
            queued.discard(n)
            r_g, r_f, w_g, w_f, succ = nodes[n]
            out_g: set = set()
            out_f: set = set()
            for s2 in succ:
                if s2 in live:
                    out_g |= live[s2][0]
                    out_f |= live[s2][1]
            new = (frozenset((r_g | (out_g - w_g)) & args_g), frozenset((r_f | (out_f - w_f)) & args_f))
            if new != live[n]:
                live[n] = new
                for q in preds[n]:
                    if q not in queued:
                        queued.add(q)
                        todo.append(q)
        in_g, in_f = live.get(self.entry_addr, (set(), set()))
        ints = max([k + 1 for k, r in enumerate(ARG_REGS) if r in in_g & args_g], default=0)
        floats = max([k + 1 for k, r in enumerate(FARG_REGS) if r in in_f & args_f], default=0)
        return ints, floats

    STACK_LOADS = {"lb": 1, "lbu": 1, "lh": 2, "lhu": 2, "lw": 4, "lwu": 4, "ld": 8, "lwc1": 4, "lq": 16}

    def _stack_arguments(self) -> int:
        """The doublewords of its caller's outgoing arguments the code reads: the ninth integer
        argument on is passed in the caller's frame, a doubleword each from its $sp, which the
        callee reads above its own frame (the size its prologue takes off $sp). The levels' copy of
        frontbin's func_003910C0 (func_L01_00412C08, the help frames) reads a ninth from 0x60($sp)
        under a 0x60-byte frame; translated with eight, its C callers' ninth never reached it
        (#719). Only loads count, and only within eight doublewords of the frame."""
        frame = None
        most = 0
        for a in sorted(self.reach):
            ins = self.at(a)
            if ins is None:
                continue
            if frame is None and ins.op in ("addiu", "daddiu") and ins.rs == 29 and ins.rt == 29:
                if ins.imm >= 0:
                    return most
                frame = -ins.imm
                continue
            width = self.STACK_LOADS.get(ins.op)
            if width is None or ins.rs != 29:
                continue
            off = ins.imm - (frame or 0)
            if 0 <= off < 64:
                most = max(most, (off + width + 7) // 8)
        return most

    def _pointer_call_arguments(self, at: int) -> set:
        """The integer argument registers a call through a pointer at `at` takes, as far as its own
        set-up says: the callee is not known, but C fills the argument registers in order, so a
        call that sets $a2 (in the instructions since the last call or branch, or its delay slot)
        takes $a0 and $a1 too, whether it sets them or hands on its caller's (func_003EEBF8's frame
        widget draw hands its a1, the parent rectangle, to the rectangle callback at vtable +0x20).
        Only $a0-$a3 say so: the code keeps temporaries in $t0-$t3 as well. A call that sets no
        argument register reads none (it may take none)."""
        written: set = set(rw(self.at(at + 4))[1]) if self.at(at + 4) is not None else set()
        for a in range(at - 4, at - 4 * 32, -4):
            i = self.at(a)
            if i is None or is_control(i) or a in self.delay_only:
                break
            written |= set(rw(i)[1])
        written.discard(self.at(at).rs)  # the call's own address (func_003D2B78's jalr $a3 to its labels)
        last = max((k for k, r in enumerate(ARG_REGS[:4]) if r in written), default=-1)
        return set(ARG_REGS[:last + 1])

    def _check_arguments(self, sym: str, sig: Signature) -> None:
        """asm.signature_check: an argument register the code reads that the signature it is
        translated with does not take (_live_arguments)."""
        if sig.variadic:
            return
        ints, floats = self._live_arguments()
        if self.stack_arguments and (stacked := self._stack_arguments()):
            ints = max(ints, len(ARG_REGS) + stacked)
        have_f = sum(1 for p in sig.params if canon(p, self.program.typedefs) == "float")
        have_i = len(sig.params) - have_f
        if ints > have_i or floats > have_f:
            self.dropped.setdefault(sym, []).append(
                f"reads {ints} integer and {floats} float argument registers, its signature takes "
                f"{have_i} and {have_f}" + (" (integers past the eighth on the stack)" if ints > len(ARG_REGS) else ""))
        # A result the code never makes: no write of the register and no call whose result it could
        # hand on (a signature given by hand, asm.signatures, that is wrong the other way).
        if sym not in self.verified_signatures:
            return
        ret = canon(sig.ret, self.program.typedefs)
        calls = any((i := self.at(a)) is not None and i.op in ("jal", "jalr", "j", *COND_CALLS)
                    for a in self.reach)
        if ret == "float" and 0 not in self._float_writes() and not calls:
            self.dropped.setdefault(sym, []).append("returns float, and its code never sets $f0")
        if ret not in ("void", "float") and not calls and not any(
                2 in rw(self.at(a))[1] for a in self.reach):
            self.dropped.setdefault(sym, []).append(f"returns {ret}, and its code never sets $v0")

    def tail_call(self, target: int, name: str | None) -> list[str]:
        return self.call(target, name) + self.leave()

    def leave(self) -> list[str]:
        return ["OPENRAC_ASM_LEAVE();", "OPENRAC_VU0_OUT();", self._return_text()]

    def _return_text(self) -> str:
        ret = canon(self.sig.ret, self.program.typedefs)
        if ret == "void":
            return "return;"
        if ret == "float":
            return "return f0;"
        if ret == "gaddr":
            return "return (gaddr)r2;"
        if ret in INT_TYPES or ret in WIDE:
            return f"return ({ret})r2;"
        if ret == "double":
            return "return openrac_ee_d(r2);"
        raise AsmUnsupported(f"a result of type {ret}")

    # ---- One instruction ----

    def stmt(self, ins: asm.Insn) -> list[str]:
        op = ins.op
        R, W = self.R, self.W
        rs, rt, rd, sa, imm = ins.rs, ins.rt, ins.rd, ins.sa, ins.imm
        if op in SKIP:
            return []
        if self.microprogram(ins) is not None:
            return [f"{{ extern void {self.microprogram(ins)}(void); {self.microprogram(ins)}(); }}"]
        if op in UNSUPPORTED:
            raise AsmUnsupported(f"{UNSUPPORTED[op]} at {ins.addr:08X}")
        u32 = lambda x: f"(openrac_u32){x}"
        s32 = lambda x: f"(openrac_s32){x}"
        if op in ("addu", "add"):
            return W(rd, S32(f"{u32(R(rs))} + {u32(R(rt))}"))
        if op in ("subu", "sub"):
            return W(rd, S32(f"{u32(R(rs))} - {u32(R(rt))}"))
        if op in ("addiu", "addi"):
            if ins.reloc and ins.reloc[0] == "hi":
                raise AsmUnsupported(f"%hi in {op} at {ins.addr:08X}")
            if rs == 28 and rt != 28 and self.gp is not None and self.gp_addresses:
                # A datum's address formed from $gp: it moves between programs as a load through
                # $gp does (func_L01_003801B8's gp-0x110 in level 18's copy is another variable).
                return W(rt, S32(self.addr(ins)))
            return W(rt, S32(f"{u32(R(rs))} + {hx(imm)}"))
        if op in ("daddu", "dadd"):
            return W(rd, f"{R(rs)} + {R(rt)}")
        if op in ("dsubu", "dsub"):
            return W(rd, f"{R(rs)} - {R(rt)}")
        if op in ("daddiu", "daddi"):
            return W(rt, f"{R(rs)} + (openrac_u64)(openrac_s64){imm}")
        if op == "and":
            return W(rd, f"{R(rs)} & {R(rt)}")
        if op == "or":
            return W(rd, f"{R(rs)} | {R(rt)}")
        if op == "xor":
            return W(rd, f"{R(rs)} ^ {R(rt)}")
        if op == "nor":
            return W(rd, f"~({R(rs)} | {R(rt)})")
        if op == "andi":
            return W(rt, f"{R(rs)} & {hx(imm)}")
        if op == "ori":
            return W(rt, f"{R(rs)} | {hx(imm)}")
        if op == "xori":
            return W(rt, f"{R(rs)} ^ {hx(imm)}")
        if op == "lui":
            value = (ins.word & 0xFFFF) << 16
            if ins.reloc and ins.reloc[0] == "hi":
                moved = self._symbol_address(ins)
                if moved is not None:
                    return W(rt, S32(f"{hx(value)} + ({moved})"))
            return W(rt, S32(hx(value)))
        if op == "slt":
            return W(rd, f"(openrac_s64){R(rs)} < (openrac_s64){R(rt)}")
        if op == "sltu":
            return W(rd, f"{R(rs)} < {R(rt)}")
        if op == "slti":
            return W(rt, f"(openrac_s64){R(rs)} < {imm}")
        if op == "sltiu":
            return W(rt, f"{R(rs)} < (openrac_u64)(openrac_s64){imm}")
        if op == "sll":
            return W(rd, S32(f"{u32(R(rt))} << {sa}"))
        if op == "srl":
            return W(rd, S32(f"{u32(R(rt))} >> {sa}"))
        if op == "sra":
            return W(rd, S32(f"{s32(R(rt))} >> {sa}"))
        if op == "sllv":
            return W(rd, S32(f"{u32(R(rt))} << ({R(rs)} & 31)"))
        if op == "srlv":
            return W(rd, S32(f"{u32(R(rt))} >> ({R(rs)} & 31)"))
        if op == "srav":
            return W(rd, S32(f"{s32(R(rt))} >> ({R(rs)} & 31)"))
        if op in ("dsll", "dsll32"):
            return W(rd, f"{R(rt)} << {sa + (32 if op.endswith('32') else 0)}")
        if op in ("dsrl", "dsrl32"):
            return W(rd, f"{R(rt)} >> {sa + (32 if op.endswith('32') else 0)}")
        if op in ("dsra", "dsra32"):
            return W(rd, f"(openrac_u64)((openrac_s64){R(rt)} >> {sa + (32 if op.endswith('32') else 0)})")
        if op == "dsllv":
            return W(rd, f"{R(rt)} << ({R(rs)} & 63)")
        if op == "dsrlv":
            return W(rd, f"{R(rt)} >> ({R(rs)} & 63)")
        if op == "dsrav":
            return W(rd, f"(openrac_u64)((openrac_s64){R(rt)} >> ({R(rs)} & 63))")
        if op == "movz":
            return [] if rd == 0 else [f"if ({R(rt)} == 0) r{rd} = {R(rs)};"]
        if op == "movn":
            return [] if rd == 0 else [f"if ({R(rt)} != 0) r{rd} = {R(rs)};"]
        if op in ("mult", "multu", "mult1", "multu1"):
            one = "1" if op.endswith("1") else ""
            fn = "openrac_ee_multu" if op.startswith("multu") else "openrac_ee_mult"
            text = f"{fn}({R(rs)}, {R(rt)}, &lo{one}_, &hi{one}_)"
            return [f"r{rd} = {text};"] if rd else [f"{text};"]
        if op in ("madd", "maddu", "madd1", "maddu1"):
            one = "1" if op.endswith("1") else ""
            text = f"openrac_ee_madd({R(rs)}, {R(rt)}, &lo{one}_, &hi{one}_, {1 if 'u' in op else 0})"
            return [f"r{rd} = {text};"] if rd else [f"{text};"]
        if op in ("div", "divu", "div1", "divu1"):
            one = "1" if op.endswith("1") else ""
            fn = "openrac_ee_divu" if op.startswith("divu") else "openrac_ee_div"
            return [f"{fn}({R(rs)}, {R(rt)}, &lo{one}_, &hi{one}_);"]
        if op in ("mfhi", "mflo", "mfhi1", "mflo1"):
            return W(rd, f"{op[2:4]}{'1' if op.endswith('1') else ''}_")
        if op in ("mthi", "mtlo", "mthi1", "mtlo1"):
            return [f"{op[2:4]}{'1' if op.endswith('1') else ''}_ = {R(rs)};"]
        if op == "mfsa":
            return W(rd, "(openrac_u64)sa_")
        if op == "mtsa":
            return [f"sa_ = (openrac_u32){R(rs)} & 15u;"]
        if op == "mtsab":
            return [f"sa_ = ((openrac_u32){R(rs)} ^ {imm & 0xFFFF}u) & 15u;"]
        if op == "mtsah":
            return [f"sa_ = (((openrac_u32){R(rs)} ^ {imm & 0xFFFF}u) & 7u) * 2u;"]
        if op in ("break",):
            return [f"openrac_ee_break(\"{self.f.symbol}\", {hx(ins.addr)});"]
        if op in ("teq", "tne", "tge", "tgeu", "tlt", "tltu"):
            c = {"teq": "==", "tne": "!=", "tge": ">=", "tgeu": ">=", "tlt": "<", "tltu": "<"}[op]
            cast = "" if op.endswith("u") else "(openrac_s64)"
            return [f"if ({cast}{R(rs)} {c} {cast}{R(rt)}) openrac_ee_break(\"{self.f.symbol}\", {hx(ins.addr)});"]
        if op in ("teqi", "tnei", "tgei", "tgeiu", "tlti", "tltiu"):
            c = {"teqi": "==", "tnei": "!=", "tgei": ">=", "tgeiu": ">=", "tlti": "<", "tltiu": "<"}[op]
            if op.endswith("u"):
                cond = f"{R(rs)} {c} (openrac_u64)(openrac_s64){imm}"
            else:
                cond = f"(openrac_s64){R(rs)} {c} {imm}"
            return [f"if ({cond}) openrac_ee_break(\"{self.f.symbol}\", {hx(ins.addr)});"]
        # Loads and stores.
        loads = {"lb", "lbu", "lh", "lhu", "lw", "lwu", "ld"}
        if op in loads:
            return W(rt, f"openrac_ee_{op}({self.addr(ins)})")
        if op in ("lwl", "lwr", "ldl", "ldr"):
            return W(rt, f"openrac_ee_{op}({self.addr(ins)}, {R(rt)})")
        if op in ("sb", "sh", "sw", "sd", "swl", "swr", "sdl", "sdr"):
            store = "sw_dma" if op == "sw" and self.starts_dma else op
            return [f"openrac_ee_{store}({self.addr(ins)}, {R(rt)});"]
        if op == "lq":
            if rt == 0:
                return []
            return [f"openrac_ee_lq({self.addr(ins)}, &r{rt}, &h{rt});"]
        if op == "sq":
            return [f"openrac_ee_sq({self.addr(ins)}, {R(rt)}, {self.H(rt)});"]
        if op == "lwc1":
            return [f"f{ins.ft} = openrac_ee_lwc1({self.addr(ins)});"]
        if op == "swc1":
            return [f"openrac_ee_swc1({self.addr(ins)}, f{ins.ft});"]
        if op == "lqc2":
            if ins.ft == 0:
                return []
            a = self.addr(ins)
            return [f"t_ = {a} & ~15u;"] + [f"{self.vf(ins.ft, c)} = openrac_ee_lwc1(t_ + {4 * c}u);" for c in range(4)]
        if op == "sqc2":
            a = self.addr(ins)
            return [f"t_ = {a} & ~15u;"] + [f"openrac_ee_swc1(t_ + {4 * c}u, {self.vf(ins.ft, c)});" for c in range(4)]
        if op in MMI_OPS:
            if rd == 0:
                return []
            name = f"OPENRAC_MMI_{op.upper()}"
            return [f"q_ = openrac_ee_mmi({name}, openrac_ee_qof({R(rs)}, {self.H(rs)}), "
                    f"openrac_ee_qof({R(rt)}, {self.H(rt)}), {sa});", f"r{rd} = q_.lo;", f"h{rd} = q_.hi;"]
        if op == "qfsrv":
            if rd == 0:
                return []
            return [f"q_ = openrac_ee_qfsrv(openrac_ee_qof({R(rs)}, {self.H(rs)}), openrac_ee_qof({R(rt)}, {self.H(rt)}), sa_);",
                    f"r{rd} = q_.lo;", f"h{rd} = q_.hi;"]
        if op in ("pmfhi", "pmflo"):
            x = op[3:5]
            return [] if rd == 0 else [f"r{rd} = {x}_;", f"h{rd} = {x}1_;"]
        if op in ("pmthi", "pmtlo"):
            x = op[3:5]
            return [f"{x}_ = {R(rs)};", f"{x}1_ = {self.H(rs)};"]
        if op in ("pmfhl.lw", "pmfhl.uw"):
            if rd == 0:
                return []
            k = 0 if op == "pmfhl.lw" else 32
            return [f"r{rd} = ((lo_ >> {k}) & 0xFFFFFFFFull) | (((hi_ >> {k}) & 0xFFFFFFFFull) << 32);",
                    f"h{rd} = ((lo1_ >> {k}) & 0xFFFFFFFFull) | (((hi1_ >> {k}) & 0xFFFFFFFFull) << 32);"]
        if op in ("pmfhl.slw", "pmfhl.lh"):
            if rd == 0:
                return []
            fn = "openrac_ee_pmfhl_slw" if op == "pmfhl.slw" else "openrac_ee_pmfhl_lh"
            return [f"q_ = {fn}(openrac_ee_qof(lo_, lo1_), openrac_ee_qof(hi_, hi1_));",
                    f"r{rd} = q_.lo;", f"h{rd} = q_.hi;"]
        if op == "pmthl":
            # pmthl.lw: rs's words 0, 1, 2, 3 to LO 0, HI 0, LO 2, HI 2; the odd words stay.
            r, h = R(rs), self.H(rs)
            keep = "0xFFFFFFFF00000000ull"
            return [f"lo_ = (lo_ & {keep}) | ({r} & 0xFFFFFFFFull);",
                    f"hi_ = (hi_ & {keep}) | (({r} >> 32) & 0xFFFFFFFFull);",
                    f"lo1_ = (lo1_ & {keep}) | ({h} & 0xFFFFFFFFull);",
                    f"hi1_ = (hi1_ & {keep}) | (({h} >> 32) & 0xFFFFFFFFull);"]
        if op in PMUL_OPS:
            lines = ["qlo_ = openrac_ee_qof(lo_, lo1_);", "qhi_ = openrac_ee_qof(hi_, hi1_);",
                     f"q_ = openrac_ee_pmul(OPENRAC_PMUL_{op.upper()}, openrac_ee_qof({R(rs)}, {self.H(rs)}), "
                     f"openrac_ee_qof({R(rt)}, {self.H(rt)}), &qlo_, &qhi_);",
                     "lo_ = qlo_.lo;", "lo1_ = qlo_.hi;", "hi_ = qhi_.lo;", "hi1_ = qhi_.hi;"]
            if rd != 0 and not op.startswith("pdiv"):
                lines += [f"r{rd} = q_.lo;", f"h{rd} = q_.hi;"]
            return lines
        if op == "pmulth":
            # Eight signed halfword products: LO takes those of halfwords 0, 1, 4, 5, HI those of
            # 2, 3, 6, 7, a word each; rd the even ones (0, 2, 4, 6).
            def half(n: int, k: int) -> str:
                reg = self.R(n) if k < 4 else self.H(n)
                return f"(openrac_s32)(openrac_s16)({reg} >> {16 * (k % 4)})"
            p = [f"(openrac_u64)(openrac_u32)({half(rs, k)} * {half(rt, k)})" for k in range(8)]
            out = [f"lo_ = {p[0]} | ({p[1]} << 32);", f"hi_ = {p[2]} | ({p[3]} << 32);",
                   f"lo1_ = {p[4]} | ({p[5]} << 32);", f"hi1_ = {p[6]} | ({p[7]} << 32);"]
            if rd != 0:
                out += [f"r{rd} = (lo_ & 0xFFFFFFFFull) | (hi_ << 32);",
                        f"h{rd} = (lo1_ & 0xFFFFFFFFull) | (hi1_ << 32);"]
            return out
        if op == "pmfhl.sh":
            # LO's and HI's eight words, saturated to halfwords: LO 0, 1, HI 0, 1, LO 2, 3, HI 2, 3.
            if rd == 0:
                return []
            def sat(src: str, k: int) -> str:
                w = f"(openrac_s32)({src} >> {32 * k})"
                return f"(openrac_u64)(openrac_u16)({w} > 32767 ? 32767 : {w} < -32768 ? -32768 : {w})"
            low = [sat("lo_", 0), sat("lo_", 1), sat("hi_", 0), sat("hi_", 1)]
            high = [sat("lo1_", 0), sat("lo1_", 1), sat("hi1_", 0), sat("hi1_", 1)]
            return [f"r{rd} = " + " | ".join(f"({v} << {16 * k})" for k, v in enumerate(low)) + ";",
                    f"h{rd} = " + " | ".join(f"({v} << {16 * k})" for k, v in enumerate(high)) + ";"]
        # The FPU.
        if op.endswith(".s") or op in ("mtc1", "mfc1", "ctc1", "cfc1", "cvt.s.w"):
            return self.fpu(ins)
        if op in ("qmfc2", "qmtc2", "cfc2", "ctc2") or op.startswith("v"):
            return self.vu0(ins)
        raise AsmUnsupported(f"{op if op != '?' else 'an instruction it does not know'} at {ins.addr:08X}")

    def _f(self, kind: str, a: str, b: str) -> str:
        """a + b, a - b or a * b (kind "add", "sub", "mul"), in the host's floats or the EE's."""
        if self.ps2:
            return f"openrac_ps2_{kind}({a}, {b})"
        return f"{a} {dict(add='+', sub='-', mul='*')[kind]} {b}"

    def _fn(self, name: str, *args: str) -> str:
        """The divide, the roots, max and min: openrac_ee_<name>, or the EE's model of it."""
        if self.ps2:
            name = {"fdiv": "openrac_ps2_div", "sqrt": "openrac_ps2_sqrt", "rsqrt": "openrac_ps2_rsqrt",
                    "max": "openrac_ps2s_max", "min": "openrac_ps2s_min"}[name]
        else:
            name = f"openrac_ee_{name}"
        return f"{name}({', '.join(args)})"

    def fpu(self, ins: asm.Insn) -> list[str]:
        op, fs, ft, fd = ins.op, f"f{ins.fs}", f"f{ins.ft}", f"f{ins.fd}"
        f = self._f
        binary = {"add.s": "add", "sub.s": "sub", "mul.s": "mul"}
        if op in binary:
            return [f"{fd} = {f(binary[op], fs, ft)};"]
        if op == "div.s":
            return [f"{fd} = {self._fn('fdiv', fs, ft)};"]
        if op == "sqrt.s":
            return [f"{fd} = {self._fn('sqrt', ft)};"]
        if op == "rsqrt.s":
            return [f"{fd} = {self._fn('rsqrt', fs, ft)};"]
        if op == "abs.s":
            return [f"{fd} = openrac_ee_fabs({fs});"]
        if op == "neg.s":
            return [f"{fd} = openrac_ee_fneg({fs});"]
        if op == "mov.s":
            return [f"{fd} = {fs};"]
        if op == "max.s":
            return [f"{fd} = {self._fn('max', fs, ft)};"]
        if op == "min.s":
            return [f"{fd} = {self._fn('min', fs, ft)};"]
        if op == "adda.s":
            return [f"facc_ = {f('add', fs, ft)};"]
        if op == "suba.s":
            return [f"facc_ = {f('sub', fs, ft)};"]
        if op == "mula.s":
            return [f"facc_ = {f('mul', fs, ft)};"]
        if op == "madda.s":
            return [f"facc_ = {f('add', 'facc_', f('mul', fs, ft))};"]
        if op == "msuba.s":
            return [f"facc_ = {f('sub', 'facc_', f('mul', fs, ft))};"]
        if op == "madd.s":
            return [f"{fd} = {f('add', 'facc_', f('mul', fs, ft))};"]
        if op == "msub.s":
            return [f"{fd} = {f('sub', 'facc_', f('mul', fs, ft))};"]
        if op == "c.f.s":
            return ["fcc_ = 0;"]
        if op in ("c.eq.s", "c.lt.s", "c.le.s"):
            if self.ps2:
                return [f"fcc_ = openrac_ps2_{op[2:4]}({fs}, {ft});"]
            c = {"c.eq.s": "==", "c.lt.s": "<", "c.le.s": "<="}[op]
            return [f"fcc_ = {fs} {c} {ft};"]
        if op == "cvt.s.w":
            if self.ps2:
                return [f"{fd} = openrac_ps2_itof((openrac_s32)openrac_ee_bits({fs}));"]
            return [f"{fd} = (openrac_f32)(openrac_s32)openrac_ee_bits({fs});"]
        if op == "cvt.w.s":
            if self.ps2:
                return [f"{fd} = openrac_ee_f((openrac_u32)openrac_ps2_ftoi({fs}));"]
            return [f"{fd} = openrac_ee_f(openrac_ee_cvtws({fs}));"]
        if op == "mtc1":
            return [f"{fs} = openrac_ee_f({self.R(ins.rt)});"]
        if op == "mfc1":
            return self.W(ins.rt, S32(f"openrac_ee_bits({fs})"))
        if op == "cfc1":
            if ins.fs != 31:
                raise AsmUnsupported(f"cfc1 of FPU control register {ins.fs}")
            return self.W(ins.rt, "(openrac_u64)(fcc_ ? 0x00800000u : 0u)")
        if op == "ctc1":
            if ins.fs != 31:
                raise AsmUnsupported(f"ctc1 to FPU control register {ins.fs}")
            return [f"fcc_ = (int)(({self.R(ins.rt)} >> 23) & 1u);"]
        raise AsmUnsupported(f"{op} at {ins.addr:08X}")

    def vu0(self, ins: asm.Insn) -> list[str]:
        op = ins.op
        V = "OPENRAC_VU0->"
        vf = self.vf
        fs, ft, fd, bc = ins.fs, ins.ft, ins.fd, ins.bc
        comps = _comps(ins.dest)
        if op == "qmfc2":
            if ins.rt == 0:
                return []
            return [f"r{ins.rt} = (openrac_u64)openrac_ee_bits({vf(ins.rd, 0)}) | ((openrac_u64)openrac_ee_bits({vf(ins.rd, 1)}) << 32);",
                    f"h{ins.rt} = (openrac_u64)openrac_ee_bits({vf(ins.rd, 2)}) | ((openrac_u64)openrac_ee_bits({vf(ins.rd, 3)}) << 32);"]
        if op == "qmtc2":
            if ins.rd == 0:
                return []
            r, h = self.R(ins.rt), self.H(ins.rt)
            return [f"{vf(ins.rd, 0)} = openrac_ee_f({r});", f"{vf(ins.rd, 1)} = openrac_ee_f({r} >> 32);",
                    f"{vf(ins.rd, 2)} = openrac_ee_f({h});", f"{vf(ins.rd, 3)} = openrac_ee_f({h} >> 32);"]
        if op == "cfc2":
            n = ins.rd
            if n < 16:
                return self.W(ins.rt, "0" if n == 0 else f"(openrac_u64){V}vi[{n}]")
            if n in (20, 21, 22):
                reg = {20: "r", 21: "i", 22: "q"}[n]
                return self.W(ins.rt, S32(f"openrac_ee_bits({V}{reg})"))
            if n in (16, 17, 18):
                # STATUS, MAC, the clip flag (ee_ops.h): MAC and STATUS as this function's own
                # vector operations left them (self.vu_flags).
                return self.W(ins.rt, f"(openrac_u64){V}{ {16: 'status', 17: 'mac', 18: 'clip'}[n] }")
            raise AsmUnsupported(f"cfc2 of VU0 control register {n}")
        if op == "ctc2":
            n = ins.rd
            if n == 0:
                return []
            if n < 16:
                return [f"{V}vi[{n}] = (openrac_u32){self.R(ins.rt)} & 0xFFFFu;"]
            if n in (20, 21, 22):
                reg = {20: "r", 21: "i", 22: "q"}[n]
                return [f"{V}{reg} = openrac_ee_f({self.R(ins.rt)});"]
            if n == 16:
                return [f"{V}status = ({V}status & 0x3Fu) | ((openrac_u32){self.R(ins.rt)} & 0xFC0u);"]  # the sticky bits
            if n == 17:
                return []  # MAC is read only
            if n == 18:
                return [f"{V}clip = (openrac_u32){self.R(ins.rt)} & 0xFFFFFFu;"]
            if n in (27, 28, 31):
                return []  # resets and microprogram starts: no microprograms run here
            raise AsmUnsupported(f"ctc2 to VU0 control register {n}")
        if op in ("vwaitq", "vnop"):
            return []
        vi = lambda n: "0" if n & 15 == 0 else f"{V}vi[{n & 15}]"
        if op in ("vlqi", "vlqd", "vsqi", "vsqd", "vilwr", "viswr"):
            # Macro-mode loads and stores reach VU0's data memory (ee_ops.h), a quadword per vi.
            # vlq*: ft from (fs); vsq*: fs to (ft); vilwr / viswr: vi ft from / to (fs). The
            # increment comes after, the decrement before; both 16-bit, and vi0 stays 0.
            ptr = fs if op in ("vlqi", "vlqd", "vilwr", "viswr") else ft
            step = lambda d: [] if ptr & 15 == 0 else [f"{V}vi[{ptr & 15}] = ({V}vi[{ptr & 15}] {d} 1u) & 0xFFFFu;"]
            lines = step("-") if op in ("vlqd", "vsqd") else []
            lines.append(f"t_ = openrac_vu0_addr({vi(ptr)});")
            for c in comps:
                if op in ("vlqi", "vlqd") and ft != 0:
                    lines.append(f"{vf(ft, c)} = openrac_ee_lwc1(t_ + {4 * c}u);")
                elif op in ("vsqi", "vsqd"):
                    lines.append(f"openrac_ee_swc1(t_ + {4 * c}u, {vf(fs, c)});")
                elif op == "vilwr" and ft & 15 != 0:
                    lines.append(f"{V}vi[{ft & 15}] = (openrac_u32)openrac_ee_lwu(t_ + {4 * c}u) & 0xFFFFu;")
                elif op == "viswr":
                    lines.append(f"openrac_ee_sw(t_ + {4 * c}u, {vi(ft)} & 0xFFFFu);")
            if op.endswith("i"):
                lines += step("+")
            return lines
        if op == "vclipw":
            return [f"openrac_vu0_clip({V[:-2]}, {vf(fs, 0)}, {vf(fs, 1)}, {vf(fs, 2)}, {vf(ft, 3)});"]

        def store(dst, values: dict[int, str], flags: bool = False) -> list[str]:
            # Every operand read before any field is written: dst may be an operand.
            lines = [f"v_[{c}] = {values[c]};" for c in values]
            if flags and self.vu_flags:
                # An FMAC operation: MAC from the fields it wrote (the others' flags are clear).
                lines.append("m_ = " + " | ".join(f"openrac_vu0_mac(v_[{c}], {c})" for c in values) + ";")
                lines.append(f"openrac_vu0_set_mac({V[:-2]}, m_);")
            result = (lambda c: f"openrac_ee_vu_flush(v_[{c}])") if flags and self.vu_flush else (lambda c: f"v_[{c}]")
            for c in values:
                if dst == "acc":
                    lines.append(f"{V}acc[{c}] = {result(c)};")
                elif dst != 0:
                    lines.append(f"{vf(dst, c)} = {result(c)};")
            return lines

        acc = lambda c: f"{V}acc[{c}]"
        f = self._f
        m = re.fullmatch(r"v(add|sub|mul|max|mini|madd|msub)(a?)(bc|q|i)?", op)
        if m:
            kind, to_acc, src = m.groups()
            dst = "acc" if to_acc else fd
            vals = {}
            for c in comps:
                b = vf(ft, bc) if src == "bc" else f"{V}q" if src == "q" else f"{V}i" if src == "i" else vf(ft, c)
                a = vf(fs, c)
                if kind in ("add", "sub", "mul"):
                    vals[c] = f(kind, a, b)
                elif kind == "max":
                    vals[c] = self._fn("max", a, b)
                elif kind == "mini":
                    vals[c] = self._fn("min", a, b)
                elif kind == "madd":
                    vals[c] = f("add", acc(c), f("mul", a, b))
                elif kind == "msub":
                    vals[c] = f("sub", acc(c), f("mul", a, b))
            return store(dst, vals, flags=kind not in ("max", "mini"))
        if op in ("vopmula", "vopmsub"):
            rot_s, rot_t = [1, 2, 0], [2, 0, 1]
            vals = {}
            for c in comps:
                if c == 3:
                    continue
                prod = f("mul", vf(fs, rot_s[c]), vf(ft, rot_t[c]))
                vals[c] = prod if op == "vopmula" else f("sub", acc(c), prod)
            return store("acc" if op == "vopmula" else fd, vals, flags=True)
        if op in ("vmove", "vabs", "vmr32"):
            vals = {}
            for c in comps:
                if op == "vmove":
                    vals[c] = vf(fs, c)
                elif op == "vabs":
                    vals[c] = f"openrac_ee_fabs({vf(fs, c)})"
                else:
                    vals[c] = vf(fs, (c + 1) % 4)
            return store(ft, vals)
        m = re.fullmatch(r"v(ftoi|itof)(0|4|12|15)", op)
        if m:
            kind, frac = m.group(1), int(m.group(2))
            vals = {}
            for c in comps:
                if self.ps2:
                    k = f"openrac_ee_f({(127 + frac if kind == 'ftoi' else 127 - frac) << 23:#010x}u)"
                    if kind == "ftoi":
                        x = vf(fs, c) if frac == 0 else f("mul", vf(fs, c), k)
                        vals[c] = f"openrac_ee_f((openrac_u32)openrac_ps2_ftoi({x}))"
                    else:
                        x = f"openrac_ps2_itof((openrac_s32)openrac_ee_bits({vf(fs, c)}))"
                        vals[c] = x if frac == 0 else f("mul", x, k)
                elif kind == "ftoi":
                    vals[c] = f"openrac_ee_f(openrac_ee_ftoi({vf(fs, c)}, {frac}))"
                else:
                    vals[c] = f"openrac_ee_itof(openrac_ee_bits({vf(fs, c)}), {frac})"
            return store(ft, vals)
        if op in ("vdiv", "vsqrt", "vrsqrt"):
            s, t = ("0.0f" if op == "vsqrt" else vf(fs, bc)), vf(ft, ins.ftf)
            flags = ([f"openrac_vu0_set_div({V[:-2]}, {('vdiv', 'vsqrt', 'vrsqrt').index(op)}, {s}, {t});"]
                     if self.vu_flags else [])
            if op == "vdiv":
                return flags + [f"{V}q = {self._fn('fdiv', s, t)};"]
            if op == "vsqrt":
                return flags + [f"{V}q = {self._fn('sqrt', t)};"]
            return flags + [f"{V}q = {self._fn('rsqrt', s, t)};"]
        if op == "vmtir":
            return [] if ft & 15 == 0 else [f"{V}vi[{ft & 15}] = openrac_ee_bits({vf(fs, bc)}) & 0xFFFFu;"]
        if op == "vmfir":
            vi = "0" if fs & 15 == 0 else f"{V}vi[{fs & 15}]"
            return store(ft, {c: f"openrac_ee_f((openrac_u32)(openrac_s32)(openrac_s16)(openrac_u16){vi})" for c in comps})
        if op in ("viadd", "visub", "viand", "vior", "viaddi"):
            vi = lambda n: "0" if n & 15 == 0 else f"{V}vi[{n & 15}]"
            if op == "viaddi":
                dst, value = ft, f"{vi(fs)} + (openrac_u32)({ins.imm})"
            else:
                o = {"viadd": "+", "visub": "-", "viand": "&", "vior": "|"}[op]
                dst, value = fd, f"{vi(fs)} {o} {vi(ft)}"
            return [] if dst & 15 == 0 else [f"{V}vi[{dst & 15}] = ({value}) & 0xFFFFu;"]
        raise AsmUnsupported(f"VU0 {op} at {ins.addr:08X}")

    # ---- The whole function ----

    def _assemble(self, f: Function, sig: Signature, body: list[str]) -> str:
        p = self.program
        fake = Function(f.symbol, sig)
        proto = prototype(fake, p.typedefs)
        if proto is None:
            raise AsmUnsupported("its prototype has a type the port's prototypes cannot write")
        if sig.variadic:
            raise AsmUnsupported("it takes variable arguments")
        if f.symbol in p.wrap:
            proto = proto.replace(f" {cname(f.symbol)}(", f" {cname(f.symbol)}__game(", 1)
        text = "\n".join(body)
        gprs = sorted({int(m) for m in re.findall(r"\br(\d+)\b", text)} | {29, 31})
        highs = sorted({int(m) for m in re.findall(r"\bh(\d+)\b", text)})
        fprs = sorted({int(m) for m in re.findall(r"\bf(\d+)\b", text)})
        decl = []
        if self.mode == "host" and getattr(self.unit.fn, "uses_self", False):
            decl.append(f"    const gaddr openrac_self_ = openrac_guest_entry((openrac_host_fn){cname(f.symbol)});")
        frame = 64 + sum(-i.imm for i in map(self.at, self.reach) if i.op in ("addiu", "daddiu") and i.rs == 29
                         and i.rt == 29 and i.imm < 0)
        decl.append(f"    OPENRAC_ASM_FRAME({frame});")
        decl.append("    openrac_u64 " + ", ".join(f"r{n} = 0" for n in gprs) + ";")
        if highs:
            decl.append("    openrac_u64 " + ", ".join(f"h{n} = 0" for n in highs) + ";")
        if fprs:
            decl.append("    openrac_f32 " + ", ".join(f"f{n} = 0.0f" for n in fprs) + ";")
        extras = [("openrac_u64", "lo_"), ("openrac_u64", "hi_"), ("openrac_u64", "lo1_"), ("openrac_u64", "hi1_"),
                  ("int", "c_"), ("int", "fcc_"), ("openrac_f32", "facc_"), ("openrac_u32", "t_"), ("openrac_u32", "j_"),
                  ("openrac_u32", "sa_"), ("openrac_ee_q", "q_"), ("openrac_ee_q", "qlo_"), ("openrac_ee_q", "qhi_"),
                  ("openrac_f32", "v_[4]"), ("openrac_u32", "m_")]
        for t, name in extras:
            bare = name.split("[")[0]
            if re.search(r"\b" + re.escape(bare) + r"\b", text):
                init = " = 0" if t not in ("openrac_ee_q",) and "[" not in name else ""
                decl.append(f"    {t} {name}{init};")
        setup = ["    r29 = OPENRAC_ASM_SP();"]
        if self.gp is not None and re.search(r"\br28\b", text):
            setup.append(f"    r28 = {S32(hx(self.gp))};")
        setup.append("    OPENRAC_VU0_IN();")
        if self.wide_register_calls:
            # The upper halves a call through a code address handed on (indirect), when this
            # function reads them before it writes them.
            taken = [f"h{r} = openrac_guest_hi_args[{k}];" for k, r in enumerate(ARG_REGS[:4]) if r in highs]
            if taken:
                setup.append(f"    if (openrac_guest_hi_pending) {{ {' '.join(taken)} }}")
            setup.append("    openrac_guest_hi_pending = 0;")
        ni = nf = 0
        stacked: list[str] = []
        for k, t in enumerate(sig.params):
            c = canon(t, p.typedefs)
            if c == "float":
                if nf < len(FARG_REGS) and f"f{FARG_REGS[nf]}" in [f"f{n}" for n in fprs]:
                    setup.append(f"    f{FARG_REGS[nf]} = a{k};")
                nf += 1
            elif c in INT_TYPES or c in WIDE or c == "double":
                value = (f"openrac_ee_dbits(a{k})" if c == "double" else
                         f"(openrac_u64)a{k}" if c in WIDE else S32(f"(openrac_s32)a{k}"))
                if ni >= len(ARG_REGS):
                    # Past the eighth: where the EE's caller put it, a doubleword each above $sp.
                    stacked.append(f"    openrac_ee_sd((openrac_u32)r29 + {8 * (ni - len(ARG_REGS))}u, {value});")
                elif ARG_REGS[ni] in gprs:
                    setup.append(f"    r{ARG_REGS[ni]} = {value};")
                ni += 1
            else:
                raise AsmUnsupported(f"a parameter of type {c}")
        if stacked:
            # A whole number of quadwords: $sp stays aligned, as the quadword stores under it need
            # (func_L00_0032A258's three took it to 8 mod 16, a movaps fault on x86-64).
            room = (8 * len(stacked) + 15) & ~15
            setup[1:1] = [f"    r29 = {S32(f'(openrac_u32)r29 - {room}u')};"] + stacked
        lines = [f"/* {f.symbol}: translated from its assembly ({self.where}) */", proto + " {"]
        lines += decl + setup
        lines += ["    " + s if not s.startswith("L_") else s for s in body]
        lines.append("}")
        return "\n".join(lines) + "\n"


# ---- Which registers an instruction reads and writes (for the signature and the calls) ----

def rw(i: asm.Insn) -> tuple[set, set, set, set]:
    op = i.op
    g_r: set = set()
    g_w: set = set()
    f_r: set = set()
    f_w: set = set()
    three = {"addu", "add", "subu", "sub", "daddu", "dadd", "dsubu", "dsub", "and", "or", "xor", "nor", "slt",
             "sltu", "sllv", "srlv", "srav", "dsllv", "dsrlv", "dsrav", "movz", "movn"} | MMI_OPS | {"qfsrv"}
    if op in three:
        g_r |= {i.rs, i.rt}
        g_w.add(i.rd)
    elif op in ("addiu", "addi", "daddiu", "daddi", "andi", "ori", "xori", "slti", "sltiu"):
        g_r.add(i.rs)
        g_w.add(i.rt)
    elif op == "lui":
        g_w.add(i.rt)
    elif op in ("sll", "srl", "sra", "dsll", "dsrl", "dsra", "dsll32", "dsrl32", "dsra32"):
        g_r.add(i.rt)
        g_w.add(i.rd)
    elif op in ("mult", "multu", "mult1", "multu1", "madd", "maddu", "madd1", "maddu1"):
        g_r |= {i.rs, i.rt}
        g_w.add(i.rd)
    elif op in ("div", "divu", "div1", "divu1", "beq", "bne", "beql", "bnel"):
        g_r |= {i.rs, i.rt}
    elif op in ("blez", "bgtz", "bltz", "bgez", "blezl", "bgtzl", "bltzl", "bgezl", "jr", "mthi", "mtlo",
                "mthi1", "mtlo1", "mtsa", "mtsab", "mtsah", "pmthi", "pmtlo"):
        g_r.add(i.rs)
    elif op == "jalr":
        g_r.add(i.rs)
        g_w.add(i.rd)
    elif op in ("mfhi", "mflo", "mfhi1", "mflo1", "mfsa", "pmfhi", "pmflo", "pmfhl.lw", "pmfhl.uw", "pmfhl.sh",
                "pmfhl.slw", "pmfhl.lh"):
        g_w.add(i.rd)
    elif op == "pmulth" or op in PMUL_OPS:
        g_r |= {i.rs, i.rt}
        if not op.startswith("pdiv"):
            g_w.add(i.rd)
    elif op == "pmthl":
        g_r.add(i.rs)
    elif op in COND_CALLS:
        g_r.add(i.rs)
        g_w.add(31)
    elif op in ("lb", "lbu", "lh", "lhu", "lw", "lwu", "ld", "lq"):
        g_r.add(i.rs)
        g_w.add(i.rt)
    elif op in ("lwl", "lwr", "ldl", "ldr"):
        g_r.add(i.rs)   # rt is merged into, its old bytes overwritten by the pair: not an input
        g_w.add(i.rt)
    elif op in ("sb", "sh", "sw", "sd", "sq", "swl", "swr", "sdl", "sdr"):
        g_r |= {i.rs, i.rt}
    elif op == "lwc1":
        g_r.add(i.rs)
        f_w.add(i.ft)
    elif op == "swc1":
        g_r.add(i.rs)
        f_r.add(i.ft)
    elif op in ("lqc2", "sqc2"):
        g_r.add(i.rs)
    elif op == "mtc1":
        g_r.add(i.rt)
        f_w.add(i.fs)
    elif op == "mfc1":
        f_r.add(i.fs)
        g_w.add(i.rt)
    elif op in ("ctc1", "ctc2", "qmtc2"):
        g_r.add(i.rt)
    elif op in ("cfc1", "cfc2", "qmfc2"):
        g_w.add(i.rt)
    elif op in ("add.s", "sub.s", "mul.s", "div.s", "max.s", "min.s", "rsqrt.s"):
        f_r |= {i.fs, i.ft}
        f_w.add(i.fd)
    elif op in ("madd.s", "msub.s"):
        f_r |= {i.fs, i.ft}
        f_w.add(i.fd)
    elif op in ("adda.s", "suba.s", "mula.s", "madda.s", "msuba.s", "c.eq.s", "c.lt.s", "c.le.s"):
        f_r |= {i.fs, i.ft}
    elif op == "sqrt.s":
        f_r.add(i.ft)
        f_w.add(i.fd)
    elif op in ("abs.s", "neg.s", "mov.s", "cvt.s.w", "cvt.w.s"):
        f_r.add(i.fs)
        f_w.add(i.fd)
    elif op in ("teq", "tne", "tge", "tgeu", "tlt", "tltu"):
        g_r |= {i.rs, i.rt}
    elif op in ("teqi", "tnei", "tgei", "tgeiu", "tlti", "tltiu"):
        g_r.add(i.rs)
    if op == "jal":
        g_w.add(31)
    g_r.discard(0)
    g_w.discard(0)
    return g_r, g_w, f_r, f_w


# ---- The program's part: which stubs, and the files ----

@dataclass
class Stage:
    """The translations of one program, and what could not be translated, for the report."""
    translated: list[Result] = field(default_factory=list)
    unsupported: dict[str, str] = field(default_factory=dict)
    without_asm: list[str] = field(default_factory=list)
    translator: "Translator | None" = None    # for the console's form (write_console)
    found: dict = field(default_factory=dict)  # name -> (program, address) of its entry
    retail: str = ""                           # the retail program read for instructions, if any
    shadows: dict = field(default_factory=dict)   # alias -> its translation's C (or None), "shadows"
    shadow_why: dict = field(default_factory=dict)  # alias -> why it has none
    inherited: list[str] = field(default_factory=list)  # results_before_calls's names
    external: list[str] = field(default_factory=list)  # built from the disc apart (asmext.py)
    program: "Program | None" = None             # for the external functions' trampolines
    c_drops: dict = field(default_factory=dict)  # asm.signature_check over the C (c_argument_drops)
    c_reads: dict = field(default_factory=dict)  # name -> [integer, float] argument registers its code reads
    params_from_code: list = field(default_factory=list)  # asm.params_from_code's additions


def asm_config(cfg: dict) -> dict | None:
    a = cfg.get("asm")
    if not a or not a.get("enabled", False):
        return None
    return a


def retail_source(a: dict, program: Program, base: Path) -> "retail.Retail | None":
    """The retail program hostgen.json's "asm" names, when it is there: "retail": {"image": ISO}
    (the player's disc image) or {"exe": ELF, "dump": level dumps}, relative to the decompilation
    the assembly is read from (where its setup puts them) unless absolute; hostgen's
    --retail-image, --retail-exe and --retail-dump give them instead."""
    r = a.get("retail") or {}
    low = (getattr(program, "relocate", None) or [0x15F000])[0]
    path = lambda key: (base / r[key]) if r.get(key) else None
    image, exe, dump = path("image"), path("exe"), path("dump")
    found = None
    if image is not None and image.is_file() and r.get("executable"):
        # "executable": another program of the disc than the one SYSTEM.CNF boots, read as the
        # executable (Up Your Arsenal's multiplayer, I5BOOTN.ELF: a loader carrying its game packed,
        # as the boot executable does); the levels are read as for the boot executable.
        found = retail.Retail.from_image(image, low, executable=r["executable"])
    elif image is not None and image.is_file():
        found = retail.Retail.from_image(image, low)
    elif exe is not None and exe.is_file():
        found = retail.Retail.from_files(exe, dump if dump is not None and dump.is_dir() else None, low)
    # "programs": {NAME: OVERLAY}, the disc's other programs read as hostgen's overlays (Up Your
    # Arsenal's frontbin, the "groups" overlay its src/frontbin is).
    if found is not None:
        found.place_named(r.get("programs", {}))
        # "levels": only these levels' programs are read as this program's (the multiplayer
        # executable's maps; the others run on the boot executable's code).
        if r.get("levels") is not None:
            keep = {int(n) for n in r["levels"]}
            found.levels = {n: parts for n, parts in found.levels.items() if n in keep}
    if r.get("required"):
        retail_required(r, image if image is not None else exe, found, program)
    return found


def retail_required(r: dict, named: Path | None, found: "retail.Retail | None", program: Program) -> None:
    """"retail": {"required": true}: a game whose code is mostly translated from the disc (Up Your
    Arsenal's: its boot program is the loader's packed payload, which no other input holds) does
    not build without it, as a stale derived table no longer builds into silent stubs (rac2's
    #418): with no disc, or another game's, nearly every function the game runs would be a stub
    that does nothing, and the program would stop at its first frame. A build with no disc on
    purpose (CI) sets OPENRAC_HOSTGEN_NO_DISC=1 and gets the stubs."""
    if os.environ.get("OPENRAC_HOSTGEN_NO_DISC") == "1":
        return
    entry = getattr(program, "entry_address", None)
    if found is None:
        why = f"{named} is not there" if named is not None else "no disc image is named"
    elif entry is not None and found.entry != entry:
        why = f"{named} is not this game's disc (its entry is {found.entry:#010x}, not {entry:#010x})"
    else:
        return
    print(f"hostgen: error: the retail program to translate the functions with no C from is "
          f"required, and {why}. Link the disc image there (the decompilation's setup does), pass "
          f"--retail-image (OPENRAC_<ID>_RETAIL_IMAGE), or set OPENRAC_HOSTGEN_NO_DISC=1 to build "
          f"with stubs on purpose.", file=sys.stderr)
    raise SystemExit(1)


def file_names(cfg: dict) -> list[str]:
    a = asm_config(cfg)
    if not a:
        return []
    names = [f"asm_{k}.c" for k in range(int(a.get("files", 8)))]
    return names + (["asm_shadows.c"] if a.get("shadows") else [])


def translate_shadows(stage: Stage, program: Program, a: dict) -> None:
    """hostgen.json "asm" "shadows" ({alias: function}): the port's own C for a function the
    decompilation keeps as assembly (a host function), translated from that assembly as well, under
    the name openrac_shadow_<alias>, so the port can run both and compare them (rac1's collision
    kernels, rac1/host/collision.c), computing its floats as the EE does (openrac/ps2float.h), as
    that C does. Never called by the game; with no assembly there is none."""
    tr = stage.translator
    if tr is not None:
        tr.ps2 = True
    for alias, sym in sorted(a.get("shadows", {}).items()):
        f = program.functions.get(sym)
        if tr is None or f is None:
            stage.shadow_why[alias] = "no assembly read" if tr is None else f"no function {sym}"
            continue
        hit = tr.find(f)
        if hit is None:
            stage.shadow_why[alias] = f"no assembly for {sym}"
            continue
        r = tr.translate(f, hit)
        if r.text is None:
            stage.shadow_why[alias] = r.reason
            continue
        head = f" {cname(sym)}("
        if head not in r.text:
            stage.shadow_why[alias] = f"no definition of {cname(sym)} in its translation"
            continue
        stage.shadows[alias] = r.text.replace(head, f" openrac_shadow_{alias}(", 1)


def apply_signatures(program: Program, a: dict, discovered: bool, host_text: str = "") -> None:
    """hostgen.json's "asm" "signatures": each function's verified prototype, a C function type,
    in place of the one its declarations or registers give. Only for functions with neither C nor
    a host definition (its C left out for its assembly, "from_assembly", or none at all). Without
    discovery (no retail program read), a name the program does not have is an entry only
    discovery finds: passed over, unless the port's own C (`host_text`) calls it (rac3's
    multiplayer HUD sprite, mp_func_00284F80, calls mp_func_00284D30), which then compiles against
    the same prototype, a stub's, as with the disc."""
    for sym, text in a.get("signatures", {}).items():
        f = program.functions.get(sym)
        if f is None and not discovered:
            if not host_text or not re.search(r"\b" + re.escape(cname(sym)) + r"\b", host_text):
                continue
            program.functions[sym] = Function(sym, Signature.of(ctype.parse(text)), None, asm=True)
            continue
        if f is None or sym in program.host:
            raise ValueError(f"assembly signature for {sym}: expected an assembly-only function")
        if f.unit is not None:
            # The decomp matched it since the signature was written: its C gives the prototype, so
            # the entry is stale rather than wrong. Say so, and leave the function to its C.
            print(f"hostgen: warning: {sym} has C now; its \"asm\" \"signatures\" entry is not used "
                  f"and can be removed from hostgen.json", file=sys.stderr)
            continue
        f.sig = Signature.of(ctype.parse(text))


def run(program: Program, cfg: dict, source: Path, host_text: str = "") -> tuple[Stage, dict[str, Result]]:
    """Reads the assembly (when it is there), and translates every stub it has assembly for.
    The functions translated are given C (Function.unit "<asm>") and the signature their
    translation is written with, before the units are translated, so calls to them are matched
    to it. Returns the stage's report and the translations by name."""
    stage = Stage()
    a = asm_config(cfg)
    if a is None:
        return stage, {}
    # The assembly is where the game's setup wrote it: the decompilation, or another copy of it.
    base = Path(a["source"]) if a.get("source") else source
    dirs = [base / d for d in a.get("dirs", [])]
    gp = int(str(a["gp"]), 0) if a.get("gp") is not None else None
    tr = Translator(program, None, gp, "host", base)
    tr.gp_addresses = bool(a.get("gp_addresses"))
    tr.forward_unprototyped = bool(a.get("forward_unprototyped"))
    tr.in_place = frozenset(a.get("in_place", []))
    tr.dma_stores = bool(a.get("dma_stores"))
    tr.wide_register_calls = bool(a.get("wide_register_calls"))
    tr.fit_declarations = bool(a.get("fit_declarations"))
    tr.register_jumps = bool(a.get("register_jumps"))
    tr.input_idioms = bool(a.get("input_idioms"))
    tr.dma_done = bool(a.get("dma_done"))
    tr.vu_flush = bool(a.get("vu_flush"))
    tr.signature_check = bool(a.get("signature_check"))
    tr.result_flow = bool(a.get("result_flow"))
    tr.live_arguments = bool(a.get("live_arguments"))
    tr.wide_call_results = bool(a.get("wide_call_results"))
    tr.float_parameters_on_calls = bool(a.get("float_parameters_on_calls"))
    tr.stack_arguments = bool(a.get("stack_arguments"))
    tr.verified_signatures = frozenset(a.get("signatures", {}))
    tr.host_text = host_text
    index = asm.read_index(dirs, tr.overlay_of)
    tr.index = index
    # The retail program's own instructions (retail.py), where the player's executable and level
    # programs are given: everything the assembly files do not have, or all of it without them.
    source_of = retail_source(a, program, base)
    if source_of is not None:
        if a.get("discover_entries"):
            # JAL and addresses formed for callbacks identify entries missed
            # by catalogue heuristics. Plain J targets include internal
            # labels, so they must not become independent functions here.
            # "discover_levels": the levels whose programs have no catalogue (a second executable's
            # maps: the catalogue's places are the boot executable's), named func_Lnn_ADDRESS.
            levels = {int(n) for n in a.get("discover_levels", [])}
            found_entries = set(source_of.called(include_jumps=False))
            if a.get("discover_pointers"):
                # The entries only a table in the data names (vtables, update functions): a word in
                # a program's data that is an address in its code just after a function's end.
                found_entries |= source_of.pointed(levels, found_entries)
            for home, address in sorted(found_entries):
                if address % 4 or (home, address) in program.by_place:
                    continue
                if home < 0:
                    name = f"func_{address:08X}"
                elif home in levels:
                    name = f"func_L{home:02d}_{address:08X}"
                else:
                    continue
                program.functions.setdefault(name, Function(name,
                    Signature(ctype.Base("int"), (), False, False), asm=True))
            program.index_places()
        from_retail = retail.RetailCode(source_of, retail.starts_of(program), source_of.low)
        from_retail.follow_far = bool(a.get("follow_saved_registers"))
        from_retail.pair_moves = bool(a.get("pair_through_moves"))
        index.code = retail.CombinedCode(index.code, from_retail)
        index.called |= source_of.called()
        stage.retail = source_of.label
    elif not index.blocks:
        # No instructions to translate (a build without the disc): the functions keep their
        # stubs, but each with its verified prototype all the same, so that the port's own C that
        # wraps one (a "wrap" row, calling NAME__game) is compiled against one signature whether
        # the build reads the disc or not. Entries only discovery would add are not there.
        apply_signatures(program, a, discovered=False, host_text=host_text)
        # With the port's interface ("interface", asmext.py): what the disc would have given, the
        # translated functions called through slots the part built from the disc fills.
        if a.get("interface_path") and Path(a["interface_path"]).is_file():
            stage.external = asmext.apply(program, asmext.read(Path(a["interface_path"])))
            stage.program = program
            # As below: a C wrapper's result from one of them, known now.
            if a.get("results_before_calls"):
                stage.inherited = inherit_results(program)
            stage.retail = "the part built from the disc (asmext.py)"
        return stage, {}
    # Apply after entry discovery: a verified prototype can name an entry
    # missing from the catalogue. Register use alone cannot distinguish a
    # packed 64-bit descriptor from an integer.
    apply_signatures(program, a, discovered=True)
    # A stub whose place has C under another spelling (func_0020C828 for FUN_0020c828) is that C
    # already: calls to it go there (Program.canonical).
    # "leave": routines kept as their stubs on purpose, each with why (a fragment of another routine
    # that reads its caller's registers, which a C signature cannot give it): their translation would
    # run on registers it does not have. The stub says so if anything ever reaches it.
    leave = set(a.get("leave", {}))
    stubs = [f for f in program.stubs() if f.symbol not in program.host and program.canonical(f.symbol) == f.symbol
             and f.symbol not in leave]
    found = {}
    for k, f in enumerate(stubs):
        if PROGRESS is not None:
            PROGRESS("find", k, len(stubs))
        hit = tr.find(f)
        if hit is None:
            stage.without_asm.append(f.symbol)
        else:
            found[f.symbol] = hit
    # The signatures first (a function declared nowhere gets the one its registers say), then
    # the places, so calls between translated functions find each other.
    def prepare_all() -> None:
        while True:
            changed = False
            for sym, hit in found.items():
                f = program.functions[sym]
                before = f.sig
                try:
                    tr.prepare_signature(f, hit)
                except (AsmUnsupported, Unsupported):
                    pass
                changed |= f.sig != before
            if not changed:
                break
    prepare_all()
    for sym in found:
        program.functions[sym].unit = "<asm>"
    program.index_places()
    if tr.signature_check:
        # asm.signature_check over the functions with C (c_argument_drops), and with
        # asm.params_from_code each such definition given the parameters its code reads
        # (params_from_code); again until none is left, as a definition that takes more makes its
        # callers that hand those registers on read them too.
        for _ in range(8):
            stage.c_drops, stage.c_reads = c_argument_drops(tr, program, leave)
            if not a.get("params_from_code"):
                break
            added = params_from_code(tr, program, stage.c_drops)
            if not added:
                break
            stage.params_from_code += added
            # Translated callers that hand those registers on take them as well.
            prepare_all()
    # A C wrapper's result from a translated callee, known now, for the translations that read it
    # after their calls (asm.results_before_calls; hostgen.py does this again for the C).
    if a.get("results_before_calls"):
        stage.inherited = inherit_results(program)
    results: dict[str, Result] = {}
    for k, (sym, hit) in enumerate(sorted(found.items())):
        if PROGRESS is not None:
            PROGRESS("translate", k, len(found))
        r = tr.translate(program.functions[sym], hit)
        if r.text is None:
            stage.unsupported[sym] = r.reason
            program.functions[sym].unit = None
        else:
            results[sym] = r
            stage.translated.append(r)
    program.index_places()
    stage.translator = tr
    stage.found = found
    translate_shadows(stage, program, a)
    return stage, results


def params_from_code(tr: "Translator", program: Program, drops: dict[str, dict]) -> list[str]:
    """asm.params_from_code: each definition c_argument_drops finds takes the argument registers its
    code reads past the ones it names (Function.extra, as params_from_declarations gives them): its
    callers pass their values, and its own short calls hand them on (lower.py), as the EE's
    registers did. The types are those of the longest declaration that agrees with the definition
    where it has them, else int and float. Not for a function the port's own C calls (it calls it
    by the prototype it has), nor one with a record parameter (its calls must pass every argument).
    Returns "symbol: n integer, m float" of each one given more; drops left keep their report."""
    out = []
    kind = lambda t: "f" if canon(t, program.typedefs) == "float" else "i"
    for sym, d in sorted(drops.items()):
        f = program.functions.get(sym)
        if f is None or tr.host_text and re.search(r"\b" + re.escape(cname(sym)) + r"\b", tr.host_text):
            continue
        if any(canon(p, program.typedefs) is None for p in f.sig.params):
            continue
        need_i = max(0, d["reads"][0] - d["takes"][0])
        need_f = max(0, d["reads"][1] - d["takes"][1])
        n = len(f.sig.params)
        own = [kind(t) for t in f.sig.params]
        decl = None
        for s2 in f.declared:
            if (s2.prototyped and not s2.variadic and len(s2.params) > n and [kind(t) for t in s2.params[:n]] == own
                    and all(canon(t, program.typedefs) is not None for t in s2.params[n:])
                    and (decl is None or len(s2.params) > len(decl.params))):
                decl = s2
        ints = [t for t in (decl.params[n:] if decl else ()) if kind(t) == "i"][:need_i]
        floats = [t for t in (decl.params[n:] if decl else ()) if kind(t) == "f"][:need_f]
        ints += [ctype.Base("int")] * (need_i - len(ints))
        floats += [ctype.Base("float")] * (need_f - len(floats))
        new = tuple(ints) + tuple(floats)
        if not new:
            continue
        f.extra = tuple(f.extra) + new
        f.sig = Signature(f.sig.ret, tuple(f.sig.params) + new, False, True)
        out.append(f"{sym}: {need_i} integer, {need_f} float")
    return out


def c_argument_drops(tr: "Translator", program: Program, leave: set) -> tuple[dict[str, dict], dict[str, list]]:
    """asm.signature_check over the functions with C: the argument registers each one's retail code
    reads on some path before writing them (_live_arguments) that its C definition does not name.
    Matched C can leave out an argument it only hands on to a call, whose register keeps the
    caller's value on the EE (Up Your Arsenal's random_vector, func_L01_00443B88, names only `out`
    and calls rand_range with no arguments, which reads the range its callers left in $f12/$f13);
    translated, the call passes nothing. hostgen.json "params_from_declarations" (the list) with
    "extra_params" gives such a definition the parameters; what is left is the report's
    "c_argument_drops" (name -> what its code reads, what its definition takes, the places read).
    A function's places are read up to four of them (copies of one code). Returns the drops and what
    each function with C reads (name -> [integer, float] counts), for the report's zero-filled
    arguments (dropped_args.py)."""
    out: dict[str, dict] = {}
    reads: dict[str, list] = {}
    for sym, f in sorted(program.functions.items()):
        if (f.unit is None or f.unit == "<asm>" or f.static or f.sig.variadic or sym in program.host
                or sym in program.wrap or sym in leave):
            continue
        have_f = sum(1 for p in f.sig.params if canon(p, program.typedefs) in ("float",))
        have_i = len(f.sig.params) - have_f
        reads_i = reads_f = 0
        seen = 0
        for place in program.code_places(sym):
            if seen >= 4:
                break
            if tr.index.code.get(tuple(place)) is None:
                continue
            seen += 1
            try:
                tr._load(f, tuple(place))
                i, fl = tr._live_arguments()
                if tr.stack_arguments and (stacked := tr._stack_arguments()):
                    i = max(i, len(ARG_REGS) + stacked)
            except (AsmUnsupported, Unsupported):
                continue
            reads_i, reads_f = max(reads_i, i), max(reads_f, fl)
        if seen:
            reads[sym] = [reads_i, reads_f]
        if reads_i > have_i or reads_f > have_f:
            out[sym] = {"reads": [reads_i, reads_f], "takes": [have_i, have_f], "unit": f.unit}
    return out, reads


def file_head(game: str) -> list[str]:
    """What every file of translations starts with (asmdev.py's too)."""
    return [f"/* Generated by port/tools/hostgen for {game} from the decompilation's assembly (asmc.py).",
            " * Do not edit: give the function C in the decompilation, and it replaces this. */",
            "#include <math.h>", "#include \"openrac/ee.h\"", "#include \"game_protos.h\"", "",
            "#define OPENRAC_ASM_FRAME(size) const gaddr sp_in_ = openrac_guest_sp",
            "#define OPENRAC_ASM_SP() OPENRAC_EE_S32(sp_in_)",
            "#define OPENRAC_ASM_CALL() (openrac_guest_sp = (gaddr)r29)",
            "#define OPENRAC_ASM_LEAVE() (openrac_guest_sp = sp_in_)", ""]


# A function hostgen.json "ps2_float" names computes through openrac/ps2float.h, and max and
# min as the EE compares (the shadows' file has its own).
PS2_LEAD = ["#include <string.h>", "#include \"openrac/ps2float.h\"",
            "static inline int openrac_ps2s_key(float f) { unsigned u; memcpy(&u, &f, 4);"
            " return (u >> 31) ? -(int)(u & 0x7FFFFFFFu) : (int)u; }",
            "static inline float openrac_ps2s_max(float a, float b) {"
            " return openrac_ps2s_key(b) > openrac_ps2s_key(a) ? b : a; }",
            "static inline float openrac_ps2s_min(float a, float b) {"
            " return openrac_ps2s_key(b) < openrac_ps2s_key(a) ? b : a; }"]


def write_files(out: Path, cfg: dict, results: dict[str, Result], game: str,
                stage: Stage | None = None) -> list[str]:
    names = file_names(cfg)
    if not names:
        return []
    shadows = asm_config(cfg).get("shadows", {})
    spread = len(names) - (1 if shadows else 0)
    chunks: list[list[str]] = [[] for _ in range(spread)]
    for k, (sym, r) in enumerate(sorted(results.items())):
        chunks[k % spread].append(r.text)
    if stage is not None and stage.external and stage.translator is None:
        program = stage.program
        lead = ["void *openrac_asm_external(void *fn, const char *name);", ""]
        for k, sym in enumerate(stage.external):
            chunks[k % spread].append(asmext.trampoline(program.functions[sym], wrapped=sym in program.wrap))
        for chunk in chunks:
            chunk[:0] = lead
    head = file_head(game)
    ps2_lead = PS2_LEAD
    if shadows:
        texts = stage.shadows if stage is not None else {}
        # Each shadow's address, or 0 where there is no translation: what the port reads.
        table = [f"void *const openrac_shadow_{alias}_fn = "
                 + (f"(void *)openrac_shadow_{alias};" if alias in texts else "0;")
                 for alias in sorted(shadows)]
        chunks.append([texts[k] for k in sorted(texts)] + table)
    for name, chunk in zip(names, chunks):
        # The shadows reach game memory through the port's checker, which keeps what they touch.
        lead = ["#include <string.h>", "#include \"openrac/guest.h\"", "#include \"openrac/ps2float.h\"",
                "void *openrac_shadow_ptr(gaddr a);", "#define OPENRAC_EE_PTR(a) openrac_shadow_ptr(a)",
                # max and min as the EE compares: sign and magnitude.
                "static inline int openrac_ps2s_key(float f) { unsigned u; memcpy(&u, &f, 4);"
                " return (u >> 31) ? -(int)(u & 0x7FFFFFFFu) : (int)u; }",
                "static inline float openrac_ps2s_max(float a, float b) {"
                " return openrac_ps2s_key(b) > openrac_ps2s_key(a) ? b : a; }",
                "static inline float openrac_ps2s_min(float a, float b) {"
                " return openrac_ps2s_key(b) < openrac_ps2s_key(a) ? b : a; }"] if name == "asm_shadows.c" else []
        if not lead and any("openrac_ps2" in t for t in chunk):
            lead = ps2_lead
        text = "\n".join(head[:2] + lead + head[2:] + chunk) + "\n"
        path = out / name
        if not path.exists() or path.read_text() != text:
            path.write_text(text)
    return names


# ---- The console's form, for the offline check against retail (rac1-decomp's tools/fcheck.py) ----

def _vu0_word(kind: str, n: int = 0, rt: int = 8, off: int = 0) -> str:
    """One VU0 instruction as its word (any assembler reads .word): what moves the vector unit's
    registers to and from a function's own state in the console's form. Encoded here from the
    instruction set, and decoded back (asm.decode) so the two agree."""
    words = {
        "sqc2": 0xF8000000 | 8 << 21 | n << 16 | (off & 0xFFFF),
        "lqc2": 0xD8000000 | 8 << 21 | n << 16 | (off & 0xFFFF),
        "cfc2": 0x48400000 | rt << 16 | n << 11,
        "ctc2": 0x48C00000 | rt << 16 | n << 11,
        "acc_to_vf1": 0x4BE00048,     # vmaddx.xyzw $vf1, $vf0, $vf0x: ACC + 0
        "q_to_vf1": 0x4B000060,       # vaddq.x $vf1, $vf0, Q
        "vf1_to_q": 0x4B800BBC,       # vdiv Q, $vf1x, $vf0w: x / 1
        "waitq": 0x4A0003BF,          # vwaitq
        "vf1_to_acc": 0x4BE009BF,     # vmulaw.xyzw ACC, $vf1, $vf0w: vf1 * 1
    }
    return f".word 0x{words[kind]:08X}"


def _vu0_sync() -> str:
    """OPENRAC_VU0_IN and OPENRAC_VU0_OUT for the console's form: the vector unit's registers
    into the function's own state and back, as macros (the candidate is one function)."""
    asm_ = '__asm__ __volatile__("move $8, %0\\n\\t{}" : : "r"({}) : "$8", "memory");'
    save = ["openrac_vu0_state *p_ = OPENRAC_VU0;", "openrac_u32 v_;"]
    for n in range(1, 32):
        save.append(asm_.format(_vu0_word("sqc2", n), f"&p_->vf[{n}][0]"))
    save.append(asm_.format(f"{_vu0_word('acc_to_vf1')}\\n\\t{_vu0_word('sqc2', 1)}", "&p_->acc[0]"))
    save.append(asm_.format(f"{_vu0_word('q_to_vf1')}\\n\\t{_vu0_word('sqc2', 1)}", "&p_->scratch[0]"))
    save.append("p_->q = p_->scratch[0];")
    save.append(asm_.format(_vu0_word("lqc2", 1), "&p_->vf[1][0]"))
    flags = {16: "p_->status", 17: "p_->mac", 18: "p_->clip"}
    for n in list(range(1, 19)) + [21]:
        save.append(f'__asm__ __volatile__("{_vu0_word("cfc2", n, rt=8)}\\n\\tmove %0, $8" : "=r"(v_) : : "$8");')
        save.append(f"p_->vi[{n}] = v_ & 0xFFFFu;" if n < 16 else f"{flags[n]} = v_;" if n in flags
                    else "p_->i = openrac_ee_f(v_);")
    save.append("p_->vi[0] = 0;")
    load = ["openrac_vu0_state *p_ = OPENRAC_VU0;", "openrac_u32 v_;"]
    for n in list(range(1, 16)) + [16, 18, 21]:  # MAC is read only; of STATUS, the sticky bits
        load.append(f"v_ = {f'p_->vi[{n}]' if n < 16 else flags[n] if n in flags else 'openrac_ee_bits(p_->i)'};")
        load.append(f'__asm__ __volatile__("move $8, %0\\n\\t{_vu0_word("ctc2", n, rt=8)}" : : "r"(v_) : "$8");')
    load.append("p_->scratch[0] = p_->q;")
    load.append(asm_.format(f"{_vu0_word('lqc2', 1)}\\n\\t{_vu0_word('vf1_to_q')}\\n\\t{_vu0_word('waitq')}",
                            "&p_->scratch[0]"))
    load.append(asm_.format(f"{_vu0_word('lqc2', 1)}\\n\\t{_vu0_word('vf1_to_acc')}", "&p_->acc[0]"))
    for n in range(1, 32):
        load.append(asm_.format(_vu0_word("lqc2", n), f"&p_->vf[{n}][0]"))

    def macro(name: str, body: list[str]) -> str:
        return f"#define {name}() do {{ \\\n" + " \\\n".join("    " + s for s in body) + " \\\n} while (0)"
    return macro("OPENRAC_VU0_IN", save) + "\n" + macro("OPENRAC_VU0_OUT", load) + "\n"


def console_text(name: str, c: str) -> str:
    """A translation's console form as one candidate file: the console's definitions, the
    instruction helpers, and the function, whose C name is its own (the decompilation's file
    may declare the function otherwise) and whose symbol is the function's."""
    include = HERE_INCLUDE / "asmc_console.h"
    ops = HERE_INCLUDE.parent.parent.parent / "runtime" / "include" / "openrac" / "ee_ops.h"
    uses_vu0 = "OPENRAC_VU0->" in c
    prelude = [include.read_text(), ops.read_text(),
               "#define OPENRAC_ASM_FRAME(size) openrac_u64 stk_[((size) + 31) / 8] __attribute__((aligned(16))); "
               "openrac_u64 vu0_raw_[sizeof(openrac_vu0_state) / 8 + 2]",
               "#define OPENRAC_ASM_SP() OPENRAC_EE_S32((openrac_u32)((char *)stk_ + sizeof stk_) & ~15u)",
               "#define OPENRAC_ASM_CALL() 0", "#define OPENRAC_ASM_LEAVE() 0",
               "#define OPENRAC_VU0 ((openrac_vu0_state *)(((openrac_u32)vu0_raw_ + 15u) & ~15u))"]
    if uses_vu0:
        prelude.append(_vu0_sync())
    else:
        prelude += ["#define OPENRAC_VU0_IN() 0", "#define OPENRAC_VU0_OUT() 0"]
    own = f"openrac_asmc_{name}"
    lines = c.splitlines()
    head = next(k for k, l in enumerate(lines) if l.rstrip().endswith("{") and "(" in l)
    proto = lines[head].rstrip()[:-1].rstrip()
    proto = re.sub(r"\b" + re.escape(cname(name)) + r"\(", own + "(", proto, count=1)
    lines[head] = proto + " {"
    return "\n".join(prelude + [f"{proto} __asm__(\"{name}\");", ""] + lines) + "\n"


HERE_INCLUDE = Path(__file__).resolve().parent / "include"


def write_console(stage: Stage, out: Path, names: list[str] | None = None) -> dict[str, str]:
    """Each translated function of the executable (or those named) in the console's form, one
    candidate file each under out (name.c). Returns name -> why, for those it could not write."""
    tr = stage.translator
    problems: dict[str, str] = {}
    if tr is None:
        return problems
    out.mkdir(parents=True, exist_ok=True)
    console = Translator(tr.program, tr.index, tr.gp, "console", tr.source)
    console.gp_addresses = tr.gp_addresses
    console.forward_unprototyped = tr.forward_unprototyped
    # Named functions are written whether or not they have C (the check of the translator on
    # more of the program than its stubs); otherwise every one translated for the port.
    todo = names or [r.name for r in stage.translated]
    for name in todo:
        f = tr.program.functions.get(name)
        found = stage.found.get(name) or (console.find(f) if f is not None else None)
        if f is None or found is None or found[0] != -1:
            problems[name] = "no assembly for it in the executable"
            continue
        again = console.translate(f, found)
        if again.text is None:
            problems[name] = again.reason
            continue
        (out / f"{name}.c").write_text(console_text(name, again.text))
    return problems
