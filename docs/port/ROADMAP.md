# The native port: order of work

What comes first, what can run in parallel, and what has to wait. It
follows from [DESIGN.md](DESIGN.md) and the studies beside it; numbers are
those of [DECOMP_STATUS.md](DECOMP_STATUS.md).

## Does the decompilation have to be finished first?

**To run the game natively, the code it runs has to be C: in practice, yes.**
The boot stage, the title loop, the level loop and the world draw are among
the 1,519 functions still in assembly; the renderer cores and the collision
code are hand-written assembly that no decompilation turns into C; no data
is C yet. The headline 59.32 % is 56.9 % C, and the C that exists is spread
over every subsystem rather than covering whole ones.

**To start the renderer, no.** The renderer is new code whichever way the
decompilation goes. The VU1 programs and the hand-written renderer cores
are not C in any matching decompilation, so understanding them is work of
its own; and the level data the renderer draws is already read by the
level editor. A native level viewer (extracted levels drawn on the GPU with
the console's conventions, no game code running) is the renderer's first
milestone and depends on nothing that is missing.

So the decompilation and the renderer run in parallel, and meet when the
frame path (the level loop, `DrawWorld`, the loaders) is C.

## Phases

| | Phase | Needs | Shows |
|---|---|---|---|
| P0 | Studies: this folder | | what stands between the decompilation and a port (**done**, 2026-10-09) |
| P1 | Disc setup, OpenGOAL's way: the extractor's extract and validate steps, and the launcher's install flow | | a player's disc checked and its files in place (**started**: [tools/extractor.py](../../tools/extractor.py)) |
| P2 | Assets: the extractor's decompile step, from the editor's readers to port-ready meshes and RGBA8 textures; the formats chosen (open decision 6) | P1 | a level, its models and textures in the port's formats |
| P3 | A native level viewer: terrain, ties, shrubs, static mobys and sky on the GPU, the console's conventions in shaders, a free camera; the graphics API chosen (open decision 2) | P2 | the first picture, with no game code |
| P4 | The renderer cores and VU1 programs read and documented, then drawn natively: ties, mobys, shrubs, terrain (level of detail, lighting), moby skinning and animation, sky, particles, the 2D path | P3, the player's disc for each program's disassembly | the viewer draws what the game draws, as the game draws it |
| P5 | The port's build of the game: the pointer model chosen (open decision 1), the header dialect, the data as C, the platform layer, C for the hand-written assembly | the decompilation of the boot, title and frame path | the game's code running natively: boot to the title screen |
| P6 | Game and renderer together: the hand-off at the sync points (open decision 3), the first level playable | P4, P5 | a native Ratchet & Clank |
| P7 | The sequels: Going Commando first (it shares the terrain and shrub programs and the frame's contract) | P6, their decompilations | |

**Throughout**, in the games' own projects: the decompilation continues
(`games/<game>/<version>`, under each project's own rules). For the port,
the functions that matter most are those on the frame path
([DECOMP_STATUS.md](DECOMP_STATUS.md), section 6), the fixes to the
decompiled C that the earlier compile passes found (the 14 files with
conflicting prototypes, the left-out functions:
[PORTABILITY.md](PORTABILITY.md), section 4), and C for the hand-written
assembly once its meaning is known.

## What can be taken now

- **P1**: the launcher's install flow on top of the extractor's extract and
  validate steps ([launcher/docs/INTEGRATION.md](../../launcher/docs/INTEGRATION.md)).
- **P2**: a converter on the editor's readers (`editor/level.py`,
  `terrain.py`, `ties.py`, `shrubs.py`, `moby_class.py`, `moby_anim.py`,
  `sky.py`), writing meshes and textures; textures need the CLUT and CSM1
  handling of [RENDERER.md](RENDERER.md), section 5.
- **P4 reading**: TieProc, MobyProc and ShrubProc's inputs (visibility,
  level of detail, light colours), the three lighting functions, then each
  VU1 program from a disassembly made locally from the player's own disc.
  Findings go into OpenRAC's docs; disassembly and microcode never do.
- **What is not read yet**: occlusion data, moby low-detail, metal and
  shadow packets, HUD banks and images.
