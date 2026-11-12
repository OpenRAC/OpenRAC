# renderer/vulkan

The port's Vulkan renderer: the short list of OpenGL calls every renderer is written against
(`renderer/gl/gl.h`, `renderer/world/gl_world.h`) carried out with Vulkan 1.3, so that the renderers,
their shaders and the frontend run on it unchanged. It is chosen at start with
`OPENRAC_GL_API=vulkan` (`platform/window.cpp`), and it is what draws on a player's own Vulkan
driver on Android. It is not a general OpenGL implementation: a GL call the renderers do not make is
not here, and a new renderer feature has to be expressible in that list. How GL's model becomes
Vulkan's is the comment at the top of [vkgl.h](vkgl.h).

## Interface

- [vkgl.h](vkgl.h): `openrac::vkgl`, the backend as the window and the frontend see it: `open` and
  `close`, `get_proc` (the GL functions by name, for `renderer/gl/gl.h`'s table), `present`, the swap
  interval, the GPU-driven instance calls, the presents' timing (`present_timing`,
  `wait_for_present`) and the counters the debug tools read.
- [pipelines/shader_compiler.h](pipelines/shader_compiler.h): GLSL made into SPIR-V with glslang, with what an OpenGL
  program would answer about its uniforms and blocks.

The implementation is split by responsibility:

- `graph/` owns Vulkan graph imports, execution, validation and prepared pass recording.
- `commands/` lowers native commands and submissions; `context/` owns device setup and GL entry points.
- `pipelines/`, `resources/` and `draw/` handle compilation, allocations and draw recording.
- `presentation/` handles the swapchain, HDR and frame pacing; `passes/` holds native compute passes.
- `cache/` and `diagnostics/` contain persistent caches, frame capture and profiling.

The public entry points and shared backend declarations remain beside this README.

