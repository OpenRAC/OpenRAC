# Profiling with Tracy

Tracy optionally profiles the native game and probe baker, independently of enhanced graphics
and F3. Ordinary builds neither fetch nor link it. The Python water baker is not instrumented.

## Build

From the repository root:

```sh
python tools/build_port.py rac1-ntsc --tracy --probe-baker
```

Direct CMake builds use `-DOPENRAC_TRACY=ON`. The client is pinned to Tracy 0.14.1,
commit `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c`; use matching profiler/capture tools.
For offline builds, set `-DFETCHCONTENT_SOURCE_DIR_TRACY=/path/to/tracy`.

`--tracy-system` enables OS tracing; `--tracy-sampling` adds sampled call stacks and requires
system tracing. Their CMake options are `OPENRAC_TRACY_SYSTEM` and `OPENRAC_TRACY_SAMPLING`,
both off by default. Windows may need administrator access; Linux may need tracing permissions.
Normal captures need neither option. OpenRAC retains its own crash handler.

Omitting Tracy flags from the build helper disables Tracy again; direct CMake builds use
`OPENRAC_TRACY=OFF`.

## Capture the game

Open the matching profiler, launch the game and connect. Zones record on demand while connected;
F3 and frame dumping can stay off. Game and Render timelines include simulation, scene/pose
capture, uploads, handoff, idle and completion waits. Simulation ticks and displayed pictures
have separate frame streams; picture markers measure API presentation, not physical scanout.

Vulkan interpolation jobs own their snapshots across simulation ticks, allowing simulation and
pose preparation to continue until graphics ownership is needed. The window thread presents
completed jobs, with one frame in flight. `Queue interpolated frame` marks submission.
OpenGL and synchronous Vulkan keep the window-thread path.

Vulkan selects its next slot after presentation but reclaims it at first graphics use.
`Prepare Vulkan frame slot` covers cleanup and command-buffer setup; `vulkan frame wait`
measures the remaining fence wait before reuse.

`stats::Scope` emits CPU zones. GPU scopes use separate query pools for Vulkan uploads and
graphics on the same queue, splitting scopes across submissions. Timestamp boundaries finish
deferred secondary passes, which can change grouping and add overhead. Collection does not
wait for each frame's timestamps. Initial GPU/CPU calibration waits during context creation
and gives approximate timing. OpenGL and devices without GPU timestamps retain CPU tracing.

Zones begin/end on their owning thread; snapshots never carry live zones. Worker overlap is
independent of F3's frame handoff. Compare captures using the same build and settings.

With a capture client waiting, `tracy_vulkan_test --capture` checks nested GPU scopes through
deferred draws, readback, resets and presentation with F3 off. Normal CTest also covers
disconnected startup and Vulkan validation.

## Capture probe baking

Start a capture client first. Short bakes can wait for it:

```sh
openrac-probe-bake --tracy-wait 30 --level-dir /path/to/level --output /new/path/sparse.orpg
```

`--tracy-wait` requires a Tracy build, accepts 0 to 120 seconds and fails if nothing connects.
Zones cover loading, scene export, Embree BVH construction, alignment, relocation, transport
and integration. Threads are named Probe worker; batches add per-volume zones and a Probe volumes
frame stream. Instrumentation preserves ray order/settings; Python planning is not traced.

Captures may contain source paths and application data. Keep captures and bakes out of Git.
See the upstream [manual](https://github.com/wolfpld/tracy/blob/30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c/manual/tracy.tex)
for profiler and capture tools.
