# Instructions for AI agents

These rules apply to any AI agent working in OpenRAC (Claude, Codex and
others), on top of [CONTRIBUTING.md](CONTRIBUTING.md), which applies to
everyone. When a game's own instructions are stricter, follow them as well.

## What this repository is

OpenRAC brings together the decompilation projects of the PlayStation 2
Ratchet & Clank games: one self-contained project per game version under
`games/`, a shared level editor in `editor/`, repository tools in `tools/`,
and the cross-game docs in `docs/`. Matching decompilation is the method and
the proof of correctness; C that compiles and runs on PC is the long-term
goal. Start with [docs/LAYOUT.md](docs/LAYOUT.md).

## Where to read first

| Working on | Read |
|---|---|
| Anything | [docs/policy/SOURCING.md](docs/policy/SOURCING.md), [docs/LAYOUT.md](docs/LAYOUT.md), [CONTRIBUTING.md](CONTRIBUTING.md) |
| rac1/pal | [games/rac1/pal/README.md](games/rac1/pal/README.md), [docs/WORKFLOW.md](games/rac1/pal/docs/WORKFLOW.md), [docs/AGENT_WORKFLOW.md](games/rac1/pal/docs/AGENT_WORKFLOW.md), [docs/LLM_DECOMP_INSTRUCTIONS.md](games/rac1/pal/docs/LLM_DECOMP_INSTRUCTIONS.md) |
| rac1/ntsc | [games/rac1/ntsc/CONTRIBUTING.md](games/rac1/ntsc/CONTRIBUTING.md), [docs/decompilation-tips.md](games/rac1/ntsc/docs/decompilation-tips.md), [docs/building.md](games/rac1/ntsc/docs/building.md) |
| rac2 | [games/rac2/ntsc/AGENTS.md](games/rac2/ntsc/AGENTS.md), [docs/START-HERE.md](games/rac2/ntsc/docs/START-HERE.md), [CONTRIBUTING.md](games/rac2/ntsc/CONTRIBUTING.md) |
| rac3 | [games/rac3/ntsc/CONTRIBUTING.md](games/rac3/ntsc/CONTRIBUTING.md), [docs/wiki/Workflow.md](games/rac3/ntsc/docs/wiki/Workflow.md), [docs/wiki/Matching-Patterns.md](games/rac3/ntsc/docs/wiki/Matching-Patterns.md) |
| rac4 | [games/rac4/README.md](games/rac4/README.md), [ntsc/CONTRIBUTING.md](games/rac4/ntsc/CONTRIBUTING.md), [ntsc/LEGAL.md](games/rac4/ntsc/LEGAL.md), [ntsc/docs/RESEARCH.md](games/rac4/ntsc/docs/RESEARCH.md) |
| The editor | [editor/README.md](editor/README.md), [editor/GDSCRIPT_CONVENTIONS.md](editor/GDSCRIPT_CONVENTIONS.md) |
| Cross-game knowledge | [docs/engine](docs/engine/README.md), [docs/toolchains](docs/toolchains/README.md), [docs/workflow](docs/workflow/README.md) |

## Rules

1. **Sources.** Use only what [SOURCING.md](docs/policy/SOURCING.md) allows:
   the retail programs of a disc the user owns, SDK library binaries,
   open-source code under its license, public projects about these games.
   Never Sony SDK source, samples or headers; never leaked or NDA material,
   not even to check a match. If a function looks like SDK sample code,
   decode it from the assembly.
2. **Removal requests.** Refer to the third-party NTSC decompilation of
   Ratchet & Clank only as "the NTSC decomp", never by its author's name,
   handle or address, and never re-add material a project removed at
   someone's request. When importing or porting from a sister project,
   check before committing; git history keeps what is committed.
