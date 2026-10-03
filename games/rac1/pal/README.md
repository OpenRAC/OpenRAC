# Ratchet & Clank Decompilation

[![Progress report](https://github.com/Lynder063/rac1-decomp/actions/workflows/progress.yml/badge.svg)](https://github.com/Lynder063/rac1-decomp/actions/workflows/progress.yml)
[![Code](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp)
[![Functions](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp)
[![Discord](https://img.shields.io/badge/Discord-Join%20Community-5865F2?logo=discord&logoColor=white)](https://discord.gg/Sfd2B54PDG)

> [!NOTE]
> Because of recent events in the Ratchet & Clank community, I had to remove
> all information associated with John Doe #1 and John Doe #2 from this project
> at their request. If anyone else would like mentions of them removed, please
> contact me on the [Discord](https://discord.gg/Sfd2B54PDG).
>
> - Kryštof "Lynder063" Malinda

A work-in-progress **matching decompilation** of *Ratchet & Clank* (Insomniac
Games, 2002) for the PlayStation 2. The goal is C/C++ source that, built with
the original toolchain, produces a byte-identical copy of the retail executable.

The project runs in two phases:

1. **Match.** Write source that compiles to exactly the retail machine code.
   This is what proves a function has been understood: the compiler judges
   the result, not a read-through.
2. **Make it readable.** Refactor matched code toward idiomatic C++ with real
   names, types and structure. The matching build acts as the regression test
   for every cleanup.

## Progress

Progress is tracked on [decomp.dev](https://decomp.dev/Lynder063/rac1-decomp).

| Version | Region | Game ID | Code | Functions |
|---|---|---|---|---|
| v2.00 | PAL (En, Fr, De, Es, It) | `SCES_509.16` | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp) |

| Category | Progress | Contents |
|---|---|---|
| Game | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=game&label=Game&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=game) | Game and SDK code (`src/core/`, `src/game/`) |
| libgcc | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=libgcc&label=libgcc&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=libgcc) | GCC runtime library rebuilt from GCC's own source (`src/libgcc/`) |

The whole image already links with every function at its retail address.
Functions that are not decompiled yet are included as assembly.

### Level code

Each level carries its own build of the game program, loaded over the
executable's game code ([`docs/OVERLAYS.md`](docs/OVERLAYS.md)). Its
functions are decompiled in `src/overlays/` and counted once each:
code shared by two or more levels under *Common*, code found in one
level only under *Level-specific* and that level's own row.

| Category | Code | Functions | Contents |
|---|---|---|---|
| Level code | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_code&label=Level%20code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_code) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_code&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_code) | All level code (`src/overlays/`) |
| Common | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=common&label=Common&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=common) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=common&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=common) | Shared by two or more levels (`src/overlays/shared/`) |
| Level-specific | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=levels&label=Levels&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=levels) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=levels&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=levels) | Found in one level only (`src/overlays/lNN_<planet>/`, docs/OVERLAYS.md "Levels") |

<details>
<summary>Per level</summary>

