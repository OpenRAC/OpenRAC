# Performance captures

A performance capture records a stretch of play, measured as the F3 overlay
measures it ([DEBUGGING.md](DEBUGGING.md), [RENDER_PERF.md](RENDER_PERF.md)),
and sums it up into one short text report: the device, the GPU and its
driver, the settings, the frame times, a verdict, where each frame's time
went on the CPU and on the GPU, the counters, the time line and the worst
frames. The report is a few kilobytes of Markdown, made to be pasted into an
issue or a chat whole; the frames themselves are written beside it as a
frame dump (`.jsonl`) for a closer look.

It works the same on a desktop and on Android. Nothing of it runs until a
capture is started.

## Recording one

| Where | How |
|---|---|
| Android (the drawer) | Developer tools, "Capture length" (15, 30, 60, 120 s or until stopped), then "Record performance". The drawer shuts and recording starts three seconds later; a red `REC 12 / 30 s` line at the top of the picture counts. Opening the drawer again ends it early. Everything is reachable with a gamepad. |
| Windows and Linux (the menu bar) | Developer, "Record performance" (Ctrl+F3), and "Recording length". |
| Any system | Ctrl+F3 starts and stops a capture; the overlay's panel has a "record performance" button. |
| By script | `OPENRAC_PERF_CAPTURE=FRAME:SECONDS[:FILE]` starts one at that window frame, for that many seconds, with the report to `FILE`. With `OPENRAC_PRESS` and `--frames` a scenario runs the same way each time. |

While it records, the overlay's panel is hidden (it costs frame time of its
own) and comes back afterwards. Paused frames and the game's own full-screen
pictures (movies) are left out of the numbers.

When it ends, the report is **copied to the clipboard** and saved:

- Android: the "Performance captures" folder of OpenRAC in the system's Files
  app (the app's `files/perf`).
- Desktop: `perf/` in the folder the game runs in.

The file is `perf_<game>_<date>-<time>.md`, the frames `…​.jsonl` beside it.

A scenario worth recording is short and repeatable: one place of one level
(standing at the start of Kerwan, running through the first area of Orxon, a
fight), the same settings, 30 seconds. Record the same scenario before and
after a change, and on each device.

## What the report holds

- **The run**: build and commit, the system and device, the processor and its
  clocks at the start and the end (a phone that runs below its top clock at
  the end is throttling), Android's thermal state and headroom, the graphics
  API, GPU, driver version and whether GPU timers exist, the window and
  frame buffer sizes, the pacing and the picture's settings.
- **Verdict**: whether the frame keeps its target (the game's tick, 20 ms in
  PAL, or the display's interval when frames are drawn between ticks); if not,
  whether the GPU or the CPU holds it up, and the costliest part on each.
- **Frame times**: average, median, 90th and 99th percentile and worst of the
  frame, the game's code and the GPU.
- **Sections**: every timed part of the frame in its tree, CPU time per
  frame with its 95th percentile and worst, GPU time, draw calls and
  triangles.
- **Counters and gauges**: draw calls, triangles, texture and buffer uploads,
  program and framebuffer binds; the gauges the renderers publish (mobys,
  the Vulkan backend's `vk_*`), each as average and maximum.
- **Time line**: one row a second (more in a long capture): frames a second,
  the average and worst frame, the game's time, the GPU's. A falling frame
  rate with steady work is heat.
- **Worst frames**: the eight longest, each with its three costliest
  sections and its uploads.

The code is [common/perf_capture.h](../../port/common/perf_capture.h) (the
summary, no window or GPU, tested by `perf_capture`) and
[game/common/debug/capture.cpp](../../port/game/common/debug/capture.cpp)
(starting, the badge, the device's details, the files and the clipboard).
The capture is a reader of stats' closed frames (`stats::set_reader`), which
get them in order once their GPU times are back, as the frame dump does.

## What is measured where, and what it costs

| Source | Desktop | Android |
|---|---|---|
| CPU sections (`stats::Scope`, steady clock) | yes | yes |
| GPU time on OpenGL: `GL_TIME_ELAPSED` queries, read a few frames late | core since 3.3 | `GL_EXT_disjoint_timer_query`: Adreno yes; Mali on most current drivers |
| GPU time on Vulkan: `vkCmdWriteTimestamp` | where the queue has `timestampValidBits` | the same (Adreno, Mali, Turnip) |
| Counters (draws, triangles, uploads, binds; `vk_*`) | always counted | always counted |
| Processor clocks | Linux (`/sys/devices/system/cpu`) | where the kernel lets apps read them |
| Thermal state and headroom | no | Android 11 and 12 on (`AThermal_*`) |

Measuring costs a few `steady_clock` reads and one GPU query pair per timed
pass: well under a millisecond a frame on a phone. Timer queries on tiled
GPUs (Adreno, Mali) can split a render pass where one ends, so a pass's GPU
time is a good guide to its share and a slight overestimate of its cost; the
frame times and the verdict are measured from the CPU side and are not.

## Other profilers, for a closer look

The capture says which part is slow. To see inside one part:

- **Android GPU Inspector** and **Perfetto** (system traces: threads, CPU
  clocks, the GPU's counters on Adreno and Mali), **Snapdragon Profiler**
  (Adreno), **Arm Performance Studio / Streamline** (Mali).
- **RenderDoc** (one frame, every call; desktop and Android, OpenGL ES and
  Vulkan) and the Vulkan labels of an `OPENRAC_GPU_MARKERS=ON` build in AMD
  RGP and others ([DEBUGGING.md](DEBUGGING.md)).
- **Tracy** would take the same `stats::Scope` sections if the port ever
  needs a live timeline; it is not part of the build.
