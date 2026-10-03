#!/usr/bin/env python3
"""
Turns a compiled call-and-return into retail's tail jump.

Why this exists
---------------
Retail forwards to another function with a bare `j target`, keeping no
frame at all. GCC 2.95 has no sibling-call optimisation (confirmed: the
`-foptimize-sibling-calls` flag does not exist in either SN sub-build,
and `-O3` does not help), so it always emits a full call-and-return:

    subu  $sp,$sp,16          <- prologue
    sq    $31,0($sp)
    lw    $4,64($4)           <- body (argument setup)
    jal   func_0012CE48
    addu  $4,$4,76            <- delay slot
    lq    $31,0($sp)          <- epilogue
    j     $31
    addu  $sp,$sp,16

where retail has:

    lw    $4,0x40($4)
    j     func_0012CE48
    addiu $4,$4,0x4C

The body and the delay-slot instruction already match — GCC schedules the
argument setup into the delay slot exactly as retail does. So the rewrite
is: drop the prologue, turn `jal` into `j`, drop the epilogue. No
instruction is reordered and nothing outside the frame is touched.

No instruction is synthesised. When there is no argument setup, GCC puts
the `$31` reload in the call's delay slot; removing the frame leaves that
slot to the assembler, which is running under `.set reorder` and fills it.
Emitting an explicit `nop` there is wrong — it becomes a third
instruction, giving 12 bytes against retail's 8. Retail's `nop` in that
slot is the assembler's, not the compiler's.

The stale `.frame`/`.mask` directives are rewritten to describe no frame,
since the one they described has been deleted.

Scope guard
-----------
A function is rewritten ONLY if its body matches this shape exactly:

  * prologue is exactly `subu $sp,$sp,N` then a `$31` spill at offset 0
  * epilogue is exactly the matching `$31` reload, `j $31`, `addu $sp,$sp,N`
  * exactly one `jal`, immediately followed by its delay slot and then
    the epilogue -- so nothing happens after the call returns
  * the frame instructions are the ONLY `$sp`/`$29` references, i.e. the
    function has no locals or spills of its own
  * no other control flow (no branches, no second jump, no labels inside)

Anything that does not fit is left completely alone. A function with work
after the call, or with its own stack locals, is NOT a tail call and must
not be transformed -- deleting its frame would corrupt it.
"""
import re
import sys

FRAME_PUSH = re.compile(r"^\s*subu\s+\$sp,\$sp,(\d+)\s*$")
FRAME_POP = re.compile(r"^\s*addu\s+\$sp,\$sp,(\d+)\s*$")
RA_SAVE = re.compile(r"^\s*(?:sq|sd)\s+\$31,0\(\$sp\)\s*$")
RA_LOAD = re.compile(r"^\s*(?:lq|ld)\s+\$31,0\(\$sp\)\s*$")
RETURN = re.compile(r"^\s*(?:j|jr)\s+\$31\s*$")
LABEL = re.compile(r"^\s*[A-Za-z_.$][\w.$]*:\s*$")
JAL = re.compile(r"^(\s*)jal(\s+)(\S+)\s*$")
SP_REF = re.compile(r"\$sp|\$29")
# any label, branch or jump that is not the single `jal`/return we expect
CONTROL = re.compile(r"^\s*(?:b\w*|j|jal|jr)\b|^\s*\$?[A-Za-z_.$][\w.$]*:")


# Mnemonics that assemble to exactly one machine instruction, so that
# sinking one into a delay slot under `.set nomacro` cannot silently
# turn into two words. Anything not listed is left where it is.
SINKABLE = re.compile(
    r"^\s*(?:l[bhwdq]u?|lwu|s[bhwdq]|addu?|addiu|daddu|subu|dsubu|move|"
    r"sll|srl|sra|dsll|dsrl|dsra|or|ori|and|andi|xor|xori|nor|lui|"
    r"slt|sltu|slti|sltiu|nop)\s")

FRAME_DIR = re.compile(r"^\s*\.frame\s")
MASK_DIR = re.compile(r"^\s*\.mask\s")


def is_directive(line: str) -> bool:
    s = line.strip()
    return not s or s.startswith(("#", "."))


