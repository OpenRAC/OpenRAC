# Contributing

## Matching a function

1. Set up the build once ([docs/BUILDING.md](docs/BUILDING.md)) and run `tools/build.py all`.
2. Pick a function from `config/boot/functions.tsv` or `config/levels/functions.tsv`. A level
   function's row lists all its places; one definition serves them all.
3. Find its `INCLUDE_ASM` line in `src/` (`grep -rn func_XXXXXXXX src`) and replace it with
   its C, in place. Name what it uses by what it is: other functions by their catalogue names,
   the boot's resident data `D_XXXXXXXX`, a level's data `D_LNN_XXXXXXXX` (its address in level
   NN). Never write a level address as a number: a level function is linked into every level
   that has it, each with its own addresses.
4. `tools/check.py func_XXXXXXXX` compiles its file and tries it at each of its places in a few
   seconds; `--diff` shows the instructions beside the retail ones.
5. `tools/build.py` the programs that have it (`boot`, `level 4`, or `all`). It places the
   function everywhere the catalogue says and prints `OK` per program, or why a place does
   not match. A function counts once its C links into a program that equals the retail one.
6. Leave `progress/report.json` and the README table alone: a maintainer regenerates them on
   `master` after merging, so pull requests do not conflict on them (AGENTS.md, "The progress report").

Close but not matching: put it in `nonmatching/` with a line in its README.

## Improving shared knowledge

Every contributor can edit any entry in [`docs/MATCHING-BLOCKERS.md`](docs/MATCHING-BLOCKERS.md)
using a fork/topic branch and a normal PR, even when another person wrote the original note.
Maintainers review the merge; they are not the sole authors. A coding AI edits this Markdown
file in the same checkout, with no separate wiki clone or browser automation.

Read existing findings before another trial. Add new measurements or correct a superseded
conclusion in the relevant entry, with the profile, applicable release, result, remaining
difference and PR/commit link. Preserve useful dated history and distinguish local candidates
from merged fixes. Discuss uncertain findings in the relevant code or documentation PR;
issue #157 is retired and kept only as historical provenance. Reservations stay in issue #152.

## Sending your work

Branch from `master`, commit, push the branch and open a pull request to `master`. In the pull
request, give the commands you ran and what they printed (`tools/build.py all --strict`,
`tools/progress.py`, the tests). CI runs the tests and the repository check; a maintainer merges.
Never push to `master` and never force-push a shared branch. AGENTS.md has the full rules and docs/AGENT_WORKFLOW.md says which model takes which functions, for
people as much as for agents.

Use `--strict` for the final build before pushing: it fails if any C function falls back
to assembly, even when the resulting program equals retail. Ordinary builds keep that
fallback available while experimenting. Compiler and assembler changes automatically
invalidate the affected cached C objects.

## Rules

- **Sources.** Use the retail programs of your own disc, SDK library *binaries*, open-source code
  under its licence, public decompilations and tools, and public hardware documentation. Never
  use Sony's SDK source, samples or headers, or leaked or NDA material of any kind (leaked
  builds, symbol dumps from them, leaked source), not even as a reference. Publicly distributed
  prototype discs may be cited, naming the build, never as the evidence for a match. OpenRAC's
  docs/policy/SOURCING.md is the full policy.
- Follow RAC1's source practices: meaningful C may call the reviewed shared EE helpers
  `qcopy`, `qcopy_nc` and `qzero` in `include/common.h`. Public runtime-library target
  primitives may retain their original assembly with upstream provenance and licence.
  No ad-hoc instruction sequences in game functions, register pins, artificial barriers,
  fake dependencies or false interfaces. Flags are per file (`config/cflags.txt`), never
  per function. These exceptions still require exact linked validation; see `AGENTS.md`.
- The build may model measured retail assembler/linker behavior, following RAC1:
  macro expansion, hardware padding and data-section placement need independent
  controls and strict linked validation. Never patch compiler-authored operations,
  values or register assignments to manufacture a match.
- Never commit game data, extracted assembly, disc images or proprietary tools: `baserom/`,
  `toolchain/` and `build/` stay local.
- Conventional commit subjects (`feat(src):`, `fix(tools):`, `docs:` ...), with a body that
  says what changed and how it was checked.
- `python -m unittest discover -s tests`, `python tests/check_repository.py` and
  `python tools/progress.py --check` pass (CI runs
  them; it cannot build, having no game data).
