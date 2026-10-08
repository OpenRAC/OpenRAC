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
| `src/ps2/dma.*` | The source-chain walker for a DMA channel, stopping on tag interrupts as the hardware does |
| `src/ps2/memory.h` | Guest memory: 32 MB and the scratchpad |
| `src/host/window.*` | An SDL3 window that shows one image per frame |
| `src/app/gsdemo.cpp` | `openrac-gsdemo`: a scene of its own, written into guest memory as a VIF1 DMA chain and drawn through all of the above |
| `tests/test_ps2.cpp` | Tests of the model against the documented layouts, formats and equations |

Not here yet, in the order of [the milestones](docs/DESIGN.md#8-milestones):
the EE interpreter and the library boundary, the VU0 and VU1 interpreters,
the memory card, pad and disc services, sound, a GPU back end.

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
