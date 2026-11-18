# The decompilations

Everything about the decompilation projects in `games/` that is not inside a
game's own directory. The port and the launcher do not depend on these pages.

| Page | For |
|---|---|
| [WORKFLOW.md](WORKFLOW.md) | How matching works in each game, side by side, and porting between games |
| [TOOLCHAINS.md](TOOLCHAINS.md) | Which compilers built what, and what each project uses to match |
| [../port/BACKFILL.md](../port/BACKFILL.md) | When to stop matching, and the C the port needs instead |

## Rules for matching work

These come on top of [CONTRIBUTING.md](../../CONTRIBUTING.md).

- **Work inside one game version.** Run its commands from its directory
  (the `dir` in its `game.json`: `games/<game>/decomp/`, the game's
  decompilation, for every version built from it; rac1/ntsc's is `games/rac1/ntsc-decomp/`). A game's build never reaches into another game's directory.
- **Each game proves a match its own way**: a byte-identical rebuild, a strict
  per-function check, or both. Its docs and checks have the last word.
- **Matches are plain C.** In rac1/pal: no register pins, no inline assembly
  in a function, no artificial barriers, no `#define` in a candidate, no
  expression aliases, and the build never edits compiler output or gives one
  function flags of its own ([BUILD_FIDELITY.md](../../games/rac1/decomp/docs/BUILD_FIDELITY.md)).
- **Claim before you match** when several people work on rac1/pal at once:
  `python3 tools/claims.py` ([AGENT_WORKFLOW.md](../../games/rac1/decomp/docs/AGENT_WORKFLOW.md)).
- **Imports from a project's own repository** go through
  `tools/sources.py update`, are reviewed against the sourcing policy, and are
  committed as `chore(import)` ([SOURCES.md](../SOURCES.md#updating-a-project)).

## Each game's documents

### Ratchet & Clank, PAL ([games/rac1/decomp](../../games/rac1/decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac1/decomp/README.md), [CONTRIBUTING.md](../../games/rac1/decomp/CONTRIBUTING.md), [LEGAL.md](../../games/rac1/decomp/LEGAL.md) | Setup and build, contributing, what the project contains |
| [docs/WORKFLOW.md](../../games/rac1/decomp/docs/WORKFLOW.md) | From an `INCLUDE_ASM` stub to matched C, step by step |
| [docs/LEVERS.md](../../games/rac1/decomp/docs/LEVERS.md), [docs/DECOMP_PROGRESS.md](../../games/rac1/decomp/docs/DECOMP_PROGRESS.md) | Matching levers; the knowledge base of solved questions and dead ends |
| [docs/TOOLCHAIN.md](../../games/rac1/decomp/docs/TOOLCHAIN.md) | What builds the image and why (SN ProDG, flags, pipeline) |
| [docs/OVERLAYS.md](../../games/rac1/decomp/docs/OVERLAYS.md) | The 19 level programs, variants, the US to PAL map |
| [docs/ASSETS.md](../../games/rac1/decomp/docs/ASSETS.md) | Disc, WAD and level data formats, with evidence; what the editor reads |
| [docs/NAMES.md](../../games/rac1/decomp/docs/NAMES.md), [docs/NONMATCHING.md](../../games/rac1/decomp/docs/NONMATCHING.md), [docs/ASM_CLASSIFICATION.md](../../games/rac1/decomp/docs/ASM_CLASSIFICATION.md) | Symbol names, near-miss staging, handwritten assembly |
| [docs/AGENT_WORKFLOW.md](../../games/rac1/decomp/docs/AGENT_WORKFLOW.md), [docs/QUEUE.md](../../games/rac1/decomp/docs/QUEUE.md), [docs/WORKER.md](../../games/rac1/decomp/docs/WORKER.md), [docs/LONG_FUNCTIONS.md](../../games/rac1/decomp/docs/LONG_FUNCTIONS.md), [docs/LLM_DECOMP_INSTRUCTIONS.md](../../games/rac1/decomp/docs/LLM_DECOMP_INSTRUCTIONS.md) | AI agent waves, worker protocols, claims (the last in Czech) |
| [docs/PERMUTER.md](../../games/rac1/decomp/docs/PERMUTER.md), [docs/CONTAINERS.md](../../games/rac1/decomp/docs/CONTAINERS.md) | decomp-permuter; the build and Ghidra containers |
| [docs/SIBLING_DECOMPS.md](../../games/rac1/decomp/docs/SIBLING_DECOMPS.md) | The other projects and how to port from them |
| [docs/DATAMINED_REVERSE_ENGINEERING.md](../../games/rac1/decomp/docs/DATAMINED_REVERSE_ENGINEERING.md) | A generated Ghidra datamining report |
| [notes/](../../games/rac1/decomp/notes) | Dated investigation logs and per-function write-ups (60 files) |
| [nonmatching/README.md](../../games/rac1/decomp/nonmatching/README.md) | Index of staged near misses (generated) |

### Ratchet & Clank, NTSC-U ([games/rac1/ntsc-decomp](../../games/rac1/ntsc-decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac1/ntsc-decomp/README.md), [CONTRIBUTING.md](../../games/rac1/ntsc-decomp/CONTRIBUTING.md) | The project, its seven-step contribution loop |
| [docs/build/building.md](../../games/rac1/ntsc-decomp/docs/build/building.md) | Automatic and manual builds, environment variables, targets |
| [docs/contributing/decompilation-tips.md](../../games/rac1/ntsc-decomp/docs/contributing/decompilation-tips.md), [docs/build/progress.md](../../games/rac1/ntsc-decomp/docs/build/progress.md) | Acceptance discipline and matching tips; what C_EXACT and C_FUZZY measure |
| [patches/README.md](../../games/rac1/ntsc-decomp/patches/README.md) | The patched EE-GCC: lineage, rules, hashes |
| [docs/build/overlays.md](../../games/rac1/ntsc-decomp/docs/build/overlays.md) | Level programs in the US build |
| [docs/game/engine-source-layout.md](../../games/rac1/ntsc-decomp/docs/game/engine-source-layout.md), [docs/game/moby-selector-scratch-path.md](../../games/rac1/ntsc-decomp/docs/game/moby-selector-scratch-path.md), [docs/README.md](../../games/rac1/ntsc-decomp/docs/README.md) | Engine source layout (from prerelease builds), a moby data path, the names catalog |
| [docs/contributing/commit-messages.md](../../games/rac1/ntsc-decomp/docs/contributing/commit-messages.md) | What a good commit summary and body say (OpenRAC's convention applies) |
| [config/README.md](../../games/rac1/ntsc-decomp/config/README.md), [src/README.md](../../games/rac1/ntsc-decomp/src/README.md) | Directory notes |

### Ratchet & Clank: Going Commando ([games/rac2/decomp](../../games/rac2/decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac2/decomp/README.md), [docs/BUILDING.md](../../games/rac2/decomp/docs/build/building.md), [CONTRIBUTING.md](../../games/rac2/decomp/CONTRIBUTING.md), [AGENTS.md](../../games/rac2/decomp/AGENTS.md) | The project, first steps, contribution rules, agent rules |
| [docs/BUILDING.md](../../games/rac2/decomp/docs/build/building.md), [docs/LEVELS.md](../../games/rac2/decomp/docs/build/levels.md), [docs/COMPILER-NOTES.md](../../games/rac2/decomp/docs/build/compiler-notes.md) | Building (Docker on any host), the level programs and the catalogue, the compiler |
| [tools/compiler/README.md](../../games/rac2/decomp/tools/compiler/README.md) | The compilers rebuilt from source |
| [docs/COMPILER-NOTES.md](../../games/rac2/decomp/docs/build/compiler-notes.md) | The C compiler profile and its patches |
| [docs/MOBY-DISPATCH-TABLES.md](../../games/rac2/decomp/docs/game/moby-dispatch-tables.md), [docs/VU-MICROPROGRAMS.md](../../games/rac2/decomp/docs/game/vu-microprograms.md) | Class dispatch tables, VU microprograms |
| [docs/COMMUNITY-ENGINE-REFERENCE.md](../../games/rac2/decomp/docs/game/community-engine-reference.md) | A reverse-engineering reference ("reference, never evidence") |
| [docs/RAC1-TO-RAC2.md](../../games/rac2/decomp/docs/research/rac1-to-rac2.md) | What carries over from RAC1 |
| [docs/PROTOTYPE-BUILDS.md](../../games/rac2/decomp/docs/research/prototype-builds.md), [docs/AUG6-RETAIL-ANCHORS.md](../../games/rac2/decomp/docs/research/aug6-retail-anchors.md), [docs/ENGINE-SYMBOL-NAMES.md](../../games/rac2/decomp/docs/game/engine-symbol-names.md), [docs/ASSERT-MESSAGE-NAMES.md](../../games/rac2/decomp/docs/game/assert-message-names.md) | Prerelease builds and the names drawn from them; names from retail assert messages |

### Ratchet & Clank: Up Your Arsenal ([games/rac3/decomp](../../games/rac3/decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac3/decomp/README.md), [CONTRIBUTING.md](../../games/rac3/decomp/CONTRIBUTING.md) | The project, contribution rules |
| [docs/wiki/Home.md](../../games/rac3/decomp/docs/wiki/Home.md) | The wiki's index: [Setup](../../games/rac3/decomp/docs/wiki/Setup.md), [Toolchain and build](../../games/rac3/decomp/docs/wiki/Toolchain-and-Build.md), [Workflow](../../games/rac3/decomp/docs/wiki/Workflow.md), [Matching patterns](../../games/rac3/decomp/docs/wiki/Matching-Patterns.md), [Pull requests](../../games/rac3/decomp/docs/wiki/Pull-Requests.md), [Tools](../../games/rac3/decomp/docs/wiki/Tools.md), [Cross-repository resources](../../games/rac3/decomp/docs/wiki/Cross-Repository-Resources.md) |
| [docs/us/compiler_matrix_findings.md](../../games/rac3/decomp/docs/research/us/compiler_matrix_findings.md) | 15 compilers against 8 flag sets: what built the game |
| [docs/us/source_files.md](../../games/rac3/decomp/docs/research/us/source_files.md), [docs/trailing_padding.md](../../games/rac3/decomp/docs/build/trailing-padding.md) | How the source files are laid out; trailing padding |
| [docs/versions.md](../../games/rac3/decomp/docs/build/versions.md), [docs/levels.md](../../games/rac3/decomp/docs/build/levels.md), [docs/us/shared_code_findings.md](../../games/rac3/decomp/docs/research/us/shared_code_findings.md) | PAL and NTSC-U from one tree; the level programs; code shared between levels |
| [docs/us/full_match_roadmap.md](../../games/rac3/decomp/docs/research/us/full_match_roadmap.md), [docs/permuter.md](../../games/rac3/decomp/docs/contributing/permuter.md), [docs/us/permuter_todo.md](../../games/rac3/decomp/docs/research/us/permuter_todo.md) | The plan to 100%; decomp-permuter and its worklist |

### Ratchet: Deadlocked ([games/rac4/decomp](../../games/rac4/decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac4/decomp/README.md), [CONTRIBUTING.md](../../games/rac4/decomp/CONTRIBUTING.md) | The project, setup, adding a function, matching tips, naming, commits |
| [LEGAL.md](../../games/rac4/decomp/LEGAL.md) | What may be committed, allowed sources, the naming rule |
| [docs/RESEARCH.md](../../games/rac4/decomp/docs/RESEARCH.md) | The packed executable, its 17 sections, the compiler and assembler, libgcc and libm, open questions |
| [docs/OVERLAYS.md](../../games/rac4/decomp/docs/OVERLAYS.md) | The 47 level overlays: obtaining, splitting, counting and disassembling them |
| [docs/CREDITS.md](../../games/rac4/decomp/docs/CREDITS.md), [THIRD_PARTY_NOTICES.md](../../games/rac4/decomp/THIRD_PARTY_NOTICES.md) | Projects it drew on; GCC and newlib notices |

### Ratchet & Clank: Size Matters ([games/rac5/decomp](../../games/rac5/decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac5/decomp/README.md), [CONTRIBUTING.md](../../games/rac5/decomp/CONTRIBUTING.md), [AGENTS.md](../../games/rac5/decomp/AGENTS.md) | The project, what is established, how to contribute and check work |
| [LEGAL.md](../../games/rac5/decomp/LEGAL.md) | What may be committed, allowed sources |
| [docs/ROADMAP.md](../../games/rac5/decomp/docs/ROADMAP.md) | The gates still open |
| [docs/REL-FORMAT.md](../../games/rac5/decomp/docs/REL-FORMAT.md), [docs/BOOT-SYMBOLS.md](../../games/rac5/decomp/docs/BOOT-SYMBOLS.md) | The SNR2 level modules; the boot's named symbols |
| [docs/BOOT-TRIAL-GATE.md](../../games/rac5/decomp/docs/BOOT-TRIAL-GATE.md), [docs/BOOT-MATCH-SCOPE.md](../../games/rac5/decomp/docs/BOOT-MATCH-SCOPE.md) | How a function is accepted, and what a boot match covers |

### Ratchet & Clank: Secret Agent Clank ([games/rac6/decomp](../../games/rac6/decomp))

| Document | For |
|---|---|
| [README.md](../../games/rac6/decomp/README.md), [CONTRIBUTING.md](../../games/rac6/decomp/CONTRIBUTING.md), [AGENTS.md](../../games/rac6/decomp/AGENTS.md) | The project, what is established, how to contribute and check work |
| [LEGAL.md](../../games/rac6/decomp/LEGAL.md) | What may be committed, allowed sources |
| [PROMPT.md](../../games/rac6/decomp/PROMPT.md) | The starter prompt and wave protocol |
| [docs/STATE.md](../../games/rac6/decomp/docs/STATE.md) | Where the project stands, open gates |
| [docs/METHOD.md](../../games/rac6/decomp/docs/METHOD.md), [docs/GATES.md](../../games/rac6/decomp/docs/GATES.md) | Wave protocol, body gate, placement gate, complete-program gate |
