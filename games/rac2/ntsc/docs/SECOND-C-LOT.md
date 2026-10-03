# Second RAC2 C lot - 2026-10-01

Ten further functions, taking the integrated C from **72 to 716 bytes**. `check_candidates.py`
reports **17/17 matched, zero different bytes** against the pinned boot.

## Where they come from, and why this is allowed

Measured 2026-10-01: RAC1's reconstruction corpus (the Lombyte tree) contains 1,414
reviewed C definitions; **24 of them are byte-identical to code that exists in RAC2's
boot or levels**, because the two games share engine code. Fourteen of those are
self-contained (no data or call references to re-anchor); eleven belong to the boot and
are used here.

A RAC1 body may only be reused when its bytes are **identical** in both games - no RAC1
name, address or compiler profile is promoted by analogy. The ten functions below were
located in the RAC2 image by exact byte search, and each was then re-verified against the
RAC2 reference with the RAC2 instrument.

## The compiler profile is confirmed for these functions

`ee-gcc2953.exe -c -O2 -G0 -ffunction-sections` (the SN ProDG 3.01 toolchain already used
for the first lot) reproduces the RAC2 bytes from the RAC1 C source. Worked example, the
204-byte polygon test `FUN_002A8AF0`: compiled object and retail bytes are identical, all
204 bytes. This extends the profile's demonstrated scope from leaf accessors to arithmetic
loops and float code; it remains a per-function qualification, not a general one.

| Symbol (RAC2) | Bytes | Where | Origin (RAC1) |
| --- | ---: | --- | --- |
| `FUN_0028B740` | 104 | boot | `FUN_L00_00235a70`, `src/overlays/shared/gameplay_animation_00235878.c` |
| `FUN_002A7AA8` | 56 | boot | `FUN_L00_00257e20`, `src/overlays/shared/gameplay_entities_00257d78.c` |
| `FUN_002A8860` | 56 | boot | `FUN_L00_00259430`, `src/overlays/shared/math_interpolation_00257ef0.c` |
| `FUN_002A8AF0` | 204 | boot | `FUN_L00_00259740`, `src/overlays/shared/gameplay_entities_00259710.c` |
| `FUN_002AA140` | 16 | boot | `FUN_L00_0025b6a8`, `src/overlays/shared/gameplay_entities_00259710.c` |
| `FUN_002AAF40` | 92 | boot | `FUN_L00_0025c088`, `src/overlays/shared/unclassified_0025bb38.c` |
| `FUN_002CC6A0` | 16 | boot | `FUN_L00_00277f60`, `src/overlays/shared/unclassified_00277f60.c` |
| `FUN_00312B58` | 16 | boot | `FUN_L16_002c4710`, `src/overlays/l16/unclassified_002a3f38.c` |
| `FUN_00312E10` | 16 | boot | `FUN_L00_002d8128`, `src/overlays/shared/unclassified_002d7f88.c` |
| `FUN_003505E0` | 68 | boot | `FUN_L00_002ef300`, `src/overlays/shared/runtime_buffers_002ef300.c` |

644 bytes in this lot, plus the first lot's 72 bytes: **716 bytes integrated**.

## What was left out, and why

- `FUN_002A8C00` (36 B) is shared too, but its body calls `qcopy()`, a RAC1 inline-assembly
  macro. A plain C rewrite would need its own byte proof; postponed.
- Eleven further shared bodies need data or call re-anchoring (their RAC1 source names
  addresses that live elsewhere in RAC2). Each needs its own mapping, like the RAC1
  campaigns.
- Two shared bodies belong to RAC2 level programs, not the boot. The current integration
  gate covers the boot only.

## Scope of the match, stated plainly

The 1.23x repetition measured across the 28 programs means matching a shared function in
one program does not multiply the reported figure. 716 bytes out of 48,788,176 executable
bytes is about **0.0015 %** - real progress over 72 bytes, and still far from a decompiled
game. The RAC1 corpus is now measured and exhausted for this purpose: everything else has
to be written against RAC2 itself.

## Update, same day

The closing claim above was premature: nine further couples of the same measurement were
integrated later the same day (`docs/THIRD-C-LOT.md`), taking the boot lot from 716 to
1076 bytes and the level catalogue to 401 placements. Two couples of the twenty-four are
refused there with their measured reasons and three are deferred.
