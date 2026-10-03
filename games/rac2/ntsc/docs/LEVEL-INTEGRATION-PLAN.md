# Level integration of the integrated C: what it takes

Measured 2026-10-01, on the same pinned image as the first two C lots.

## Why this is the next step

The level overlay programs are the same engine compiled into 28 programs: the
boot's 17 integrated C functions appear again inside each overlay - **13 of the
17 in every one of the 27 levels** (the four tiny boot-only accessors are the
exception). Their addresses differ per level, and the mapping is measured, not
assumed: byte search of each function's exact body, disambiguated for bodies
that are byte-identical to each other.

    couples (function, level) : 351 / 459        (raw byte search - overcounts, see below)

**Corrected after the reviewed placement rule (2026-10-01, same day).** The raw
byte search overcounts: three 8-byte bodies (`FUN_0026F710`, `FUN_00120BC8`,
`FUN_0026F718`) occur 97-374 times per level and are byte-twins of each other, so
they are never attributable and are excluded (81 couples); `FUN_00312E10` is a
complete body at two or three addresses in four levels and is excluded there.
The reviewed catalogue keeps **266 placements, 17,324 C bytes in the levels**
(10 per level, 9 in four of them), with 85 motivated exclusions.

If the levels count the same way the boot does - one program at a time, which is
how the 48,788,176-byte total is built - the present 716 bytes become
**716 + 17,324 = 18,040 bytes counted** (about 0.037 %), without writing a line
of new C. Every one of the 27 overlay gates passes: 76,965,088 loaded bytes
compared across the 27 overlays, all identical to retail.

## What is missing

1. **Per-level linker scripts.** `scripts/integration.py` already places each C
   function in its own section (`.text.FUN_xxxxxxxx`) through a generated linker
   script, so the same compiled object can be re-linked at another set of
   addresses. A catalog per level (symbol, address, size) is all it needs; the
   first one is measured above.
2. **A level build that integrates.** `scripts/build.py --all-levels` rebuilds
   and verifies every overlay from assembly, but the C path (`--c-toolchain`) is
   wired for the boot only. The overlay rebuild needs the same treatment the boot
   already has: substitute the C sections, then compare all loaded bytes.
3. **A report that counts per program.** `scripts/decomp_report.py` validates the
   scope as 28 programs but takes its matched bytes from the single boot
   integration proof, so a level match currently counts for nothing. The proof
   format already carries per-program identity; the report needs a proof per
   program and a matched-bytes sum across them.

## What is explicitly not assumed

- That a function present in a level sits in an executable section there (to be
  checked per level, like `owners` does for the boot).
- That the level's copy is reachable the same way; the comparison gate decides.
- That any of this raises the reported figure until the report counts it: the
  number to watch stays `matchedCode` in `build/decomp/report.json`.

## Order of work

1. Catalog per level (done: `catalogues-niveaux.json`, 351 couples).
2. Overlay integration on one level (0_aranos_tutorial), full-gate compared.
3. The other 26, then the per-program report change.
4. Only then port more C: each additional distinct byte counts 28 times.

## Appendix - state after the third C lot (2026-10-01)

The plan above was executed, and the third lot extended it. Current measured state:

- boot: 26 reviewed bodies, 1076 bytes;
- levels: **401 reviewed placements, 24,236 bytes** (the third lot added 135 placements,
  6912 bytes: five bodies - `FUN_002A8C00`, `FUN_002B7DF8`, `FUN_002B7FB0`,
  `FUN_002E60A0`, `FUN_002E60E8` - placed in all 27 overlays);
- counted total: **1076 + 24,236 = 25,312 bytes** (about 0.052 %).

Exclusions in the third lot are measured, not assumed: three bodies do not occur in any
level's text at all, and one (`FUN_002B6770`) is a complete function at two addresses of
every level, so neither occurrence is attributable to the reviewed symbol.

A further three RAC1-reviewed bodies were byte-proved in isolation but not integrated:
they exist **only** inside overlays (`FUN_00310838`, `FUN_00423BB0`, `FUN_0031C1E0`;
3804 bytes of potential placements). Placing them needs a second reviewed source wired
through `check_candidates`, `integration.py`, `build.py` and `decomp_report.py`, since the
level catalogue and the level qualification link both only admit bodies reviewed against
the boot. See `docs/THIRD-C-LOT.md`.
