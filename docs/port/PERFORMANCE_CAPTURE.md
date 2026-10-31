# Diagnostics recordings

A diagnostics recording measures a short stretch of play. It produces a small report
that says where each frame's time goes. Attach it to a bug report or paste it into a
chat. In the code it is called a *performance capture*.

It works the same on desktop and Android. Nothing runs until you start one.

## How to record

| Where | How |
|---|---|
| Android | Drawer > Developer tools > **Record diagnostics**: pick a **Recording length**, then **Gameplay recording**. Recording starts 3 s after the drawer closes. Opening the drawer stops it. |
| Desktop | Developer menu > **Gameplay recording**, or **Ctrl+F3** to start and stop. |
| Script | `OPENRAC_PERF_CAPTURE=FRAME:SECONDS[:FILE]` starts one at that frame. Combine it with `OPENRAC_PRESS` and `--frames` to get repeatable runs. |

While it records, a red `REC` line shows at the top and the debug panel is hidden.
Paused frames and movies are not counted.

When it finishes, the report is **copied to the clipboard** and saved as
`perf_<game>_<date>-<time>.zip`, which holds the report (`.md`), every frame (`.jsonl`)
and snapshots of the game's state (`_effects.json`, below). Send the whole zip.

- Android: the **Diagnostics recordings** folder in the Files app.
- Desktop: `perf/` next to the game.

**Tips:** keep it short and repeatable. Stay in one place in one level, keep the same
settings and record for 30 s. Record the same scene before and after a change.

### With and without the performance hint (Android)

On Android 13 and later the game tells the system each frame's target time and how long
the work took (ADPF). The system then keeps the CPU fast instead of lowering its clocks
while the game waits for the display. To compare, record once as usual and once with
`OPENRAC_ADPF_OFF` set. The header's **performance hint** line says which one you have.

### Simulating a slower device

**Simulate slow CPU** (2x, 4x, 6x), in the same menus, makes the game's threads that many
times slower. The GPU is not slowed. It gives a rough idea, not an exact match for a real
weak phone. Works on Linux, Android and macOS.

### Timing each GPU pass on a phone

On Mali and Adreno the GPU time of a section inside a render pass reads about zero: the GPU
renders the whole pass later, tile by tile. A recording on such a GPU (Vulkan) therefore splits
the pass around each timed section in one drawn frame of ten, so that those sections' GPU times
are their own. Those frames give the per-section GPU times and are left out of the frame's GPU
time; the other nine give the frame's true GPU time. Nothing needs switching on. The report's
**gpu passes** line says so, and each split frame has `"passes_split":true` in the frames file.

### What each graphics setting costs (settings sweep)

Developer tools > **Record diagnostics** > **Graphics settings sweep** records 68 seconds and
changes the graphics settings every 4 seconds: Enhanced graphics off, every Enhanced effect off,
probe lighting off, render scale 50%, then each effect off on its own. VSync is off while it runs,
so a step's time is the device's own and not a multiple of the display's refresh, and split GPU
pass timing is off but in one step of its own. Stand still for the whole recording. The report
lists each step's time a picture (ticks the game runs without drawing are not counted) and GPU
time; the first 1.5 s of each step are left out, and each frame carries a `settings sweep step`
gauge. The settings are given back when it ends.

## What the report contains

- **Setup**: build, device, CPU and its clock speeds, thermal state (Android), GPU and
  driver, window size and settings. On Android, **performance hint** says whether the game used the hint (below).
- **Verdict**: whether frames hit their target and, if not, whether the CPU or the GPU is
  the bottleneck. It also says how much slower a CPU and GPU could be and still run at
  60 fps.
- **Frame times**: average, median, 90th and 99th percentile and worst, for the frame, the
  game code and the GPU. It also shows the *work* of each thread, without time spent
  waiting. On a weaker device only the work counts.
- **Sections**: every timed part of the frame, with CPU time, GPU time, draw calls and
  triangles.
- **Counters and gauges**: draws, uploads, binds and renderer stats.
- **Timeline**: one row per second. A frame rate that drops while the work stays the same
  means the device is overheating.
- **Worst frames**: the 8 slowest frames and what they spent time on.
- **Hot functions** (Linux and Android): the 30 functions each thread spends the most time
  in, found by sampling.

## Snapshots of the game's state

`_effects.json` holds a snapshot every 20 frames (the last 400), to help find a picture or
gameplay bug. Each one lists:

- the effect strips and quads drawn within 6 units of Ratchet (GS state, bounds, colours,
  distance);
- the particles by type, with the update each type runs, and the full records of those near
  Ratchet;
- the game's own notes. RAC1: the moby table's free slots and spawned classes, and the
  collision queries that started within 3 units of Ratchet in the last tick, with their result.

## What is measured, and where

| | Desktop | Android |
|---|---|---|
| CPU time per section | yes | yes |
| GPU time (OpenGL) | yes | Adreno, most Mali |
| GPU time (Vulkan) | yes | yes |
| Counters | yes | yes |
| CPU clock speeds | Linux | if the system allows it |
| Thermal state | no | Android 11+ |

Measuring costs well under 1 ms per frame. On phone GPUs (Adreno, Mali) the per-pass GPU
times are a little high. The whole-frame times and the verdict are accurate.

## Going deeper

The report tells you *which* part is slow. To look inside it:

- **Android GPU Inspector**, **Perfetto**, **Snapdragon Profiler** (Adreno),
  **Arm Performance Studio** (Mali)
- **RenderDoc** for a single frame. A build with `OPENRAC_GPU_MARKERS=ON` labels the passes
  ([DEBUGGING.md](DEBUGGING.md)).

## Code

- [common/perf_capture.h](../../port/common/perf_capture.h): builds the report (tested by
  `perf_capture`).
- [game/common/debug/capture.cpp](../../port/game/common/debug/capture.cpp): starting,
  saving and copying.
- [common/sampler.h](../../port/common/sampler.h): hot functions.
- [common/slow_cpu.h](../../port/common/slow_cpu.h): slow CPU simulation.
- [game/common/frontend/effects_dump.h](../../port/game/common/frontend/effects_dump.h): the
  snapshots; the game's notes come from `GameProfile::diagnostics` (RAC1:
  [game/rac1/profile.cpp](../../port/game/rac1/profile.cpp),
  [host/collision_notes.h](../../port/game/rac1/host/collision_notes.h)).
