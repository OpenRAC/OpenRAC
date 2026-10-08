# The runtime

The part of OpenRAC that runs the games on a PC: the model of the
PlayStation 2 hardware the games draw through, and the host side that shows
the result. It is shared by all four games and has no game code or data in
it. It is at its first milestone: the renderer's end of the path exists and
draws, and nothing runs a game yet.

- [docs/DESIGN.md](docs/DESIGN.md): what is being built, in which order, and
  what is still open.
- [docs/RAC1_PAL_SURVEY.md](docs/RAC1_PAL_SURVEY.md): what the decompilation
  of Ratchet & Clank shows about its frame loop, display list, microprograms,
  menu, disc, memory card and library surface.
- [docs/OPENGOAL_NOTES.md](docs/OPENGOAL_NOTES.md): how OpenGOAL, the port of
  the Jak and Daxter games, solved the same problems.

## What is here

| Path | What it is |
|---|---|
| `src/ps2/gs_memory.*` | GS local memory: 4 MB, with the block and column layout of every pixel format |
| `src/ps2/gs.*` | The Graphics Synthesizer as a software model: registers, primitive assembly, a rasteriser for points, lines, triangles and sprites, texturing with colour tables, mipmaps and filtering, fog, the alpha, destination alpha and depth tests, blending, masks, transfers in, out and within local memory, and the display read-out |
| `src/ps2/gif.*` | GIF packets (PACKED, REGLIST, IMAGE) on the three paths |
| `src/ps2/vif.*` | VIF1: every UNPACK format with masks, modes and write cycles; MPG; MSCAL and the double buffer; DIRECT |
| `src/ps2/vu.*` | A vector unit running microprograms, with the timing they depend on: both halves of an instruction pair see the same state, flags arrive four instructions late, Q and P when their units finish, a branch tests the integer from before the instruction ahead of it, XGKICK sends one instruction late. Numbers have no infinities or denormals and round towards zero |
| `src/ps2/vu_asm.h` | Encoders for vector unit instructions, for tests and for programs written here |
| `src/ps2/graphics.h` | VIF1, VU1, the GIF and the GS connected as on the board |
| `src/ps2/dma.*` | The source-chain walker for a DMA channel, stopping on tag interrupts as the hardware does |
| `src/ps2/memory.h` | Guest memory: 32 MB and the scratchpad |
| `src/host/window.*` | An SDL3 window that shows one image per frame |
| `src/app/gsdemo.cpp` | `openrac-gsdemo`: a scene of its own, written into guest memory as a VIF1 DMA chain and drawn through all of the above, with a cube whose vertices a microprogram written for it transforms on VU1 |
| `src/app/vuscan.cpp` | `openrac-vuscan FILE`: finds the VU1 microprograms in an executable from your own disc, loads each through VIF1 as the game would and reports whether the interpreter decodes every instruction. It prints counts, never the programs |
| `tests/test_ps2.cpp`, `tests/test_vu.cpp` | Tests of the model against the documented layouts, formats, equations and timing |

Not here yet, in the order of [the milestones](docs/DESIGN.md#8-milestones):
the EE interpreter and the library boundary (with VU0 behind the EE's
vector instructions), the memory card, pad and disc services, sound, a GPU
back end. The vector unit has run only programs written here so far; the
games' own microprograms will be its real test.

## What it has been run against

`openrac-vuscan` on the boot executable of each supported disc (2026-10-08).
Decoding every instruction is necessary, not sufficient: it says nothing yet
about whether the programs compute the right thing here.

| Disc | VU1 programs | Instructions | Not decoded |
|---|---:|---:|---:|
| Ratchet & Clank, `SCES_509.16` | 9 | 8,625 | 0 |
| Ratchet & Clank, `SCUS_971.99` | 9 | 8,625 | 0 |
| Going Commando, `SCUS_972.68` | 10 | 10,086 | 0 |
| Up Your Arsenal, `SCUS_973.53` (`boot_elf.elf`) | 11 | 10,804 | 0 |
| Deadlocked, `SCUS_974.65` (unpacked image) | 13 | 10,996 | 0 |

## Building

macOS (the first target), with Xcode's command line tools:

```sh
brew install cmake ninja sdl3
cmake -S runtime -B build/runtime -G Ninja
cmake --build build/runtime
ctest --test-dir build/runtime            # the tests
build/runtime/openrac-gsdemo              # a window; Escape closes it
build/runtime/openrac-gsdemo --headless --frames 100 --ppm frame.ppm
```

`-DOPENRAC_RUNTIME_WINDOW=OFF` builds without SDL3 (tests and the headless
demo only). Linux and Windows are meant to work and are not tried yet.

## Rules for this directory

- GPL-3.0-or-later ([LICENSE.md](../LICENSE.md)); every source file starts
  with its SPDX line.
- Written from public hardware documentation and from what the games' own
  code shows. Never from Sony's SDK source, samples or headers
  ([SOURCING.md](../docs/policy/SOURCING.md)). Code adapted from a
  permissively licensed project says so in the file and in
  [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).
- No game code, microcode, data or bytes, in sources or in tests. A test's
  expected values come from documentation and arithmetic.
- Nothing specific to one game in `src/ps2` or `src/host`. What differs per
  game is a table generated from that game's files
  ([DESIGN.md](docs/DESIGN.md#6-what-is-per-game)).
- When the model meets something it does not do, it says so once
  (`gstodo` in `gs.h`) and carries on; it never guesses silently.