3. **Game data stays local.** Never commit or upload disc images,
   executables, extracted data, generated assembly, build output or
   toolchains, and never paste retail bytes into a file, issue or message.
   They live in ignored directories (`baserom/`, `toolchains/`, `assets/`,
   `build/`, each game's `asm/` and build directories).
4. **Work inside one game version.** Run its commands from its directory
   (`games/<game>/<version>/`); its paths are relative to it. Do not make
   one game's build depend on another's files.
5. **Plain C in matches.** Follow the game's matching rules. In rac1/pal:
   no register pins, no inline assembly in functions, no artificial
   barriers, no `#define` in candidates, no expression aliases. Every game
   requires its own proof (a byte-identical build, a strict per-function
   check, or both) before a function counts as matched.
6. **Commits.** Conventional Commits, `type(scope): summary`, with the types
   and scopes in [CONTRIBUTING.md](CONTRIBUTING.md#commits): for example
   `feat(rac1/pal): func_L05_002D48B8 exact match` or
   `docs(engine): describe the moby dispatch tables`. Commit granularly, one
   logical change at a time, each leaving the build and tests working; keep
   regenerated output in its own commit, except where a game's checks need it
   with the source (rac1/pal's `progress/report.json`). Write a body that says what changed,
   why, and what you verified. End it with a `Co-Authored-By:` line naming
   the assistant ([open question 2](docs/policy/OPEN_QUESTIONS.md#2-credit-lines-for-ai-assistants-in-commits)).
7. **Never push, open pull requests or publish anything** unless the person
   you work for asks for that specific action. Never rewrite history that
   has been pushed. Where an imported doc says to commit per function or to
   push (rac1/pal's `docs/LLM_DECOMP_INSTRUCTIONS.md`), these rules win. Do
   not install a project's own git hooks: `.git/hooks` serves all of OpenRAC.
   rac3's `localdecomp` keeps its auto-commit and push off inside OpenRAC.
8. **Never delete or overwrite what you did not create.** Leave other
   people's and other agents' uncommitted work alone. In rac1/pal, follow
   the claims protocol when several agents work at once
   (`python3 tools/claims.py claim|release|lock|unlock`, described in its
   docs/AGENT_WORKFLOW.md).
9. **Regenerate generated files with their tools**; never hand-edit them
   (`progress/`, `shared/xmap/`, the marked tables in the READMEs, rac1/pal's
   `progress/report.json`, `nonmatching/README.md`, `include/names.h`). A file
   listed in `shared/files.json` is one file in several games: change every
   copy together (`tools/shared.py sync`) and verify each game that has it.
10. **Verify before you say it is done.** Run the checks for what you
    touched and report the results as they are, failures included.
11. **Do not settle open questions alone.** If a task depends on one
    ([docs/policy/OPEN_QUESTIONS.md](docs/policy/OPEN_QUESTIONS.md)), say so
    and ask.

## Commands

```sh
python3 tools/openrac.py discs              # identify and check the images in baserom/
python3 tools/openrac.py setup [GAME/VER]   # place each game's inputs (hard links, boot executables, toolchain links)
python3 tools/openrac.py progress [--fetch] # consolidate progress, refresh the README tables
python3 tools/openrac.py tables             # refresh the generated tables only
python3 tools/sources.py status|sync GAME/VER   # follow the original repositories while they are active
python3 tools/xmap.py scan|report|ports FROM TO # the function map across the games (docs/engine/SHARED_CODE.md)
python3 tools/shared.py check|find|sync PATH    # files several games share must stay identical (shared/README.md)
python3 -m unittest discover -s tools       # tests for tools/
python3 -m unittest discover -s editor      # tests for the editor
```

Per-game build and check commands are in each game's README and in
[docs/workflow](docs/workflow/README.md).

## Where new things go

See [docs/LAYOUT.md](docs/LAYOUT.md#where-a-new-file-goes). In short: game
work inside the game version's directory; knowledge that spans games in
`docs/engine`, `docs/toolchains` or `docs/workflow`; tools that serve several
games in `tools/`, with tests.
