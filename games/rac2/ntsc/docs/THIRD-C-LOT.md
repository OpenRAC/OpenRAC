# Third RAC2 C lot - 2026-10-01

Nine further shared bodies are integrated in the boot, taking the integrated C from
**716 to 1076 bytes** (26 functions), and they are placed in the level overlays as well.
`check_candidates.py` reports **26/26 matched, zero different bytes** against the pinned boot.

## Where they come from

The same measured corpus as the second lot: RAC1's reconstruction tree (Lombyte) holds
reviewed C whose compiled bytes are byte-identical to code in RAC2. The fourteen remaining
couples of that measurement are the subject of this lot. Every body was located in the
pinned RAC2 image by exact byte search of its RAC1 bytes, then re-proved against the RAC2
reference with the RAC2 instrument - no RAC1 address, name or profile is promoted by
analogy.

| Symbol (RAC2) | Reviewed bytes | Splat boundary | Origin (RAC1) |
| --- | ---: | ---: | --- |
| `FUN_0011C880` | 28 | 32 | `FUN_0011a428`, `src/runtime/sio/fun_0011a428.c` |
| `FUN_0011C8A0` | 12 | 16 | `FUN_0011a448`, `src/runtime/sio/fun_0011a448.c` |
| `FUN_0012DA98` | 40 | 40 | `FUN_00129b38`, `src/sdk/library/fun_00129b38.c` |
| `FUN_002A8C00` | 36 | 36 | `FUN_L00_00259888`, `src/overlays/shared/gameplay_entities_00259710.c` |
| `FUN_002B6770` | 24 | 24 | `FUN_0022dd78`, `src/audio/sound/fun_0022dd78.c` |
| `FUN_002B7DF8` | 48 | 48 | `FUN_00216990`, `src/audio/streaming/fun_00216990.c` |
| `FUN_002B7FB0` | 60 | 64 | `FUN_00216b28`, `src/audio/streaming/fun_00216b28.c` |
| `FUN_002E60A0` | 68 | 72 | `FUN_0022dd90`, `src/audio/sound/fun_0022dd90.c` |
| `FUN_002E60E8` | 44 | 48 | `FUN_0022ddd8`, `src/audio/sound/fun_0022ddd8.c` |

360 bytes in this lot: **1076 bytes integrated in the boot**.

Measured on the committed bodies: none of the nine contains a `jal` or a `lui` instruction,
so no body calls out or materialises an address, and nothing had to be re-anchored to a
RAC2 data or call target. The only memory access beyond the caller's frames is
`FUN_002A8C00`'s quadword load/store pair, which goes through registers.

## Two measured points about the reviewed size

- Five of the nine have a splat boundary four bytes longer than the compiler output. The
  extra word is a `nop` that the compiler does not emit after a store in a return delay
  slot; the RAC1 project's own report counts the same shorter extents (`FUN_0011a428` 28,
  `FUN_0011a448` 12, `FUN_00216b28` 60, `FUN_0022dd90` 68, `FUN_0022ddd8` 44). The
  catalogue stores the compiler-produced extent, which is what the gate can require, and
  the padding word stays in the assembly input.
- `FUN_002A8C00` closes with a 16-byte quadword copy. The RAC1 corpus performs it with an
  inline-assembly macro (`qcopy`), which this repository's candidate gate refuses. The body
  here is a plain C quad copy through a `volatile` destination pointer; without the
  qualifier the compiler moves the quad store into the return's delay slot and the bytes do
  not match. The C is ordinary compiler input - no assembly, no patched bytes - and the
  gate compares all 36 bytes.

## Levels

The same bodies are placed in the overlays where their bytes are a complete function at one
of that level's own splat entries, by the measured rule of `docs/LEVEL-INTEGRATION-PLAN.md`:

- 135 new placements, 6912 C bytes, taking the level catalogue to **401 placements,
  24,236 bytes**;
- five symbols (`FUN_002A8C00`, `FUN_002B7DF8`, `FUN_002B7FB0`, `FUN_002E60A0`,
  `FUN_002E60E8`) are placed in all 27 levels;
- 108 measured exclusions: `FUN_0011C880`, `FUN_0011C8A0` and `FUN_0012DA98` do not occur
  in any level's text at all (they are boot-core code), and `FUN_002B6770` is a complete
  function at two addresses of every level, so no occurrence is attributable to one
  reviewed symbol.

## What was refused, and why

- `FUN_00283678` (32 bytes): the same kind of three-quadword copy. Its retail order is
  three loads into `$at`, `$v0`, `$v1` followed by three stores; no plain C form tried
  reproduces that schedule or register choice (an aligned-struct copy interleaves
  load/store pairs, a TImode copy does the same, and the volatile form keeps the pair
  order but not the register choice). In the RAC1 tree this function is an `INCLUDE_ASM`
  body under `src/assembly/textbin/` - never matched in C there either; the 42.5 % body
  in the measured corpus is that project's unverified non-matching variant. Not added.
- `FUN_0021A318` (16 bytes): its RAC2 location is the address of the already-integrated
  `FUN_002CC6A0`, and the bytes are identical. Nothing new to count.
- `FUN_00310838` (116 bytes, `0_aranos_tutorial` 0x310838), `FUN_00423BB0` (16 bytes) and
  `FUN_0031C1E0` (120 bytes, `17_smolg` 0x31C1E0) are three further RAC1-reviewed bodies
  whose bytes exist in RAC2 **only inside level overlays** (27, 27 and 2 occurrences
  respectively). Each one compiles byte-exact in isolation with the same profile and
  compiler, but the counting path admits only bodies reviewed against the boot: the level
  catalogue validates every placement against the boot catalogue, and the level
  qualification link compiles `candidates/boot.c` only. Counting them needs a second
  reviewed source (a level-body lot) wired through `check_candidates`, `integration.py`,
  `build.py` and `decomp_report.py`. Deferred, not refused: the measurement and the byte
  proofs exist, and the placements they would add are measured at 3804 bytes.

## Scope, stated plainly

1076 boot bytes plus 24,236 level placement bytes are **25,312 counted C bytes** of
48,788,176 executable bytes, about **0.0519 %**. The twenty-four measured couples are now
all accounted for: ten integrated in the second lot, nine here, two refused above, and
three deferred above. Further progress has to be written against RAC2 itself, or the
deferred level-body path has to be opened first.
