# Docs

## Start here

| Doc | What it covers |
|---|---|
| [BUILD.md](BUILD.md) | Build the game and check the match |
| [WORKFLOW.md](WORKFLOW.md) | Match a function, from start to finish |
| [STYLE.md](STYLE.md) | How the C should look |
| [REVIEWING.md](REVIEWING.md) | What to check in a PR |
| [INCLUDES.md](INCLUDES.md) | Include order and function headers |
| [NAMES.md](NAMES.md) | How symbols get readable names |
| [OVERLAYS.md](OVERLAYS.md) | Level code |
| [VERSIONS.md](VERSIONS.md) | US, PAL and JP |

## Where things are

| Path | What it holds |
|---|---|
| `src/core/` | Kernel, libc, CD, graphics, sound and other low-level code, by system |
| `src/game/` | Game code, by system. These files are the original compile units, so they are not split |
| `src/overlays/` | Level code: one folder per level, and `shared/` for code in many levels |
| `src/data/` | Data in C: names, types and owners. Values come from your baserom at build time |
| `src/versions/` | Functions that differ in US, JP or the other PAL build |
| `include/` | `structs/`, `enums/`, function headers, and `names/` (generated) |
| `config/` | Symbol names, file lists, version maps, and the counts CI checks |
| `tools/` | Build and checks. `tools/tidy/` holds the scripts that clean up the source |
| `tools/old/` | Tools no longer in use |

## Checks

| Command | What it does |
|---|---|
| `bash tools/check_all.sh` | Everything: build, all versions, reports, format and counts |
| `python3 tools/gate_loop.py` | The same, and it reverts any file that breaks a match |
| `bash tools/land_pr.sh BRANCH` | Merges a PR once its newest commit passes |
| `python3 tools/ratchets.py --check` | Counts of things to reduce. They may only go down |
| `python3 tools/fakematch_scan.py --check` | Every fakematch site has a row in `config/fakematch.tsv` |

## What is left

The match is finished. This is what could still read better:

- **Raw offsets and local structs.** Each one needs its record identified, so no tool can finish them. See the counts in
  `config/ratchets.json`.
- **Fakematch sites.** `config/fakematch.tsv` lists each one and what it does. Each has had one reasoned try at a natural
  form, and those that are left changed the code. Only a new idea is worth another try.
- **Fake bases.** About 600 sites in shared C and 340 in version fragments still spell a global as
  `hero_globals_base + 0xE1D`. `tools/tidy/fake_bases.py` tests each function it can place, local aliases included: these
  change the code or clash with another view.
- **Small-data globals.** `tools/tidy/gp_scalars.py` converts `extern short x` read through one type. Those left are real
  shorts, or are read as two types, or fail the match.
- **Moby class numbers.** `oclass_literals` rose when raw offsets became real `oclass` fields. The classes left, like
  0x28F, are in no name source, so they stay numbers.
- **Control modes.** `enum HeroControlMode` names what the code proves. 0x09, 0x0A, 0x0B, 0x10, 0x13, 0x15, 0x16 and 0x18
  are still numbers.
- **Sound and animation IDs.** No name data exists for them.
- **Data shapes.** All of `.data` is in C, but many blocks are still plain words of an unknown record. See
  `config/data_in_c.tsv`, and give a block a real type with `tools/data_in_c.py set-type`.

## Game knowledge

[kb/README.md](kb/README.md) covers how the game works: architecture, data structures and mobys.

## Reference

Long and detailed. Read these when you need them.

| Doc | What it covers |
|---|---|
| [TOOLCHAIN.md](reference/TOOLCHAIN.md) | Compilers and assemblers, and why each is used |
| [BUILD_FIDELITY.md](reference/BUILD_FIDELITY.md) | Every step between the compiler and the image |
| [CONTAINERS.md](reference/CONTAINERS.md) | The build and Ghidra containers |
| [DECOMP_PROGRESS.md](reference/DECOMP_PROGRESS.md) | History log and solved problems |
| [ASM_CLASSIFICATION.md](reference/ASM_CLASSIFICATION.md) | Which code was assembly in the original |
| [NONMATCHING.md](reference/NONMATCHING.md) | Near-misses and how they are kept |
| [PVARS.md](reference/PVARS.md) | Moby private variables |
| [ASSETS.md](reference/ASSETS.md) | Level data and the Godot export |
| [MOVIE.md](reference/MOVIE.md) | Movie player code |
| [DATAMINED_REVERSE_ENGINEERING.md](reference/DATAMINED_REVERSE_ENGINEERING.md) | Findings from data mining |
| [GDSCRIPT_CONVENTIONS.md](reference/GDSCRIPT_CONVENTIONS.md) | Rules for `.gd` scripts |

## For agents

| Doc | What it covers |
|---|---|
| [AGENT_WORKFLOW.md](agents/AGENT_WORKFLOW.md) | How agent waves run |
| [WORKER.md](agents/WORKER.md), [QUEUE.md](agents/QUEUE.md) | Worker instructions |
| [LLM_DECOMP_INSTRUCTIONS.md](agents/LLM_DECOMP_INSTRUCTIONS.md) | Decompiling with an LLM |
| [LEVERS.md](agents/LEVERS.md) | What closes a near-miss |
| [LONG_FUNCTIONS.md](agents/LONG_FUNCTIONS.md) | Working on long functions |
| [PERMUTER.md](agents/PERMUTER.md) | The permuter (not in use for now) |
| [SIBLING_DECOMPS.md](agents/SIBLING_DECOMPS.md) | Related projects and porting |
