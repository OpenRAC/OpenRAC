# The native port: order of work

The phases of the port, which of them are done, and where it stands. The
plan came from [DESIGN.md](DESIGN.md) and the studies beside it.

## Phases

| | Phase | Needs | Shows |
|---|---|---|---|
| P0 | Studies: this folder | | what stands between the decompilation and a port (**done**, 2026-10-09) |
| P1 | Disc setup, OpenGOAL's way: the extractor's extract and validate steps, and the launcher's install flow | | a player's disc checked and its files in place (**done**: [tools/extractor.py](../../tools/extractor.py), the launcher) |
| P2 | Assets: the extractor's decompile step, from the editor's readers to port-ready meshes and RGBA8 textures; the formats chosen (open decision 6) | P1 | a level, its models and textures in the port's formats |
| P3 | A native level viewer: terrain, ties, shrubs, static mobys and sky on the GPU, the console's conventions in shaders, a free camera; the graphics API chosen (open decision 2) | P2 | the first picture, with no game code |
| P4 | The renderer cores and VU1 programs read and documented, then drawn natively: ties, mobys, shrubs, terrain (level of detail, lighting), moby skinning and animation, sky, particles, the 2D path | P3, the player's disc for each program's disassembly | the viewer draws what the game draws, as the game draws it |
| P5 | The port's build of the game: game memory and hostgen (pointer model: OpenGOAL's offsets), the library replacements, C for the hand-written assembly | the decompilation of the boot, title and frame path | the game's code running natively (**done** for Ratchet & Clank) |
| P6 | Game and renderer together: the hand-off at the sync points (open decision 3), the first level playable | P4, P5 | a native Ratchet & Clank (**done**: both discs play from start to end) |
| P7 | The sequels: Going Commando first (it shares the terrain and shrub programs and the frame's contract) | P6, their decompilations | (**started**: Going Commando reaches its main menu with a further-along decompilation) |

## Where the port stands

Ratchet & Clank plays from start to end on both discs (`openrac-rac1-pal`,
`openrac-rac1-ntsc`). Functions still in assembly in the decompilation run
as the port's own C; hostgen's report lists what is left. Known bugs are in
[KNOWN_ISSUES.md](KNOWN_ISSUES.md).

The sequels build the same way, each from its decompilation
(`openrac-rac2-ntsc`, `-rac3-ntsc`, `-rac3-pal`, `-rac4-ntsc`). What they still need
before they run is in [port/game](../../port/game/README.md): their `main`
and their library tables.

The decompilation itself goes on in the games' own projects
(`games/<game>/<version>`, under each project's rules).
