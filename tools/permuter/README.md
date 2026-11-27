# tools/permuter

[decomp-permuter](https://github.com/simonlindholm/decomp-permuter) for every game in OpenRAC: one
engine, a small profile per game, and a queue runner for the build machines. It replaces the glue
each decomp wrote for itself (rac1-decomp's `tools/permuter_*.py`, the rac2 swarm's `permute.py`,
the boxes' `run_queue.py`, rac3-decomp's `tools/permuter_engine.py`); those stay in their projects
for the people who use them.

The permuter randomly rewrites a function's C (reorders statements, adds temporaries, changes
types, flips branches) and keeps whatever compiles closer to retail. Use it on near misses whose
instructions are right but whose registers or order are not; skip those that differ by a constant
or an address, which no rewrite moves.

## Layout

| File | What it does |
|---|---|
| `__main__.py`, `cli.py` | the command line: `python3 tools/permuter COMMAND` |
| `upstream.py` | the pinned decomp-permuter (8556c81), cloned once into the cache and patched; never vendored |
| `patches/` | rac3-decomp's two patches, rebased on the pin: `asm-passes.patch` (the fence and barrier passes), `aligned-scorer.patch` (an external-scorer hook for decomp-permuter's own command line) |
| `profile.py`, `profiles/*.json` | a game's profile: where a function's C and retail assembly are, how one candidate compiles, what the game's checker says for EXACT |
| `score.py`, `elf.py` | aligned instruction diffs of a compiled object against the function's splat `.s` |
| `engine.py` | the native search (rac3-decomp's engine, made game-agnostic): worker processes, live progress, results |
| `structural.py` | finite C transformations selected only where they apply; an optional alternative to upstream random retry |
| `accept.py` | the artefacts a result must not have |
| `runner.py` | the queue runner for the boxes |
| `test_permuter.py` | the tests: `python3 -m unittest discover -s tools` |

The cache is `$OPENRAC_PERMUTER_CACHE`, else `$XDG_CACHE_HOME/openrac`, else `~/.cache/openrac`:
the patched checkout (`decomp-permuter-8556c81/`), the workers' directories (`work/`) and the
profiles' shims. `$DECOMP_PERMUTER` names another checkout to use as it is. Needs Python 3.10 or
later (3.11+ for decomp-permuter's TOML without the `toml` package), git, and a host `gcc -E`.

## How it scores, and what 0 means

A candidate compiles through the game's own single-function tool or build steps, with the file's
own flags, so it compiles exactly as the project's checker compiles it. `score.py` then aligns the
object's function against the retail words in the function's splat `.s` (difflib on keys without
the fields a relocation fills, then each pair compared outside the fields either side relocates,
branches through the alignment, calls to `func_<address>` by name). The score is the number of
instructions that differ.

A 0 goes to the game's checker before anything is called a match. Only when it says EXACT does the
run stop as `matched`; a 0 it rejects scores 1. So **score 0 is always the project's own EXACT**,
and every profile was checked that way: a matched function's base scores 0 and its checker agrees.

## Running it

```sh
python3 tools/permuter list                                  # the profiles and their checkouts
python3 tools/permuter fetch                                 # clone and patch decomp-permuter (once)
python3 tools/permuter base rac4/ntsc func_00163218          # compile the base once: score and checker
python3 tools/permuter run rac4/ntsc func_00163218 -j8 --seconds 600
python3 tools/permuter run rac3/pal func_0038E1E8 --c ../rac3-decomp/nonmatching/frontbin/func_0038E1E8.c
```

Without `--c`, the run starts from the C the project has for the function (its definition in
`src/`, or its draft in `nonmatching/`); with it, from a draft: the declarations it needs, then the
function, the input the game's checker takes. The draft goes into the function's source file in
place of its stub, so base.c has the file's context. Results go to `build/permuter/<profile>/<func>/`
(`--out` for another place): `output-<score>-<n>/source.c` and `function.c` for each candidate that
ties or beats the base, `artefacts.txt` where it has some, and `result.json` with the best clean
candidate. `--no-asm-passes` leaves out the fence and barrier passes (they put empty inline asm in
the C, which rac1/pal's matching rules forbid; such a result is a hint to rewrite by hand).

The projects are found the usual way (`games.source_checkout`): `$OPENRAC_SOURCE_<GAME>_<VERSION>`,
else the version's submodule under `games/`. Set the variable only for a checkout kept elsewhere.

`--max-candidates N` limits a run's candidates, **including the base compiled by the upstream
constructor**. A bounded run needs `-j1`, so extra workers cannot multiply the allowance. A profile
can set a default `max_candidates` and a hard `candidate_ceiling`; an override above that ceiling
is rejected before preparing the backend. Existing profiles remain unbounded unless they opt in.
`result.json` reports attempted `candidates_including_base` and
`completed_candidates_including_base` separately; an interrupted compiler attempt still consumes
the allowance. A cached score can avoid recompiling a generated candidate. `--keep-work`, or a
profile's `keep_work`, retains compiler scratch evidence. A base that is already exact ends the
run without requesting a random seed or another candidate. Its C is still exported
as the winning result, so the queue can save it without generating a variant.

The time limit and a manual stop also interrupt retries inside the upstream randomizer. If a
worker does not finish during the short grace period, the engine stops that worker and its own
compiler/checker descendants. It does not stop unrelated processes. Cleanup can take a few
seconds beyond the search deadline, and interrupted work still produces a result receipt.

### Per game

| Profile | Checkout | Compiles with | Checker (EXACT) | Runs on |
|---|---|---|---|---|
| `rac1/pal` | rac1-decomp | `tools/permuter_compile.py` (try_func's `build()`: SN ee-gcc under Wine, `config/file_cflags.txt`, the fix-up passes) | `tools/try_func.py` (`EXACT`) | its build image, one container per session (`docker rm -f openrac-permuter-rac1` after) |
| `rac2/ntsc` | rac2-decomp | the SN `cpp`, `cc1`, `as` of `build/compiler/tools`, flags from `tools/build.py`'s `flags_for` | `tools/check.py` (`N of N places match`), with the candidate in the source file for the length of the check | its Linux image (`rac2-linux`), container `openrac-permuter-rac2` |
| `rac3/pal` | rac3-decomp | `tools/check.py --keep-object` (the full build's context and the function's own flags) | the same run's `MATCH` | natively: wibo (`$UYA_TOOLCHAIN`, `$UYA_RUNNER`); `$UYA_TARGET` picks frontbin, boot_elf or i5bootn |
| `rac3/us` | rac3-decomp | the same with `--version us`, scored against the NTSC-U twin (`config/us/map.tsv`) | `MATCH` | natively |
| `rac4/ntsc` | rac4-decomp | `tools/cc.sh` on a copy of the file with the candidate in place (its `/* cflags: */` comment, through `tools/layout.py`) | `tools/check_func.py --version us` (`MATCH`) | natively: a `wine` shim runs wibo; needs `asm/us/` (`tools/asm_programs.py --version us main ...`) |
| `rac4/pal` | rac4-decomp | the same | `check_func.py --version pal`, by US name mapped through `config/pal/map.tsv` | natively |
| `rac5/ntsc` | rac5-sm-decomp | its public `tools/rac5_permuter_backend.py`, canonical per-source flags and pinned tools | all recorded SNR2 placements, module hashes and named imports | Linux or WSL, with a native Linux scratch directory |

A profile is data (`profile.py` lists the keys): globs for the C and the assembly, the host
preprocessor for base.c, the compile steps and the checker as argument lists with placeholders,
the checker's EXACT as a regex, and what a machine needs (environment defaults, shims, a container
to start). A new game version is a new JSON file, verified the same way: `base` on a matched
function must print `base score 0` and `EXACT`.

### Size Matters level C

Use a RAC5 checkout containing `tools/rac5_permuter_backend.py`; an older pinned submodule may
predate this hook. Set `OPENRAC_SOURCE_RAC5_NTSC` to that checkout and `RAC5_BACKEND` to the script.
Its runtime settings are documented in RAC5's `docs/contributing/permuter.md`, with the private
configuration based on `config/permuter-runtime.example.json`. No runtime configuration or game
data belongs in OpenRAC. Follow [RAC5's contribution rules](https://github.com/OpenRAC/rac5-sm-decomp/blob/master/CONTRIBUTING.md).

Also set `RAC5_MATCH_RUNTIME` to the private configuration, `RAC5_SOURCE_GLOB` to the precise source
path/glob relative to RAC5, and `RAC5_RETAIL_DIR` to the private retail cache expressed relative to
that checkout. Prepare only the function being worked on:

```sh
python3 "$RAC5_BACKEND" --repo "$OPENRAC_SOURCE_RAC5_NTSC" --runtime "$RAC5_MATCH_RUNTIME" \
  prepare --function FUNCTION --source-key /path/to/source.c
python3 tools/permuter base rac5/ntsc FUNCTION --no-asm-passes
python3 tools/permuter run rac5/ntsc FUNCTION -j1 --max-candidates 3 --seconds 60 \
  --no-asm-passes --out /path/to/private/result
```

This profile has a hard ceiling of three candidates, base included, and retains work files by
default. It never rewrites the RAC5 source tree. Its explicit `structural_passes` replace the
upstream random-weight search with four finite C transformations:

- reverse an `if` with an `else`, or a ternary, using logical negation and exchanged branches;
- exchange adjacent uninitialized scalar or pointer declarations without moving initializers,
  address-taken locals, variable-length arrays or inline tag definitions;
- turn an eligible returning branch into an early-return guard, retaining lexical scopes;
- turn a `while` with an eligible pure condition into a guarded `do` loop, retaining the zero-trip
  case and the original condition evaluations.

Only applicable sites are tried. Each rewrite starts from the original base, and an exhausted
site list ends normally instead of retrying forever. `result.json` records `applicable_passes`
and `stop_reasons`. With nonempty `structural_passes`, random weights do not choose the search
and asm passes are disabled. Other profiles retain upstream search unless they opt in. RAC5
also sets `asm_passes=false`, so neither its profile nor the explicit `--no-asm-passes` command
can enable the optional empty-asm passes.

The scope is level C supported by the parser and measured tool route: not C++, the boot or a
whole-game qualification. Transforms still need semantic, ABI and boundary review, particularly
for FPU/NaN conditions, aliases and side effects. A placement check or a `matched` status does not
promote C or replace RAC5's full-program gates. A zero score with a wrong zero-valued SNR2 import
is rejected by the backend. Nonzero scores from different diff algorithms need not agree.

Its cached checker may rerun preprocessing and the source policy to detect changed headers or
macros. That validation does not run CC1, the assembler or linker again and does not create a new
source candidate; the candidate ceiling still includes each constructor/source variation.

Run the focused tests with the pinned checkout available as a Python module:

```sh
PYTHONPATH="tools:$DECOMP_PERMUTER" python3 -m unittest discover -s tools/permuter -p 'test_*.py'
```

The structural tests require upstream's `perm_pycparser`; they report explicit skips if it is
absent. With a host GCC installed, the semantic tests compile and execute synthetic C including
NaN comparisons, side effects, lexical scopes and loop exits. These checks test the tool's
rewrites; they do not qualify game functions or prove arbitrary C programs equivalent.

## Accepting a result

Only plausible C is accepted (WORKFLOW.md). The search takes any C that compiles closer, including
C that does something else, so every result is a hint until a person has read it. `accept.py`
flags what a program can see, comparing the candidate with the base so that only new ones count:

- **duplicated calls**: the same call statement in both branches of an `if`;
- **dead stores**: a local assigned and assigned again before a read, or never read;
- **uninitialised reads**: a local without a value whose first use reads it.

A 0 with artefacts does not end the run and is never a WIN. The reader checks the rest: an argument
that changed (0 instead of a value), a type narrowed (`u16` for `s32`), a store dropped, inline asm
(the fence and barrier passes), and in general whether the code does what the original did. Then
the useful change is rewritten into the real source by hand and checked with the game's own tools.

## The queue runner on the boxes

One queue per machine, any games, a few runs at a time:

```sh
python3 tools/permuter queue add rac3/pal func_003B3440 --c draft.c --env UYA_TARGET=boot_elf
python3 tools/permuter queue import-legacy ~/Projects/RAC/permuter --profile rac3/pal
nohup python3 tools/permuter queue run --slots 7 -j4 --minutes 20 > queue.log 2>&1 &
python3 tools/permuter queue status
```

The queue is a directory (`--dir`, `$OPENRAC_PERMUTER_QUEUE`, default `build/permuter/queue`):
`queue.tsv` (profile, func, draft, env, minutes), `done.tsv` (profile, func, base, best, verdict,
seconds, result), `runs/` and `wins/`. `PAUSE` stops new runs from starting; `STOP` ends the
running ones, which are not recorded and so run again next time, and the runner. Each run is under
`nice -n 10`; at most `--slots` runs of `-j` workers each; nothing starts while the one-minute load
average is at or above `--max-load` (default: the CPU count). Verdicts are `WIN` (score 0, the
checker said EXACT, no artefacts: the function is in `wins/<profile>/<func>.c`), `artefact`,
`better`, `none` and `error`.

`import-legacy` reads the boxes' old rac3 queue (`queue/queue.tsv`, its drafts in `queue/src/`,
leaving out what its `done.tsv` and `skip.tsv` list) without changing it. The old queue keeps
working with its own `run_queue.py`. The rule of thumb from WORKFLOW.md stands: 7 slots of `-j4`
on box1's 32 threads, 9 of `-j6` on box2's 64, paused when a build needs the machine.

## Not covered yet

- rac3's levels (`level_NN` targets) and `@ee29` ranges; rac2 files built with the SDK compiler or
  `-Wa,` options; rac1's NTSC-U tree; rac5's boot and C++ sources. Each needs its profile (or a key
  in it) and a check.
- rac3-decomp's exhaustive fence search (`tools/fence_search.py`) and localdecomp's Permuter panel
  stay in rac3-decomp; the engine keeps the panel's state format (`Job.snapshot`).
- On Windows the engine needs a profile without shell steps or shims; none is written yet.
