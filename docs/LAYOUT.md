# How the repository is organised

```
OpenRAC/
├── README.md, CONTRIBUTING.md, AGENTS.md, CLAUDE.md
├── LICENSE.md, THIRD_PARTY_NOTICES.md, CREDITS.md
├── games/                  one directory per game, one per version inside it
│   ├── rac1/               Ratchet & Clank (2002)
│   │   ├── game.json       discs, checksums, what is decompiled, where it came from
│   │   ├── pal/            SCES_509.16, from rac1-decomp
│   │   └── ntsc/           SCUS_971.99, from Lombyte
│   ├── rac2/ntsc/          Going Commando, SCUS_972.68 v1.01, from rac2-decomp
│   ├── rac3/ntsc/          Up Your Arsenal, SCUS_973.53, from ratchet-uya-decomp
│   └── rac4/ntsc/          Deadlocked, SCUS_974.65, from rac-deadlocked-decomp
├── editor/                 Godot level editor and extractor (shared)
├── tools/                  repository-wide tools: openrac.py, sources.py
├── docs/                   knowledge and rules that span the games
│   ├── policy/             sourcing policy, open questions
│   ├── engine/             what is known about the engine, across games
│   ├── toolchains/         which compilers built what
│   └── workflow/           how matching works in each game
├── progress/               consolidated progress (generated)
├── shared/                 what the games have in common: the function map, the shared-files list
├── baserom/                your own disc images (ignored, except its README)
└── toolchains/             compilers you supply (ignored, except its README)
```

## The rules that keep it organised

**A game version is a self-contained project.** `games/<game>/<version>/`
has its own README, license, build, tools and docs, and you work on it from
inside that directory: its paths are relative to it, and its build never
reaches into another game. Its own docs are the authority on how a match is
proven there.

**Shared things live at the top level, and only shared things.** A tool,
document or component goes to the top level when it serves more than one
game, or the repository itself: the editor, `tools/openrac.py`, the policies,
the cross-game references. A game may use top-level components (rac1/pal
uses the editor's readers); the top level never depends on one game's
internals, apart from the per-game knowledge the editor decodes.

**Generated and personal files stay out of git.** Disc images, extracted
data, assembly, build output, the generated Godot project and toolchains
are ignored, at the top level and in each game's own `.gitignore`.
`progress/` and the tables in `README.md` are generated but committed: they
are what readers see.

## Where a new file goes

| You are adding | Put it in |
|---|---|
| Decompiled code, a symbol map, a game's build change | `games/<game>/<version>/`, following that project's layout |
| A note about one game | that game's `docs/` |
| Knowledge that holds across games (a format, a subsystem, a compiler) | `docs/engine/`, `docs/toolchains/` or `docs/workflow/`, linking the per-game evidence |
| A tool that serves several games or the repository | `tools/` (standard library Python where possible), with tests |
| A file a second game needs unchanged (a library source, a build helper) | a copy in that game, listed with the original in `shared/files.json` ([shared/](../shared/README.md)) |
| Editor code | `editor/` |
| A decision or rule for everyone | `docs/policy/`, and `AGENTS.md` and `CONTRIBUTING.md` if it changes how people work |
| A new game version | `games/<game>/<version>/`, plus its entry in `games/<game>/game.json` |

## Names

- Games are `rac1`, `rac2`, `rac3` and `rac4` in release order. `rac4` is
  Ratchet: Deadlocked (Ratchet: Gladiator in Europe).
- Versions are named by region: `pal` (Europe), `ntsc` (North America,
  NTSC-U). A Japanese release would be `ntsc-j`. The serial and disc version
  are in `game.json`.
- Commit scopes use the same names: `rac1/pal`, `rac1/ntsc`, `rac2`, `rac3`,
  `rac4`, and `editor`, `tools`, `docs`, `progress` for the top level
  ([CONTRIBUTING.md](../CONTRIBUTING.md#commits)).

## Where this is heading

The layout keeps each project buildable as it was. Later steps the group has
discussed, in [OPEN_QUESTIONS.md](policy/OPEN_QUESTIONS.md): one RAC1 tree
that builds both regions, a shared tree for code that is identical across
games, and one build container for every game.
