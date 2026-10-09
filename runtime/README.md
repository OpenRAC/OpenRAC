# The runtime

The part of OpenRAC that runs the games on a PC: the model of the
PlayStation 2 hardware the games draw through, and the host side that shows
the result. It is shared by all four games and has no game code or data in
it. It runs the retail program of Ratchet & Clank (PAL) from your own disc
image: the title screen, the main menu, the memory card screens (the game
creates, lists, saves and loads its files in a folder of yours), a new game
and the first level, which draws as it should at close to full speed on an
M-series Mac, with its music and speech. Sound effects are not made yet.

- [docs/CODING_CONVENTIONS.md](docs/CODING_CONVENTIONS.md): how the C++ here
  is laid out, documented and commented. Read it before writing any.
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
| `src/ps2/vu.*` | A vector unit running microprograms, with the timing they depend on: both halves of an instruction pair see the same state, flags arrive four instructions late, Q and P when their units finish, a branch tests the integer from before the instruction ahead of it (unless that one read flags, or the branch had to wait), XGKICK sends one instruction late. Numbers have no infinities or denormals and round towards zero |
| `src/ps2/fp.h`, `fp_quad.h` | The console's float arithmetic on bit patterns (no infinities, no denormals, rounding towards zero, the adder's one guard bit), and a four-field fast path that is exact where it applies |
| `src/ps2/ee.*`, `ee_mmi.cpp` | The Emotion Engine's CPU core as an interpreter: MIPS III, the 128-bit and multimedia instructions, the FPU, and the vector instructions on VU0, which runs beside it as on the console |
| `src/ps2/vu_asm.h` | Encoders for vector unit instructions, for tests and for programs written here |
| `src/ps2/graphics.h` | VIF1, VU1, the GIF and the GS connected as on the board |
| `src/ps2/dma.*` | The source-chain walker for a DMA channel, stopping on tag interrupts as the hardware does |
| `src/ps2/memory.h` | Guest memory: 32 MB and the scratchpad |
| `src/ps2/vu_dis.h` | A disassembler for microprograms, for the terminal |
| `src/sys/disc.*` | A disc image: sectors and the ISO 9660 directory |
| `src/sys/machine.*` | The console as a game program needs it: memory, the DMA controller, timers, the interrupt controller, the kernel's services, and the replacing of library functions by name |
| `src/sys/drawing.*` | The drawing path on a thread of its own: the EE's side copies what a DMA channel sends, and VIF1, VU1, the GIF and the GS take the copies in order |
| `src/sys/services.cpp` | What the replaced library functions do: the disc, the pad, the sound server (streams and sound effects), the display's timing |
| `src/sys/sound.*` | The sound library's streams (music, speech): ADPCM files on the disc, decoded and mixed one field's worth after each field |
| `src/snd/*` | The sound library's sound effects: banks of sounds, each a short script of steps (tones, waits, loops, random picks, registers, modulators), played on 48 voices with the sound processor's sample format and envelope, and mixed over the streams after each field |
| `src/sys/memcard.cpp` | The memory card library answered from a directory of the host: a folder on the card is a directory, a file a file |
| `games/SERIAL.hooks` | Per game: which addresses of its program are which library functions. Addresses and names only |
| `src/host/window.*` | An SDL3 window that shows one image per frame and reads the keyboard and a game controller |
| `src/app/boot.cpp` | `openrac-boot`: runs the program on your own disc image, in a window or headless, with scripted input and listings for working on the model |
| `src/app/gsdemo.cpp` | `openrac-gsdemo`: a scene of its own, written into guest memory as a VIF1 DMA chain and drawn through all of the above, with a cube whose vertices a microprogram written for it transforms on VU1 |
| `src/app/vubench.cpp` | `openrac-vubench FILE`: times the vector unit on one frame of a game's own display list (written by `openrac-boot --dump-vif` on your machine) and prints a sum over what VU1 sends, to show that a change computes the same |
| `src/app/sndbank.cpp` | `openrac-sndbank DISC.iso SECTOR [--sound N] [--wav FILE]`: reads a sound effect bank from your own disc, lists its sounds and their steps, and plays each one into a sound file |
| `src/app/vuscan.cpp` | `openrac-vuscan FILE`: finds the VU1 microprograms in an executable from your own disc, loads each through VIF1 as the game would and reports whether the interpreter decodes every instruction. It prints counts, never the programs |
| `tests/test_ps2.cpp`, `tests/test_vu.cpp`, `tests/test_snd.cpp` | Tests of the model against the documented layouts, formats, equations and timing; the sound tests build their own bank |

Not here yet, in the order of [the milestones](docs/DESIGN.md#8-milestones):
the last of the speed (the first level runs at
40 to 48 frames a second of 50 on an M5), a GPU back end, the other games'
tables.

## Running a game

With your own disc image of Ratchet & Clank (PAL, `SCES_509.16`):

```sh
build/runtime/openrac-boot DISC.iso --hooks runtime/games/SCES_509.16.hooks --window
```

| Control | Key | Control | Key |
|---|---|---|---|
| Left stick | W A S D | Right stick | I J K L |
| Direction pad | arrows | Cross | space |
| Square | F | Circle | E |
| Triangle | R | L1, R1 | Q, left shift |
| L2, R2 | Z, C | Start, select | return, backspace |

A game controller works as it is labelled. Escape closes the window.

The memory card is a directory: by default
`~/Library/Application Support/OpenRAC/memcard/SERIAL` on macOS and
`~/.local/share/openrac/memcard/SERIAL` elsewhere, or the one `--card
DIRECTORY` names; `--no-card` leaves the slot empty. The game's files in it
are the files a card would hold.

Headless, for working on the model: `--frames N` stops after N fields,
`--ppm FILE` writes the last picture, `--press FRAME:BUTTONS[:FRAMES]` holds
buttons (a hexadecimal mask; cross is 4000, start 8), `--report N` prints
counts and the speed every N fields, `--gs-states FRAME` lists what that
frame is drawn with, state by state, `--one-thread` keeps the drawing path
on the program's thread (the picture is the same either way), `--dump-vif
FRAME FILE` writes a frame's display list for `openrac-vubench`, `--wav FILE`
writes everything heard.

## What it has been run against

`openrac-vuscan` on the boot executable of each supported disc (2026-10-08).
Decoding every instruction is necessary, not sufficient; whether the programs
compute the right thing shows only when a game runs, as the first one now
does.

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

- The C++ follows [docs/CODING_CONVENTIONS.md](docs/CODING_CONVENTIONS.md);
  the layout is what `.clang-format` produces.
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