| Level | Code | Functions |
|---|---|---|
| 00 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_00&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_00) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_00&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_00) |
| 01 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_01&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_01) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_01&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_01) |
| 02 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_02&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_02) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_02&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_02) |
| 03 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_03&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_03) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_03&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_03) |
| 04 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_04&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_04) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_04&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_04) |
| 05 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_05&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_05) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_05&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_05) |
| 06 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_06&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_06) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_06&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_06) |
| 07 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_07&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_07) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_07&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_07) |
| 08 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_08&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_08) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_08&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_08) |
| 09 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_09&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_09) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_09&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_09) |
| 10 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_10&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_10) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_10&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_10) |
| 11 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_11&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_11) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_11&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_11) |
| 12 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_12&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_12) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_12&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_12) |
| 13 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_13&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_13) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_13&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_13) |
| 14 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_14&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_14) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_14&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_14) |
| 15 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_15&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_15) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_15&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_15) |
| 16 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_16&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_16) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_16&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_16) |
| 17 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_17&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_17) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_17&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_17) |
| 18 | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_18&label=Code&measure=matched_code_percent)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_18) | [![](https://decomp.dev/Lynder063/rac1-decomp.svg?mode=shield&category=level_18&label=Functions&measure=matched_functions)](https://decomp.dev/Lynder063/rac1-decomp/SCES_509.16?category=level_18) |

</details>

## Disclaimer

This repository contains **no game assets, executable, or disassembly**. To
build it you need your own legally obtained copy of the game. Read
[`LEGAL.md`](LEGAL.md) before contributing.

## Building

The original compiler is SN Systems ProDG, a set of 32-bit Windows programs.
There are two ways to run it:

- **Windows**, natively, with **Git Bash** and Python 3.10 or newer.
- **Linux and macOS**, through 32-bit Wine in a container
  (`tools/docker/`). Works with **Podman** (Fedora, RHEL, etc.) or **Docker**
  (Ubuntu, Debian, macOS OrbStack/Docker Desktop). The image is pulled
  from GitHub Container Registry (`ghcr.io/lynder063/rac1-build:latest`),
  so there is no 15-minute local image build. The container build
  reproduces the Windows build's progress report byte for byte. See
  [`docs/CONTAINERS.md`](docs/CONTAINERS.md).

Every command below runs the same on both. On Linux and macOS, prefix it with
`bash tools/docker/run.sh` (which automatically pulls or builds the container and runs the command inside it):

```
bash tools/docker/run.sh bash tools/build_sn.sh
```

### 1. Clone

```
git clone https://github.com/Lynder063/rac1-decomp.git C:\rac1-decomp
```

On Windows keep the path short: the toolchain's `make` 3.77 fails with
`CreateProcess ... failed` when the repository path is long.

### 2. Add your executable

Copy `SCES_509.16` from your disc to `baserom/SCES_509.16`. From a disc
image, `bsdtar -xf game.iso -C baserom SCES_509.16` extracts it (the image
itself stays out of the way; `baserom/` is ignored by git). The expected
SHA-1 is:

```
79956931bd62fafd8d20fa2eae796dbaf2e15e83
```

### 3. Install Python dependencies and generate the disassembly

```
pip install -r requirements.txt      # Windows only; the Docker image has them
bash tools/setup_asm.sh
```

`setup_asm.sh` checks the executable's hash and the pinned splat and
spimdisasm versions, then generates `asm/` with
[splat](https://github.com/ethteck/splat). `asm/` is not tracked in git.

### 4. Get the toolchain

The build uses two community mirrors of the SN Systems / Sony PS2 toolchains.
They are third-party mirrors of commercial software and are not part of this
repository:

```
git clone https://github.com/AngheloAlf/SN-Systems-ProDG_for_PS2_3.01 toolchain/sn-prodg-3.01
git clone https://github.com/AngheloAlf/sce_ps2_sdk_24 toolchain/sn-prodg-24
```

- `sn-prodg-3.01` provides `make`, the assembler and the linker.
- `sn-prodg-24` provides the compilers:
  - GCC 2.95.3 (SN BUILD v1.14) for game code and the 989snd sound library;
  - Sony's `2.9-ee-991111` for Sony's SDK code and libgcc (the objects
    marked `ee29` in `config/core_text.objects`).

See [`docs/TOOLCHAIN.md`](docs/TOOLCHAIN.md) for how this was determined.

### 5. Build

```
bash tools/build_sn.sh
```

This builds and links `build-sn/rac1.elf`, then audits every decompiled
function against the retail executable on size and bytes. The output looks
like this (these are the numbers as of 2026-09-27; decomp.dev has the
current ones):

```
=== 1002 decompiled functions audited ===
  exact (size AND bytes): 989
  size mismatch:          0   (always revert these -- see docs)
  byte mismatch:          13
every function is at its retail address
image matches retail outside the decompiled near-misses (559 bytes differ inside them)
```

Hand-written assembly and the linker's dead-strip remnants are included as
assembly on purpose and count as finished in the progress report
([`docs/ASM_CLASSIFICATION.md`](docs/ASM_CLASSIFICATION.md)).

## Community

Come hang out with us! Join the **[Ratchet & Clank Decompilation Discord](https://discord.gg/Sfd2B54PDG)**.

Whether you're interested in matching functions, analyzing PS2 disassembly, researching engine quirks, or simply following along with the progress, everyone is warmly welcome!

## Project structure

| Path | Contents |
|---|---|
| `src/core/` | The `core_text` segment, one file per retail object, split at the retail linker's own fill between objects. Files are named by start address until their real source is identified (e.g. `989snd.c`) |
| `src/game/` | The `text` segment, one file per original source file (`hud`, `camera`, `mobyfunc`, `movie/*`...), named after the originals |
| `src/libgcc/` | GCC's `libgcc2.c` and `fp-bit.c` (GPL with the libgcc exception) plus stubs, see its README |
| `include/` | Shared headers, recovered structs, assembly macros |
| `include-sn/` | Assembly macros for assembling the data objects with SN's assembler |
| `config/splat.yaml`, `config/symbol_addrs.txt` | How the executable is split into functions |
| `config/core_text.objects`, `config/text.objects` | Link order and start address of every object |
| `Makefile.sn`, `rac1.ld.sh` | Compile and link at retail addresses |
| `tools/` | Build, audit, progress-report and decompilation helper scripts |
| `tools/docker/` | The build container and the Ghidra MCP container ([docs](docs/CONTAINERS.md)) |
| `../../../editor/` | Level editor (OpenRAC's top level): your own disc's levels as an editable Godot project ([README](../../../editor/README.md)) |
| `docs/` | Workflow, toolchain notes, progress log, containers, asset formats |
| `notes/` | Round notes from September 2026, kept as history; `docs/DECOMP_PROGRESS.md` has the current state |
| `progress/report.json` | objdiff-format progress report read by decomp.dev |

## Resources

- [Discord](https://discord.gg/Sfd2B54PDG): community server for chat, collaboration and questions
- [decomp.wiki](https://decomp.wiki): matching-decompilation knowledge base
- [decomp.dev](https://decomp.dev): progress tracking
- [splat](https://github.com/ethteck/splat),
  [spimdisasm](https://github.com/Decompollaborate/spimdisasm),
  [m2c](https://github.com/matt-kempster/m2c),
  [asm-differ](https://github.com/simonlindholm/asm-differ),
  [objdiff](https://github.com/encounter/objdiff)
- [AngheloAlf's PS2 toolchain mirrors](https://github.com/AngheloAlf)
- [Lombyte](https://github.com/mateuszklysz/Lombyte) (MIT): matching
  decompilation of the same game's NTSC build; some real names and struct
  layouts in `src/` comments (e.g. `src/game/draw.c`, `src/game/vuchain.c`)
  are corroborated against it, and `tools/lombyte.py` pairs its functions
  with ours as starting points (see `docs/SIBLING_DECOMPS.md`)
- [ReRAC](https://github.com/re-rac/rerac) (ISC): native PC port of the
  same game's US build; its format notes and parsers inform the level
  extractor (moby placements, models and animations, collision), its
  documented names feed
  `config/names.tsv` (`docs/NAMES.md`), and its notes on what functions do
  reach worker packets through `config/overlays/rerac_notes.tsv` and the
  US map (`docs/OVERLAYS.md`); see `THIRD_PARTY_NOTICES.md`
- [ratchet-uya-decomp](https://github.com/vetusmagnus/ratchet-uya-decomp):
  matching decompilation of R&C 3 with the same SN compiler; its compiler
  and flag research (per-file `-mno-split-addresses`) is summarised in
  `docs/SIBLING_DECOMPS.md`
- [Wrench](https://github.com/chaoticgd/wrench): Ratchet & Clank PS2 modding
  tools. Most of the level extractor's format knowledge comes from its
  source; OpenRAC's `editor/README.md` credits it and the other projects the
  extractor drew on