def rewrite_function(lines: list[str]) -> list[str] | None:
    """Return rewritten lines, or None if the function is not a pure tail call."""
    idx = [i for i, l in enumerate(lines) if not is_directive(l)]
    code = [lines[i] for i in idx]

    # A leading `name:` label belongs to the function, not its body.
    if code and LABEL.match(code[0]):
        idx, code = idx[1:], code[1:]
    if len(code) < 5:
        return None

    # --- prologue: `subu $sp,$sp,N`, and a `$31` spill at offset 0 somewhere
    # before the call. The spill is NOT required to sit immediately after the
    # push: GCC interleaves argument setup with it (e.g. `lui $5,%hi(..)`
    # lands between the two), so requiring adjacency rejects real tail calls.
    #
    # Offset 0 with nothing else in the frame is what says "no locals": a
    # function with its own stack slots spills $ra above them (e.g. at 16),
    # and deleting its frame would corrupt those slots.
    jal_positions = [i for i, l in enumerate(code) if JAL.match(l)]
    if len(jal_positions) != 1:
        return None
    j = jal_positions[0]
    after = code[j + 1:]

    # The push is NOT required to be the first instruction, for the same
    # reason the $31 spill isn't: GCC interleaves argument setup with the
    # prologue, so `lui $2,%hi(sym)` can precede `subu $sp,$sp,N`.
    # Requiring position 0 silently rejected real tail calls (measured on
    # func_0011BC70, which compiled to 36 bytes against retail's 12).
    # Exactly one push before the call is still required, so a function
    # touching $sp more than once is refused.
    pushes = [i for i in range(0, j) if FRAME_PUSH.match(code[i])]
    if len(pushes) != 1:
        return None
    push_at = pushes[0]
    push = int(FRAME_PUSH.match(code[push_at]).group(1))

    saves = [i for i in range(0, j) if i != push_at and RA_SAVE.match(code[i])]
    if len(saves) != 1:
        return None
    save_at = saves[0]

    # --- two accepted tails, and nothing else.
    #   (a) [$31 reload, j $31, frame pop]        -- no argument setup, so
    #       the reload sits in the call's delay slot and retail has a nop
    #   (b) [delay, $31 reload, j $31, frame pop] -- delay holds argument
    #       setup, exactly as retail schedules it
    # `work-after-the-call` fails both: its reload is followed by the extra
    # work rather than by `j $31`.
    if (len(after) == 3 and RA_LOAD.match(after[0])
            and RETURN.match(after[1]) and FRAME_POP.match(after[2])):
        delay, epilogue = None, after
    elif (len(after) == 4 and RA_LOAD.match(after[1])
            and RETURN.match(after[2]) and FRAME_POP.match(after[3])):
        delay, epilogue = after[0], after[1:]
    else:
        return None
    if int(FRAME_POP.match(epilogue[-1]).group(1)) != push:
        return None

    body = [code[i] for i in range(0, j) if i not in (push_at, save_at)]
    checked = body + ([delay] if delay else [])
    # The frame instructions must be the ONLY stack references, and there
    # must be no other control flow in the function.
    if any(SP_REF.search(l) for l in checked):
        return None
    if any(CONTROL.match(l) for l in checked):
        return None

    m = JAL.match(code[j])
    lead, gap, target = m.group(1), m.group(2), m.group(3)
    tail = f"{lead}j{gap}{target}\n"

    # --- case (a) with a non-empty body: sink the last body instruction
    # into the jump's delay slot.
    #
    # Leaving the slot to the assembler is right only when the body is
    # EMPTY, i.e. retail is a bare `j` plus nop at 8 bytes. When there is
    # a preceding instruction, retail has it in the slot: retail's
    # compiler filled the unconditional jump's delay slot from BEFORE the
    # jump, which is the one direction SN's assembler will not do -- it
    # fills only from after, so at the end of a function it has nothing
    # to take and emits a nop. func_0011BC70 is the case: retail is
    # `lui / j / lw(delay)` = 12 bytes, we were `lui / lw / j / nop`.
    #
    # This is the only motion this file performs, and it is safe by
    # construction rather than by analysis: a delay-slot instruction runs
    # BEFORE control reaches the target, so program order is unchanged; a
    # direct `j` reads no registers, so there is nothing for the sunk
    # instruction to clobber; and the scope guard above already rejects
    # any function containing an internal label, so the instruction
    # cannot be some other path's branch target.
    sink_at = None
    if delay is None:
        cand = [i for i in range(0, j) if i not in (push_at, save_at)]
        if cand and SINKABLE.match(code[cand[-1]]):
            sink_at = cand[-1]

    drop = {idx[push_at], idx[save_at]} | {idx[j + 1 + k] for k in range(len(after))
                               if after[k] in epilogue and
                               (delay is None or k > 0)}
    if sink_at is not None:
        drop = drop | {idx[sink_at]}
    jal_line = idx[j]
    out = []
    for i, l in enumerate(lines):
        if i in drop:
            continue
        # The frame no longer exists, so stop advertising one: `.frame`
        # still claimed 16 bytes with $ra saved, and `.mask` still named
        # the (now deleted) $ra spill.
        if FRAME_DIR.match(l):
            out.append("\t.frame\t$sp,0,$31\n")
            continue
        if MASK_DIR.match(l):
            out.append("\t.mask\t0x00000000,0\n")
            continue
        if i == jal_line:
            if sink_at is not None:
                # Written out under noreorder so the assembler leaves
                # the slot exactly as given.
                out.append("\t.set\tnoreorder\n\t.set\tnomacro\n")
                out.append(tail)
                out.append(code[sink_at])
                out.append("\t.set\tmacro\n\t.set\treorder\n")
                continue
            out.append(tail)
            # With an EMPTY body, case (a) deliberately emits nothing
            # here. The assembler runs under `.set reorder` and fills
            # the slot itself, so an explicit `nop` becomes a THIRD
            # instruction: measured 12 bytes against retail 8 before
            # this was removed.
            continue
        out.append(l)
    return out