| File | What it holds |
|---|---|
| [graph/graph_execution.cpp](graph/graph_execution.cpp), [graph/graph_imports.cpp](graph/graph_imports.cpp) | Ordered plan execution and canonical allocation imports |
| [graph/graph_immediate_work.cpp](graph/graph_immediate_work.cpp), [graph/graph_image_work.cpp](graph/graph_image_work.cpp) | Compatibility draws, dispatches, clears and transfers captured as declared graph work |
| [graph/graph_pipeline.cpp](graph/graph_pipeline.cpp) | Active pass contracts, binding validation and pipeline command lowering |
| [graph/graph_state_translation.cpp](graph/graph_state_translation.cpp) | Context-independent stage, access and barrier translation shared by frame rendering and the baker |
| [graph/graph_states.cpp](graph/graph_states.cpp), [graph/graph_buffer_execution.cpp](graph/graph_buffer_execution.cpp) | Frame-owned transitions and buffer-only plans on independent devices |
| [commands/graph_queue.cpp](commands/graph_queue.cpp), [commands/native_command_encoder.cpp](commands/native_command_encoder.cpp) | Native queue submission and command recording boundaries |
| [vkgl_internal.h](vkgl_internal.h) | The types, constants and state (`Context`, the one `g`) the files share, and the functions each file offers the others, grouped by the file that defines them |
| [vkgl.cpp](vkgl.cpp) | `g`, `open`, `close`, `present` and the rest of `vkgl.h` |
| [commands/vkgl_commands.cpp](commands/vkgl_commands.cpp) | Work deferred to the frame's end, the frame's ring, the two command buffers (`upload`, `main`, and `present` for the copy to the screen under `vk-tight-sync`) and the order between them (each barrier's stages: RENDER_PERF.md, "R-08"), GPU timing, submission, render targets, the passes (under `attachment-discard` recorded into a secondary buffer, so that their load and store operations follow `glInvalidateFramebuffer`) and `glClear` |
| [resources/vkgl_resources.cpp](resources/vkgl_resources.cpp) | Images, views, pixel uploads and mip chains, samplers, buffers on the GPU and their texel views |
| [pipelines/vkgl_pipelines.cpp](pipelines/vkgl_pipelines.cpp) | Vertex formats, fixed-function state, pipelines by key, descriptor sets, `glDispatchCompute` and `glMemoryBarrier` |
| [draw/vkgl_draw.cpp](draw/vkgl_draw.cpp) | One draw recorded with the current state; includes `draw/gpu_draw.inc` and `draw/gpu_materials.inc` |
| [draw/gpu_materials.inc](draw/gpu_materials.inc) | The native material path: material records and textures indexed by the draw |
| [pipelines/gl_program.cpp](pipelines/gl_program.cpp), [pipelines/gl_program.h](pipelines/gl_program.h) | Private GL shader/program entry points, linking and reflected uniform writes |
| [draw/multisample_blit.inc](draw/multisample_blit.inc) | Single-sample colour copied into MSAA targets by a fullscreen draw, with caller state restored |
| [draw/gpu_draw.inc](draw/gpu_draw.inc) | GPU-driven instances: culling on the GPU and indirect draws |
| [context/vkgl_gl.cpp](context/vkgl_gl.cpp) | The GL functions themselves (state, textures, buffers, vertex arrays, samplers, framebuffers) and their table |
| [presentation/vkgl_present.cpp](presentation/vkgl_present.cpp) | The default framebuffer, the swapchain and its present mode, moving on to the next frame, presenting (the picture's waiting copy composed straight into the screen under `present-single-copy`) |
| [context/vkgl_device.cpp](context/vkgl_device.cpp) | The instance and the validation layer, the physical and logical device, the pipeline cache on disk, the frames' objects, the dummy resources, the surface, and teardown |
| [cache/cache_file.h](cache/cache_file.h), [.cpp](cache/cache_file.cpp) | The envelope of the backend's cache files: a magic, the data's length and hash, read back only when whole, saved through a temporary file of each save's own and renamed into place |
| [cache/spirv_cache_file.h](cache/spirv_cache_file.h), [.cpp](cache/spirv_cache_file.cpp) | The SPIR-V cache: what glslang made of each program and stage, in memory and in its file beside the pipeline cache, and the encoding of a compiled program |
| [cache/pipeline_cache_file.h](cache/pipeline_cache_file.h), [.cpp](cache/pipeline_cache_file.cpp) | The pipeline cache's files in that envelope: the driver's data, and the manifest of the keys made, with each key's program identity and age |
| [presentation/hdr_output.cpp](presentation/hdr_output.cpp), [shaders/hdr_encode.comp](shaders/hdr_encode.comp) | HDR output ([HDR.md](../../../docs/port/HDR.md)): the HDR swapchain, the pass that writes the frame into it, and the HDR metadata |
| [presentation/surface_format.h](presentation/surface_format.h) | Picks the swapchain's format and colour space from what the surface offers (no GPU code, so tests can check it) |
| [resources/vma.cpp](resources/vma.cpp) | The Vulkan Memory Allocator's implementation, compiled once |

## Depends on

`renderer` (the GL interface it carries out, `renderer/gl/gpu_draw.h`), `renderer/world`
(`gl_world.h`'s calls and enums) and `common` (the log, the statistics, `float_encoding.h`). Below
them, SDL3 for the window's surface. The third-party libraries it wraps: volk (Vulkan's functions),
the Vulkan Memory Allocator, and glslang (in `pipelines/shader_compiler.cpp` only). The CMake target is
`openrac_renderer_vulkan` (`port/cmake/Window.cmake`).

Each program retains its last complete graphics pipeline key. Repeated draws compare that key
before hashing a context-cache lookup. Relinking or deleting a program clears its retained
pipeline before retirement. The context cache and pipeline pre-warm path remain available.

Shadow-depth combination uses the CPU [frame planner](../graph/README.md) to declare
its sampled inputs, storage output and exported sampling state. `GraphExecution`
records those dependencies on the existing command buffer; `graph/graph_states.cpp`
translates barriers and updates the legacy layout tracker. The dispatch, descriptors and frame-slot ownership are unchanged.

`graph/graph_execution.h` runs synchronous pass callbacks against borrowed image and storage-buffer
allocations. It validates the complete subgraph before execution, records exact image or buffer
barriers, and exports consumer states on the existing queue. GTAO's prepass, optional Hi-Z,
horizon and filter integration uses this executor.

Mesh material records select base, emission and edge-aware upscaled colour textures from the
same bindless array, so changing the upscaled texture does not split compatible indirect batches.
Cache readiness stays a shared uniform to keep its fragment branch uniform within a batch.
Sampler and mip-view identity remain part of each array entry. Non-material shaders retain
ordinary descriptor bindings.

Rendering tests may explicitly enable `vkgl::test_reuse_programs`. A bounded context-local pool
retains immutable modules, layouts and pipelines across fresh GL links only when the complete
SPIR-V and layout modes match. Uniforms and descriptor state always start fresh. The pool is off
for player launches, bypassed by cache/lifecycle equivalence cases, and released before device
teardown. The focused support test checks pixel equality, uniform reset and driver-call savings.
Shadow and probe matrices also reuse these objects. `sun_shadow_test --gpu --fresh-programs`
(or `--gpu-reference --fresh-programs`) and `probe_gpu_test --fresh-programs` run the same checks
with fresh objects for performance and correctness comparisons.

## State and threads

All of the backend's state is one `Context`, reached through `detail::g` (`vkgl_internal.h`): made
by `open()` and destroyed by `close()`, null while the backend is closed. Only one backend is open
at a time, with one host thread accessing it. The frontend's render worker records an owned world
snapshot while the game simulates its next tick. A completed mutex/condition-variable handoff
returns exclusive ownership to the window thread for debug UI, final composition and presentation.
GPU submission keeps its existing two-frame queue; recording a world snapshot adds no submission.
Resource changes, readbacks and shutdown drain the worker first. The one asynchronous callback is
`on_event`, which SDL calls from its event thread when an Android app returns to the foreground; it
only sets `surface_lost`, which is atomic. Two frames are in flight: each frame slot's objects are
reused, and its deferred work run, once its fence has signalled. A static buffer of 64 KiB or more
lives in device memory from its first draw and its CPU copy is let go; a reader of its bytes reads
them back with a command buffer of its own (`buffer_bytes`,
[RENDER_PERF.md](../../../docs/port/RENDER_PERF.md), "R-05"). Turning `g` into an object the
backend passes around is P3-03 of the restructure
([PROGRESS.md](../../../docs/restructure/PROGRESS.md)). While a pass is open under `attachment-discard`, `Frame::main` is that pass's
secondary command buffer; the pass's end gives the frame's own back (`OpenPass`).

`Context::recorded` (a `RecordedState`) is what the frame's main command buffer holds of a draw's
state: the viewports, scissors, blend constants and stencil states last set, and the descriptor
sets with their layout and offset, the push constant, the vertex buffers and the index buffer last
bound. A draw records only what differs from it (the registry's `vk-state-skip`); the pipeline is
`Context::bound_pipeline`. It is forgotten at every begin of rendering and of the command buffers,
so nothing is trusted across passes, and a change of pipeline layout forgets the sets. Code that
records graphics state or binds into `main` outside `draw()` must forget it too.

The draw finds its pipeline in `Context::pipelines`, a `PipelineTable` (each pipeline once, found
by a hash of its key's bytes and a compare of the whole key, or by the key as a string on the
reference path), and its descriptor set in `Program::set_cache`; its temporary arrays are
`Context::scratch`'s, kept for their capacity, so a draw that finds both allocates nothing (the
registry's `vk-draw-cache`). A deleted program's pipelines leave the table with it.

Worker threads (one to four) build pipelines ahead (`PipelinePrewarm`, R-10). When a program
links, the keys earlier runs used (`PipelineManifest`) are queued. One mutex guards the queue and
results. The drawing thread takes a finished key, waits for a queued one, and builds the rest
itself. A level load waits for the queue (`vkgl::finish_pipelines`). The backend's close joins
the workers first.

The game keeps a deleted program's modules, layouts and pipelines (`vkgl::keep_programs`, the
registry's `vk-program-keep`, `test_program_cache.cpp`). The next link of the same SPIR-V and
layout takes them under its new serial, so a level's load makes no pipelines again.

A shader's mode uniforms (`// openrac_mode: ...`) are specialisation constants of its pipelines
(`vk-mode-constants`, `mode_sources` in `gl_program.cpp`). The key holds their values. A draw
whose specialised pipeline is not made yet asks the workers for it and uses the pipeline that
reads the uniforms (`stand_in_pipeline`).

With `vk-pipeline-library` (desktop), the workers also build each program's two shader parts. A
draw with an unknown key links four parts (`PipelineLibraries`) at once; the whole pipeline is
built on a worker and swapped in at a frame's start (`collect_prewarm`). The manifest and
pipeline cache are saved on `Context::cache_saver` at each load, every two minutes, and at close.

`Context::spirv_cache` keeps what glslang made (R-12): `compile_program` and `check_stage` take a
program or stage compiled before from it, by a hash of its kind, sources and compiler checked by a
second hash and the length, and glslang runs only for the rest. It is read at open and written at
close, beside the pipeline cache, and off when the pipeline cache is.

`Context::persistent_sets` keeps, from frame to frame, the descriptor sets that bind only
textures, samplers and the ring (R-14, `vk-persistent-descriptors`), keyed by the program's serial
and each image's `Image::serial`, which is never reused, so a set of a deleted texture is never
found again; the least recently bound are freed past 4,096.

With `vk-extended-dynamic-state` on (off by default, R-11), the cull, front face, depth and
stencil state (`FixedState`) leaves the pipeline's key and the draw records it as Vulkan 1.3
dynamic state, through the same tracker.

## Errors

`open()` returns false with a message when Vulkan cannot start (the window then falls back to
OpenGL and logs why). Recording, submission, fence and pool-reset failures stop the frame through
its existing exception boundary. A failed submit is never marked submitted or waited on. Shader
object failures leave the program unlinked with a diagnostic log. Descriptor allocation retries
only exhausted or fragmented existing pools; other failures and failures in a fresh pool stop
recording. Upload allocation failures cannot return null staging memory to callers.

Invalid texture extents, mip shifts, buffer update ranges and compute dispatch sizes are rejected
before unsafe arithmetic or driver calls. Native compute rejects image aliases involving a write
before changing layouts; duplicate read-only imports remain valid. GL input errors are recorded
for `glGetError`. Optional resource operations still return their documented failure values.
Single-sample colour copied into MSAA targets uses a fullscreen draw rather than an invalid
Vulkan image blit, including retained loading/menu backgrounds and flipped copies.
The GPU-driven path throws `std::runtime_error` when a page's fixed capacity is exceeded.
With the validation layer on, its messages go to the log and its errors are counted.
The pipeline cache's file reports a damaged file in its `SavedCache` and a failed save as false
with a message (`cache/pipeline_cache_file.h`); `context/vkgl_device.cpp` logs both as warnings, and the run goes
on building its pipelines afresh.

## Configuration

Read at start. [DEBUGGING.md](../../../docs/port/DEBUGGING.md) lists them, except the two
GPU-driven references, which [RAC1_GPU_DRIVEN.md](../../../docs/port/RAC1_GPU_DRIVEN.md) lists:

- `OPENRAC_VULKAN_VALIDATION` (`vkgl.cpp`): Khronos' validation layer.
- `OPENRAC_VULKAN_PIPELINE_CACHE` (`context/vkgl_device.cpp`): where pipelines are cached between runs,
  or off; the pipeline manifest (R-10) and the SPIR-V cache (R-12) sit beside that file. The cache file is named after a key that hashes this directory's sources
  (`OPENRAC_VK_PIPELINE_KEY`, `port/cmake/Window.cmake`), so a build whose pipeline code changed
  starts a fresh cache. Programs that save the same file at once (tests run in parallel) each
  write their own temporary file and rename it into place, and a file that is not one whole save
  is dropped on reading (`cache/pipeline_cache_file.h`): a damaged cache once drew wrong colours.
- `OPENRAC_VK_UNIFORM_REUSE_OFF` (`vkgl_internal.h`), `OPENRAC_GPU_DRIVEN_OFF` (`vkgl.cpp`),
  `OPENRAC_GPU_MATERIALS_OFF` (`draw/gpu_materials.inc`): the reference paths the faster ones are
  measured against, as statistics probes.
- `OPENRAC_GPU_MARKERS` (`commands/vkgl_commands.cpp`, `context/vkgl_device.cpp`): names the timed GPU passes for
  profilers, in a build configured with `-DOPENRAC_GPU_MARKERS=ON`.

## Graph imports

`GraphExecution` borrows resources through `import_image`, `import_buffer` and
`import_attachment`. `InitialContents::Preserve` retains valid incoming contents;
`InitialContents::Discard` requires producers to define them before any read.
These policies describe graph contents, not immediate GPU clears. Texture imports
use actual allocation identity and layout. Attachments accept one colour or depth
aspect; unsupported aspects and external layouts return an error.

## Graph transfers

`GraphExecution::copy_buffer_to_image` uses typed imported handles and an explicit
transfer-read byte range. Its work pass must declare the destination transfer write;
a later export supplies shader visibility. The command is valid only during a work
callback, checks bounds/format/texel alignment and records no extra barriers. Water's
graph and the legacy `copy_buffer_to_texture` share the native copy recorder.

## Compute graph inputs

`GraphExecution::capture_compute_reads` captures sampled images, texel buffers and uniform
blocks from the bound compute shader, using the descriptor path's texture lookup.
Storage reads and writes remain explicit pass declarations. `ComputeInputs::UniformBlocks`
omits samplers for shortlist branches that only read metadata. A failed capture leaves
the caller's pass unchanged. Loose uniforms and immutable
dummy descriptors stay backend-owned. Sampled images capture their complete mip chains
under the backend's one-layout-per-image tracker.

Read-only ring views retain their actual offsets and share one allocation identity.
A buffer subsequently bound for writable storage is promoted to stable storage,
including when it was already sampled earlier in the frame. Recording adds no
submissions, CPU waits or copies beyond existing buffer uploads.

## Tests

The renderer's tests run again on this backend under `_vulkan` names (`port/cmake/Window.cmake`),
with `OPENRAC_GL_API=vulkan` and the validation layer where it is installed (Mesa's lavapipe in
CI). Where Vulkan cannot open, the window falls back to OpenGL and they repeat the OpenGL runs.

- `renderer_direct_vulkan`, `renderer_shader_vulkan`, `touch_overlay_vulkan`, `imgui_gl_vulkan`,
  `world_gl_smoke_vulkan`, `viewer_vulkan`: every GL call the renderers make, carried out here.
- `vulkan_uniform`, `vulkan_uniform_reference`: uniform uploads and descriptor reuse, with the
  reuse on and off (`OPENRAC_VK_UNIFORM_REUSE_OFF`); a state bounce (A, B, A) draws A's pipeline
  and set again, deleting one program's pipelines leaves the others' usable, and cull and front
  face toggled in one pass draw alike as dynamic and as baked state, and a texture deleted and
  replaced never draws through the old one's set.
- `vulkan_failure`: injected shader, descriptor and submit failures, bounded upload sizes,
  framebuffer completeness and conflicting native-compute image imports, on an 8x8 offscreen target.
- `render_opt_vulkan`: the render counters of `SubmissionCounters` follow a fixed scene's draws
  (every draw a draw command, one submit a frame), printed for the reports of
  [RENDER_PERF.md](../../../docs/port/RENDER_PERF.md).
- `vulkan_present_timing`: the device opened with each presentation extension switched off
  (`vkgl::Config`), and presenting.
- `vulkan_present`: the one copy to the screen against the two it replaces, drawn into stand-ins of
  the screen and compared byte for byte, and a draw after the blit making the copy first.
- `vulkan_buffer`: a static GL buffer drawn, rewritten in part, filled again, read through 8-bit
  indices and as a uniform block, frame after frame, under the reference path, device memory and
  the staging copy (`static-buffers-*`); the mirror let go and nothing through the ring.
- `pipeline_cache_file`: the cache's file read back whole, a damaged one dropped, and writer threads
  and writer processes saving one file at once leaving one whole save; the manifest read back, and
  one of another key size, cut short or of another magic dropped. It needs no GPU.
- `spirv_cache`: a program from the SPIR-V cache is the program glslang makes, word for word; a
  damaged, stale or colliding entry gives nothing. It needs no GPU.
- `vulkan_prewarm`: a second run's draws make none of the first run's pipelines (the worker made
  them), and nothing is made ahead for a program no longer built, with the entry off, or from a
  damaged manifest.
- `early_lighting_vulkan`, `probe_gpu_vulkan`, `hero_shadow_vulkan`, `sun_shadow_vulkan`,
  `sun_shadow_filter_reference_vulkan`, `gtao_vulkan`, `gtao_horizon_vulkan`,
  `enhanced_rendering_vulkan`, `enhanced_rendering_off_vulkan`: the enhanced rendering's passes on
  Vulkan.

## Graphics graph inputs

The backend implements `renderer/frame/colour_graph_backend.h`; the core renderer
includes that contract rather than Vulkan headers. Native colour and snapshot
declarations stay in this module, and compatibility recording stays in renderer.

`GraphExecution::capture_graphics_reads` captures the bound graphics program's
samplers, uniform ranges and read-only storage ranges. Both vertex and fragment
reader scopes are retained. Capture is transactional, and rejects compute programs.
Callers must declare any writable storage separately. Graphics samplers import
complete mip chains under the backend's one-layout-per-image tracker, as do compute
samplers. Writable-image imports retain their single-level contract.

## Further reading

- [vkgl.h](vkgl.h): how GL's objects, programs, state and rows map onto Vulkan.
- [docs/port/ANDROID.md](../../../docs/port/ANDROID.md): why the port has a Vulkan renderer, and
  the custom drivers it serves.
- [docs/port/RAC1_GPU_DRIVEN.md](../../../docs/port/RAC1_GPU_DRIVEN.md): the GPU-driven instances
  and the native material path.
- [docs/port/DEBUGGING.md](../../../docs/port/DEBUGGING.md): the variables above, and the
  counters (`vkgl::submission_counters()`).
- [docs/port/RENDER_PERF.md](../../../docs/port/RENDER_PERF.md): every render counter and gauge,
  where it is counted, the timed sections, and the optimisation registry.

## Temporal graph recording

`graph/temporal_graph.h` declares synchronous temporal raster, complete colour blit and
opaque native-compute phases. Raster inputs are explicit; read-only graphics storage
and palettes are captured once at object-motion shader setup. Overwrite and preserved
outputs have different contracts, and final display attachments remain available to
the HUD. No allocation or submission is added by these helpers.

`graph/graph_presentation.h` is the backend's private acquired-image bridge. It records the
compiled transfer and PRESENT transitions on the command buffer already chosen by the
presentation path. Swapchain ownership and semaphore waits stay with the context.
Vulkan uses graph dependency planning for temporal recording.

Temporal composition joins compatible temporal phases in a shared plan. Native SDK
dispatches flush the pending section before returning their success or failure;
fallback copies, blur and colour grading can then form the next section. SDK-private
histories and barriers remain opaque. Late overlays, HUD and acquired-image
presentation retain their existing boundaries.

`PreparedRaster` retains each pass's loose uniforms, sampler/block bindings and
framebuffer attachment descriptions. Execution installs those values and restores
later preparation values afterward, including on callback failure. Owners and
callbacks remain alive until the synchronous scope returns. Displayed-frame
history advances only after the complete temporal scope succeeds.

## Frame capture

`diagnostics/frame_capture.h` requests and reads a CPU-only frame-wide diagnostic on the owning
graphics thread. The existing executors append declarations at pass entry and after
nested work. Presentation compiles the capture, independently of the GPU submission.
`OPENRAC_RENDER_GRAPH_DUMP=1` logs one nonempty capture. Disabled capture retains no
source declarations. See [frame-wide diagnostics](../graph/README.md#frame-wide-diagnostics)
for opaque boundaries and the limits of the captured model.

## Prepared surface recording

`graph/surface_batch.h` joins existing surface graph declarations within the synchronous
`renderer::run_surface_chain` scope. `graph/surface_batch_internal.h` owns the temporary
plan, image bindings and callbacks. Each compute work pass prepares one immutable
`PreparedDispatch`; multiple dispatches require separate declared passes. Graphics
and native-compute callbacks opt into deferred recording through `run_prepared`.

The collector rejects undeclared draws, copies, clears, barriers and submissions
during preparation. Compute setup may upload independent inputs and create frame
descriptors, but sampled-image transitions and GPU dispatches wait for the shared
compile. GPU timing surrounds execution rather than setup. Ordinary recording uses
the same dispatch helper without collecting packets.

`graph/surface_copy.cpp` declares complete depth/stencil copies, sample-zero depth resolves
and averaged normal-colour resolves. Native copy/resolve commands retain their
original coverage and formats. `graph/attachment_copy.h` shares their command emitters with
GL-shaped blits, so graph transitions do not duplicate native barriers.
Sources, destinations, extents and sample conversions
validate before the shared section executes.

The sky scope joins shadows, atmosphere transport and sky composition. Late shadows
join the surface scope, followed by the first contiguous opaque/cutout world batch.
Explicit early-cache reads connect that batch to its compute producer; per-material
descriptor reads remain local to drawing. Blended work retains the original batch boundaries.
Deferred callbacks borrow frame/provider
state only until their enclosing call returns. Native bind caches are invalidated
when restoring raster state. Failed preparation or compilation returns an error to the existing frame
boundary. It does not retry a partially prepared frame.

Native shadow composition uses `GraphExecution`, including the existing wrapped
static-depth offset, R32F storage output and sampled export. Its descriptors, push
constants and image owners are captured before compilation. No shadow-only barrier
executor remains.

`GraphExecution::run_ordered` records whole bucket callbacks that inherit the last
ordered callback's outgoing graphics state. The first callback starts from its
captured entry state; the scope publishes the last outgoing state. It does not
replace `run_prepared`, whose low-level draws require their captured bindings.
`world_colour.cpp` retains world-layer and blend order. Effects prepare simulation
and geometry before joining refraction copies and their sampled colour/depth reads.

`ColourAttachments::Colour` declares depth-free sky blending; ordinary world phases
retain colour/depth preservation. Descriptor-only graphics reads also use prepared
callbacks, so transport visibility can join the same plan as sky attachment writes.

`BucketRenderer::record_colour` declares ordinary bucket draws or prepares known
effect geometry before retaining its draw metadata. With opted-in temporal hooks,
the last supported world/effects sequence joins opaque snapshots, refraction and
motion/reconstruction inputs in one section. Nested `temporal_chain` calls finish
pending work before returning to the history owner. Native SDK decisions still
flush synchronously; unsupported providers and HUD retain their boundaries.

`run_surface_setup` finishes pending declarations and suspends collection around
material setup. Cold water simulation can then finish its borrowed callbacks and
transfers synchronously before effect snapshots enter the next shared section.
