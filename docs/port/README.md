# The native port

OpenRAC's goal for playing the games is a **native PC port**, built the way
[OpenGOAL](https://github.com/open-goal/jak-project) ported Jak and Daxter:

- the game's own code, decompiled to C, compiled for the PC and run as a
  normal program;
- a renderer of OpenRAC's own that draws what the game asks for on a modern
  GPU, written per renderer (terrain, ties, shrubs, mobys, sky, particles,
  2D), not by modelling the PlayStation 2's graphics hardware;
- the game's assets (levels, textures, models, sound) extracted once from the
  player's own disc into formats the port reads.

**No emulation.** OpenRAC does not interpret the console's processors, run the
retail program, or model the GS, the vector units or the DMA controller to
play a game. An earlier experiment that did, `runtime/` (2026-10-08 to
2026-10-09), is being taken out of the repository; git history keeps it, and
the studies it produced about the games move to this folder.

Nothing from the games is ever committed: no code bytes, data, microcode or
media taken from a disc ([SOURCING.md](../policy/SOURCING.md)). The player
supplies their disc; the port reads what was extracted from it.

## Where it starts

With Ratchet & Clank (PAL), [games/rac1/pal](../../games/rac1/pal), whose
matching decompilation is the furthest along. The sequels follow, sharing the
renderer where their engines agree ([docs/engine](../engine/README.md)).
