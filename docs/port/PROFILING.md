# Profiling with Tracy

Tracy is an optional development dependency for the native game and probe baker. It is independent
of enhanced graphics and the F3 profiler. Ordinary builds do not fetch or link it. The Python water
baker is not instrumented.

## Build

From the repository root:

```sh
python tools/build_port.py rac1-ntsc --tracy --probe-baker
```

For a direct CMake build, add `-DOPENRAC_TRACY=ON` when configuring the port. The client is pinned to
Tracy 0.14.1 at commit `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c`. Use a profiler or capture tool
built from that same revision. An offline checkout can be supplied with
`-DFETCHCONTENT_SOURCE_DIR_TRACY=/path/to/tracy`.

`--tracy-system` adds operating-system tracing; `--tracy-sampling` also adds sampled call stacks.
Their CMake equivalents are `OPENRAC_TRACY_SYSTEM` and `OPENRAC_TRACY_SAMPLING`; sampling requires
system tracing. Both are off by default. Access to system tracing depends on the OS: Windows may
need an administrator launch, and Linux may require changes to its tracing permissions. Normal
Tracy captures do not need these options. Tracy's crash handler is disabled so OpenRAC keeps its
own crash reporting.

Running the build helper without Tracy flags configures Tracy off again. For direct CMake builds,
set `OPENRAC_TRACY=OFF`.

## Capture the game

Start the matching Tracy profiler, then launch the game normally and connect to its process. The
client records zones on demand while a capture is connected. F3 and frame dumping can stay off.

The Game and Render threads show simulation, scene capture, pose work, uploads, render handoff,
worker idle time and completion waits. Simulation ticks and displayed pictures have separate frame
streams. A displayed-picture marker records presentation through the graphics API, not a measured
physical scanout time.

Vulkan interpolation jobs can cross a simulation tick boundary. They retain their owned snapshot
until completion; CPU simulation and pose preparation can continue before graphics ownership is
needed again. The window thread presents completed jobs, with at most one frame in flight.
The `Queue interpolated frame` zone identifies these submissions. OpenGL and synchronous Vulkan
retain their existing window-thread path.

Vulkan selects the next frame slot after presentation but waits for its fence only when graphics
work first needs that slot. `Prepare Vulkan frame slot` includes deferred cleanup and command-buffer
setup; its nested `vulkan frame wait` measures the remaining fence wait before resources are reused.

Existing `stats::Scope` sections also emit independent Tracy CPU zones. GPU-marked sections use
separate Vulkan query pools and appear on the Vulkan uploads and Vulkan graphics timelines.
Both streams run on the same queue. GPU scopes crossing submissions are split into recording segments.
Timestamp boundaries finish deferred secondary passes so GPU timing follows executed work. This can
change pass grouping and add profiling overhead while connected. Collection does not wait
for each frame's timestamps. The initial GPU/CPU clock calibration waits during Vulkan context
creation; it is approximate and does not promise exact physical presentation timing. OpenGL and
devices without graphics-queue timestamps retain CPU tracing.

Each zone starts and ends on the thread that owns it. Scene snapshots carry data, never a live
profiling zone. This keeps worker overlap visible without tying zone lifetimes to the F3 profiler's
frame handoff. Instrumentation has overhead: compare captures from the same build and settings.

For a connected GPU regression, run `tracy_vulkan_test --capture` with a capture client waiting.
It checks nested timestamp scopes through deferred draws, readback, command-buffer resets and
presentation while F3 profiling is off. A normal CTest run also checks disconnected startup and
Vulkan validation.

## Capture probe baking

Start a capture client before launching the native baker. For short jobs, wait for a connection:

```sh
openrac-probe-bake --tracy-wait 30 --level-dir /path/to/level --output /new/path/sparse.orpg
```

`--tracy-wait` accepts 0 to 120 seconds and fails if no capture connects. It requires a Tracy build.
The native tool shows asset loading, scene export, Embree BVH construction, grid alignment,
relocation, transport and probe-map integration. Worker threads are named Probe worker; batch jobs
also have per-volume zones and a Probe volumes frame stream. Ray order and bake settings are not
changed by instrumentation. The surrounding Python planning scripts are not traced.

Tracy captures can contain source paths and application data. Keep local captures and bake outputs
out of Git. The upstream [manual](https://github.com/wolfpld/tracy/blob/30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c/manual/tracy.tex)
describes the profiler and command-line capture tools.
