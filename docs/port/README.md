# The native port

OpenRAC plays the games as a **native PC port**, built the way
[OpenGOAL](https://github.com/open-goal/jak-project) ported Jak and Daxter:

- the game's own code, decompiled to C, compiled for the PC and run as a
  normal program;
- OpenRAC's own renderer, which draws what the game asks for on a modern GPU,
  one renderer per kind of thing (terrain, ties, shrubs, mobys, sky,
  particles, 2D);
- the game's assets extracted once from the player's own disc.

**No emulation.** OpenRAC does not interpret the console's processors, run
the retail program, or model the GS, the vector units or the DMA controller.
An early emulation experiment was removed in October 2026 (git history keeps
it); the studies it produced are in this folder.

Nothing from the games is ever committed: no code bytes, data, microcode or
media from a disc ([SOURCING.md](../policy/SOURCING.md)).

The port starts with Ratchet & Clank ([games/rac1/decomp](../../games/rac1/decomp)), whose
decompilation is furthest along. The sequels follow, sharing the renderer
where their engines agree ([docs/engine](../engine/README.md)). The code is
in [port/](../../port/README.md).

## Start here

| Document | For |
|---|---|
| [WORKFLOW.md](WORKFLOW.md) | **To help:** from a player's report to a release: reproduce it by script, find the cause, fix it, check it |
| [DESIGN.md](DESIGN.md) | What is decided, the shape of the port, open decisions |
| [ROADMAP.md](ROADMAP.md) | The phases of the port and where it stands |
| [KNOWN_ISSUES.md](KNOWN_ISSUES.md) | Bugs that are understood and guarded but not properly fixed yet |

## Features

| Document | For |
|---|---|
| [SETTINGS.md](SETTINGS.md) | The settings file, frame pacing against the game's tick, graphics card checks, game fixes a player can switch off |
| [CONTROLS.md](CONTROLS.md) | The controls files, bindings, sticks, triggers, players |
| [CONTROLLERS.md](CONTROLLERS.md) | Controllers: what the game asks of a pad, DualSense features in SDL3, what to build |
| [MODS.md](MODS.md) | Native and script mods, their manifest, hook points, purist mode, the [SDK](../../sdk/README.md) |
| [TEXTURE_PACKS.md](TEXTURE_PACKS.md) | Replacing the game's textures with your own pictures |
| [HDR.md](HDR.md) | HDR output |
| [SPEEDRUN.md](SPEEDRUN.md) | Timing, LiveSplit, practice tools, save states, the game's speedrun meta |
| [ANDROID.md](ANDROID.md) | The port and the launcher on Android; building and device tests in [port/android](../../port/android/README.md) |

## Debugging and testing

| Document | For |
|---|---|
| [DEBUGGING.md](DEBUGGING.md) | The debug overlay, frame dumps, pause and slow motion, every `OPENRAC_*` variable |
| [RAC1_PAL_TESTING.md](RAC1_PAL_TESTING.md) | Playtesting by script: start any level, teleport, press buttons, take screenshots, read memory |
| [REGRESSION.md](REGRESSION.md) | Regression runs: every level played by route, traces compared with another build |
| [RAC1_PAL_BUGFIX_GUIDE.md](RAC1_PAL_BUGFIX_GUIDE.md) | Building, development switches, finding a gameplay bug's cause, supplying C for a function |
| [RAC1_NTSC_HINTS.md](RAC1_NTSC_HINTS.md) | Where the NTSC-U game's own code troubles a native build, and what the port does |
| [WEAPONS.md](WEAPONS.md) | Every weapon and how to test it (`GiveItem`, vendors, an `OPENRAC_WRITE` recipe) |
| [BACKFILL.md](BACKFILL.md) | Functions with no matching C yet: which to fill in, and the fcheck gate that checks them |
| [DECOMP_STATUS.md](DECOMP_STATUS.md) | How much of Ratchet & Clank (PAL) is C a PC can build |
| [RAC2_IDENTIFIED.tsv](RAC2_IDENTIFIED.tsv) | Going Commando: functions and data identified during bring-up, for naming later |

## Rendering and performance

| Document | For |
|---|---|
| [RENDERER.md](RENDERER.md) | How the game builds a frame, what it draws, the graphics chip's conventions |
| [VU_PROGRAMS.md](VU_PROGRAMS.md) | The games' vector unit programs, and which they share |
| [RAC1_GPU_DRIVEN.md](RAC1_GPU_DRIVEN.md) | Vulkan visibility, compaction, indirect batching, resource lifetime |
| [RENDER_PERF.md](RENDER_PERF.md) | What drawing costs: counters, timed passes, the optimisation registry |
| [PERFORMANCE_CAPTURE.md](PERFORMANCE_CAPTURE.md) | Recording a stretch of play into one short report |
| [PROFILING.md](PROFILING.md) | Optional Tracy CPU and Vulkan GPU captures |
| [RAC1_INTERIOR_INFERENCE.md](RAC1_INTERIOR_INFERENCE.md) | Offline interior lighting for probe bakes |
| [Enhanced rendering docs](../../port/enhanced/README.md) | [Probe GI](../../port/enhanced/docs/RAC1_PROBE_GI.md), [hero shadow](../../port/enhanced/docs/RAC1_HERO_SHADOW.md), [sun shadows](../../port/enhanced/docs/RAC1_SUN_SHADOW.md), [water](../../port/enhanced/docs/water.md), [atmosphere](../../port/enhanced/docs/atmosphere.md) |

## Background and research

| Document | For |
|---|---|
| [HARDWARE_SEMANTICS.md](HARDWARE_SEMANTICS.md) | What the code relies on in the PS2's processors (rounding, no NaN, 128-bit registers, ...) and how the port keeps each |
| [PORTABILITY.md](PORTABILITY.md) | What the matching C assumes about the console, and the choices for a PC build |
| [OPENGOAL_NOTES.md](OPENGOAL_NOTES.md) | How OpenGOAL's renderer and platform layer work, and what carries over |
| [RAC1_PAL_SURVEY.md](RAC1_PAL_SURVEY.md) | The first survey of the game for a native build (2026-10-08) |
| [RAC1_PAL_CAMPAIGN.md](RAC1_PAL_CAMPAIGN.md) | The rules and dated log of the campaign for a playable Ratchet & Clank (PAL) |
| [tools/extractor.py](../../tools/extractor.py) | The disc extractor (OpenGOAL's `extractor`, for OpenRAC) |