def load_whitelist(path: str = "tools/tail_call_functions.txt") -> set[str]:
    """
    Functions whose RETAIL form is a bare tail jump.

    This list is the only valid authority for applying the rewrite.
    Matching the call-and-return shape in our OWN output is not
    sufficient and actively dangerous: GCC 2.95 compiles `return f(x);`
    to call-and-return, and so did retail's compiler, so a function can
    have that shape on both sides. Rewriting such a function turns a
    working match into a size mismatch -- which then drifts everything
    after it. Measured when this guard was missing: 272 exact -> 217,
    with 8 size mismatches and 85 byte mismatches from the drift.
    """
    out = set()
    try:
        for line in open(path, encoding="utf-8"):
            line = line.strip()
            if line and not line.startswith("#"):
                out.add(line)
    except FileNotFoundError:
        pass
    return out


def rewrite(text: str, allow: set[str] | None = None) -> tuple[str, int]:
    if allow is None:
        allow = load_whitelist()
    lines = text.splitlines(keepends=True)
    out, i, n = [], 0, 0
    while i < len(lines):
        m = re.match(r"^\s*\.ent\s+(\S+)", lines[i])
        if not m or m.group(1) not in allow:
            out.append(lines[i])
            i += 1
            continue
        end = i
        while end < len(lines) and not re.match(r"^\s*\.end\b", lines[end]):
            end += 1
        if end >= len(lines):
            out.extend(lines[i:])
            break
        inner = lines[i + 1:end]
        new = rewrite_function(inner)
        out.append(lines[i])
        out.extend(new if new is not None else inner)
        out.append(lines[end])
        if new is not None:
            n += 1
        i = end + 1
    return "".join(out), n


def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__)
        raise SystemExit(2)
    src, dst = sys.argv[1], sys.argv[2]
    text = open(src, encoding="utf-8", errors="surrogateescape").read()
    new, n = rewrite(text)
    open(dst, "w", encoding="utf-8", errors="surrogateescape").write(new)
    print(f"fix_tail_calls: converted {n} call-and-return(s) to tail jumps {src} -> {dst}")


if __name__ == "__main__":
    main()
