# Profiling with Tracy

Tracy optionally profiles the native game and probe baker, independently of enhanced graphics
and F3. Ordinary builds neither fetch nor link it. Native water rasterisation, distance fields,
hydraulics and rest-mesh generation are instrumented; source decoding is instrumented too.

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

The launcher runs the native `--bake-all` pipeline directly. `Automatic bake pipeline`,
`Automatic level pipeline`, `Compute native cache identity` and `Install verified lighting`
cover orchestration and publication. `Decode water source tables`, water calculations,
interior generation, planning, GI and reflections have their own native zones.


Start a capture client first. Short bakes can wait for it:

```sh
openrac-enhanced-baker --tracy-wait 30 --level-dir /path/to/level --output /new/path/sparse.orpg
```

`--tracy-wait` requires a Tracy build, accepts 0 to 120 seconds and fails if nothing connects.
Zones cover loading, scene export, Embree BVH construction, alignment, relocation, transport
and integration. Bake pool worker threads expose idle and task scopes; level controllers expose
`Prepare region plan`, including any wait for its request. Reflection captures separate face tracing from GGX filtering. Batches add
per-volume zones and a Probe volumes frame stream. Instrumentation preserves ray order/settings;
Automatic region planning has `Plan probe regions`, `Seed probe regions`,
`Expand probe region coverage` and `Partition probe regions` zones. Source decoding runs
on the native level controller.
`Assemble final bake plan` covers native report assembly and resume checks;
`Register verified bake outputs` covers final asset checks and registry publication.
Emission stages add `Prepare emission profile`, `Resolve emission catalog` and
`Audit emission surfaces` zones. The automatic flow refreshes material profiles with a short
native command before cache checks; the mesh audit runs inside the persistent batch after loading.

Water stages use `--bake-water INPUT.json OUTPUT.bin [--tracy-wait SECONDS]`. Zones separate
`Load water collision`, `Rasterise waterline`, `Water shore distances`, `Water hydraulics`,
`Write water fields` and `Water rest mesh`. These run in the native baker before GI preparation.

Captures may contain source paths and application data. Keep captures and bakes out of Git.
See the upstream [manual](https://github.com/wolfpld/tracy/blob/30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c/manual/tracy.tex)
for profiler and capture tools.

Offline alignment exposes `Align probe grid`, `Preview alignment phases` and worker-side
`Score alignment samples` zones. Phase reductions and grid writes remain on the controller.

Offline bake captures also separate `Prepare batch grids`, `Build density receivers`,
`Simplify probe density` and `Verify adaptive GI field`. Reference-field sampling, candidate
comparisons and GI indexing are nested below field validation. Probe encoding, GPU decoding,
CPU expansion, atlas packing, file writing and readback have separate zones; their inclusive
times overlap with parent stages and must not be added together.

`Sample GI field batch` measures each bounded query submission; worker-side
`Sample GI field receivers` zones show parallel sampling CPU time. Candidate error/rejection
reduction stays on the controller. Ordinary combined GI/reflection bakes emit one
`Read playable floors` zone because both stages share the prepared floor graph.

`Bake output worker` and `Write and verify probe output` show queued local-grid I/O and readback.
`Wait for bake output` includes ordered retirement and any controller wait for queue capacity.
These tasks share tracing workers; their inclusive times overlap grid baking and cannot be added
to controller times to estimate wall time.

`Read and compare probe bytes` isolates file I/O and exact byte comparison beneath readback.
`Read probe header` covers the shared format contract; `Read probe planes` validates packed
channels and optionally expands CPU maps in the same pass. `Read adaptive GI asset` counts only
candidate volumes and separate fine references. `Read reflection GI field` appears only when a
completed density field was unavailable for reuse.
