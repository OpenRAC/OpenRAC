# Credits

This file records every outside project or person whose work this repository
uses, and what was used. Credit here is attribution only: it never adds
matching credit, and nothing listed is evidence for a match. The only evidence
is compiler output that compares byte for byte against the pinned executable.

Add a row for every item taken from an outside source, at the time it is taken,
with the source commit and the repository file it informed.

## Promises — RC2-Going-Decompiled

- Project: [Promises/RC2-Going-Decompiled](https://github.com/Promises/RC2-Going-Decompiled)
  (commits are authored under the name Henning Berge)
- Reviewed revision: `855deaff0eed040190198f83ed5fa7b871f52efb`
- Target of that project: Going Commando USA v2.00 and EU v1.00, not this
  repository's USA v1.01 target. Its addresses, sources and proofs are leads
  here until re-measured against `config/target.json`.
- Licence: the repository carries no `LICENSE` file at the reviewed revision.
  The initial gap-report PR copied no source. Subsequent tooling contributions
  import the reference helpers below under their upstream GPL-2.0 licence.
- Permission: the maintainer reports that the author agreed to reuse. The
  author's own approval on the pull request that introduced this file is the
  record; the scope and licence of any later adoption are to be stated in the
  row that adopts it.

### What was taken

| Item | Used for | Where |
| --- | --- | --- |
| Reference shell/AWK/sed helpers, once in `tools/ee/` | Experimental reference only; removed when the build was rewritten (2026-10-11) | contributor import history in PR #135; upstream licence revision `f134130e8e979843da090f89109a480b773bc2e3` |
| v2.00/EU symbol maps and derived name hints | Exploratory naming with region-specific addresses | `symbol_addrs/` and `config/datamine/`; naming adds no match credit |
| Gap comparison of their public tree against this repository | Listing leads we do not yet have | docs/PROMISES-GAP-REPORT.md |

Anything adopted later from this project is added to the table above, with the
source file and commit, in the same change that adopts it.

## Made in Slovakia — Ratchet & Clank PCSX2 cheat codes

[Made-in-Slovakia/rac](https://github.com/Made-in-Slovakia/rac) is credited by
the Promises project for its first global-naming anchors (v2.00 addresses).
It is listed here only because the Promises leads below depend on it. Nothing
from it is used in this repository.

## Lombyte (the NTSC decomp)

- The patch stack for the game compiler (`tools/compiler/patches/`, `sce-991111b`, MIT) and the
  recipe for the SDK compiler (`tools/compiler/sdk/build.py`, adapted from
  `scripts/build-patched-toolchain.py`, MIT, Mateusz Kłysz).

The inherited files above retain their historical notices. Current Lombyte at
`0d5b2b0cb5c0de696425831ba9b995be8fbbd439` is GPL-3.0. Its
[qcopy helper](https://github.com/lombyte-project/Lombyte/blob/0d5b2b0cb5c0de696425831ba9b995be8fbbd439/include/qcopy.h)
and [RAC1's helpers](https://github.com/OpenRAC/rac1-decomp/blob/813090403fe822acd929616da0fee2081131f92e/include/common.h)
informed the register-address and scratch-register model in
`tools/compiler/transformers/qcopy_pattern.py`. That transformer and its tests
were independently authored without copying inline-assembly helpers.

On 25 October 2026, the source-method policy was aligned with RAC1. The shared
`qcopy`, `qcopy_nc` and `qzero` helpers in `include/common.h` adapt
[RAC1's GPL-3.0 implementation at 69c3e42d](https://github.com/OpenRAC/rac1-decomp/blob/69c3e42d792adb90fabb90184a4fac915a48f5b2/include/common.h).
RAC2's clear helper additionally declares a memory clobber. The constant-pool
placement in `tools/build.py` follows the same revision's
[`core_rodata.py`](https://github.com/OpenRAC/rac1-decomp/blob/69c3e42d792adb90fabb90184a4fac915a48f5b2/tools/core_rodata.py)
and scalar-pool placement principle; RAC2 derives addresses from unchanged object
relocations rather than rewriting assembly section directives or storing an
address registry. Its validation and splitter extensions are independently authored.

`tools/regalloc.py` adapts the dump-parser/priority approach from
[RAC1's diagnostic at 948d2252](https://github.com/OpenRAC/rac1-decomp/blob/948d2252ef6b7885418fa980db7ed9e4ec0fefa6/tools/regalloc.py)
(unchanged at `69c3e42d`), itself adapted from RAC3. The RAC2 command uses its own
source/profile lookup, isolated compilations and complete-object equality guard.
The referenced projects and this adaptation are GPL-3.0.

`tools/sdata_aliases.py` adapts only the explicit `SDATA` address-alias metadata
from [RAC1's `check_macro_slots.py` at `209d3381`](https://github.com/OpenRAC/rac1-decomp/blob/209d3381ebaa8c06d5df98eeabad4adad45bdfb7/tools/check_macro_slots.py)
(GPL-3.0). RAC2 does not adopt its delay-slot operand transformations. Recognition
of incomplete-array aliases, metadata placement and fail-closed checks are
qualified by independently authored controls; canonical storage extents remain unchanged.

The ordinary-C packet cursor views used by `func_0027DB68`, `func_0027E920`
and `func_0027EA28` also reference Lombyte's GPL-3.0
[`dma_tag.h` at c41bf1c](https://github.com/lombyte-project/Lombyte/blob/c41bf1c324eecae8006bf82808ad19ed42ee5eb9/include/rnc/rendering/dma_tag.h).
Their source comments record the pinned reference and local adaptations. The
retail-specific bindings and packet writes were reconstructed for RAC2; no inline
assembly, SDK header or assembler alias was imported.

## Public C runtime sources

| Source and revision | Use in this repository |
| --- | --- |
| [Newlib `_strtol_r`, 039e8a3](https://github.com/mirror/newlib-cygwin/blob/039e8a3c16964d02838b204b72bfdd4b7529107c/newlib/libc/stdlib/strtol.c) | Existing retained adaptation promoted to `src/core/00117278.c`; the Berkeley notice is preserved using Newlib's [advertising-clause removal](https://github.com/mirror/newlib-cygwin/commit/9042d0ce65533a26fc3264206db5828d5692332c). |
| [GNU GCC `fp-bit.c`, 17b916d](https://github.com/gcc-mirror/gcc/blob/17b916df3d859b72736d537f6c2a98b2d76decf8/gcc/config/fp-bit.c) | Existing retained double packer, adder and multiplier promoted to `src/core/00122630.c`, `src/core/00122800.c` and `src/core/00122B00.c`, with the original fraction union in their shared caller/callee record; full GPL-2.0-or-later notices, historical linking permissions and credits remain in all three files. |
| [Public GCC runtime sources recorded by RAC1, revision `31cf01446d`](https://github.com/OpenRAC/rac1-decomp/blob/69c3e42d792adb90fabb90184a4fac915a48f5b2/src/libgcc/README.md) | `src/core/0011FC68.c` and `src/core/00121450.c` adapt the `L_divdi3`/`L_moddi3` modules, preserving the original public MIPS multiplication primitive and complete leading-zero macro. Both original FSF notices and the libgcc linking exception are retained. Their lookup tables are compiled from public source, not copied game data. |
| [RAC1 formatter, `69c3e42d`](https://github.com/OpenRAC/rac1-decomp/blob/69c3e42d792adb90fabb90184a4fac915a48f5b2/src/core/00119D88.c) | `src/core/0011C090.c` adapts `func_00119F38` under GPL-3.0, with RAC2's measured callback, software-double helpers and full-width conversion interface. |
| [RAC1 assembler model, 8130904](https://github.com/OpenRAC/rac1-decomp/blob/813090403fe822acd929616da0fee2081131f92e/tools/ps2eeas_dli.py) | Reference for the isolated sign-bit expansion in patch `0052`; the new assembler tests and synthetic constant cases are independently authored. |
