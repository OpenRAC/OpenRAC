# Where the code comes from

OpenRAC brings four decompilation projects together. Each was imported as a
snapshot of its default branch on 2026-10-03, when none had open pull
requests. A snapshot is the project's tree at one commit, taken with
`git archive`, not its git history. The histories hold material the projects
later removed, so they stay in the original repositories, which remain the
record of who wrote what. [CREDITS.md](../CREDITS.md) lists the people behind
each project.

| Directory | Project | Repository | Branch | Commit | License |
|---|---|---|---|---|---|
| [`games/rac1/pal`](../games/rac1/pal) | rac1-decomp | https://github.com/Lynder063/rac1-decomp | main | `e67fe2010ed1` | MIT |
| [`games/rac1/ntsc`](../games/rac1/ntsc) | Lombyte | https://github.com/mateuszklysz/Lombyte | main | `64543183305b` | MIT (+ GPL-2.0, newlib) |
| [`games/rac2/ntsc`](../games/rac2/ntsc) | rac2-decomp | https://github.com/llesieur99/rac2-decomp | RAC2 | `0fd4b7755381` | MIT |
| [`games/rac3/ntsc`](../games/rac3/ntsc) | ratchet-uya-decomp | https://github.com/vetusmagnus/ratchet-uya-decomp | main | `dcc2c1f28414` | none stated |

The full commit hashes are in each `games/<game>/game.json` under `source`,
which [tools/sources.py](../tools/sources.py) reads and updates.

## What changed on import

Everything else is exactly as it was at that commit.

**rac1/pal**
- Left out `.agents/mcp_config.json`, a machine-specific MCP configuration.
- Moved `tools/extract/` (the level extractor) and
  `docs/GDSCRIPT_CONVENTIONS.md` to the top-level [`editor/`](../editor/README.md),
  and pointed `tools/overlays.py`, `tools/overlay_scan.py`, `tools/dossier.py`
  and the docs at it.
- `tools/docker/run.sh` mounts all of OpenRAC in the build container, so the
  editor, `baserom/` and `toolchains/` are reachable.
- `tools/wave.py` names its own directory in worker prompts instead of a
  path on one machine.

**rac1/ntsc**
- The README credits the NTSC decomp by that name only
  ([removal requests](policy/SOURCING.md#removal-requests)).

**rac2/ntsc**
- Left out `include/moby.h`. Its structure was taken from leaked material
  ([SOURCING.md](policy/SOURCING.md)); `include/engine.h`, which no C file
  includes, now forward-declares `struct Moby` instead.
- `docs/COMMUNITY-ENGINE-REFERENCE.md` loses its moby field list, and the
  table row and note taken from a leaked port's source path (same reason),
  and names no community archive.
- `docs/PROTOTYPE-BUILDS.md` loses one sentence about another game's
  prototype.
- `CONTRIBUTING.md` points to the sourcing policy: leaked material is never
  used.

**rac3/ntsc**
- Left out `localdecomp/server.py.bak_0926`, a stale backup of `server.py`.

## Bringing in later work

While the projects are still active in their own repositories, their new
commits can be applied here as a diff, so the changes above survive:

```sh
python3 tools/sources.py status            # how far each checkout is past its recorded commit
python3 tools/sources.py sync rac1/ntsc    # apply the checkout's new commits to games/rac1/ntsc
```

A checkout is looked for at the path in `game.json` (for example
`~/Projects/rac1-decomp-ntsc`), or in `$OPENRAC_SOURCE_RAC1_NTSC`. A hunk
that touches a line changed here is left as a `.rej` file to resolve by hand.
Before committing a sync, check it against the
[sourcing policy](policy/SOURCING.md), including the removal requests, and
commit it as one `chore(import)` commit per project, naming the new commit.

## Other projects OpenRAC builds on

These are credited where their work is used, and in
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md):

- [ReRAC](https://github.com/re-rac/rerac) (ISC): the moby, collision and
  animation formats in the editor, and function notes in rac1/pal.
- [Wrench](https://github.com/chaoticgd/wrench) (GPL-3.0-or-later): asset
  formats, read for the editor; the tool rac2 and rac3 use to unpack discs.
- splat, spimdisasm, objdiff, asm-differ, decomp-permuter, m2c and wibo:
  the decompilation tooling the projects use; none is vendored here.
