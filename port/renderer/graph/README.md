# renderer/graph

A CPU-only pass planner, command contracts, submission scheduler and resource-retirement
ownership. Passes declare image subresources and buffer ranges; the planner validates
contents and produces dependencies, barriers and inclusive resource lifetimes in pass
insertion order. The submission graph orders native command streams on the existing
single queue. Vulkan executors lower those plans and report fence/idle completion.
Owned Vulkan transfer graphs can reuse backing images for graph-local transient colour
images with disjoint lifetimes.

## Interface

- `frame_graph.h`: `Graph`, typed handles, imports, pass accesses and compiled `Plan`.
- `command_buffer.h`: owned, backend-independent transfer commands. The executor checks their
  graph handles and ranges against the owning pass before compilation. Only the Vulkan encoder
  translates them into native commands.
- Pipeline commands in the same header describe direct draws, indexed draws, indirect indexed
  draws and dispatches. Vulkan checks their pass kind and stage, draw attachment bindings and
  compute storage declarations before recording work. Every bound storage allocation must be
  declared, and every declared output must be bound. Surface compute packets are owned by
  the compiled execution; the existing raster setup callbacks still supply draw bindings.
- `Plan::describe()`: deterministic text without logging or file I/O.
- `frame_context.h`: frame-wide composition for diagnostics, not GPU scheduling.
- `submission.h`: command-stream dependencies, acquisition wait placement, producer signals,
  presentation requests, retained resource owners and completion-gated retirement callbacks.
  Cycles, duplicate command streams and binary-semaphore misuse fail before submission.
- `breadcrumbs.h`: bounded operation history with begin, completion and failure events. Labels
  are copied without allocation; exceptions retain the interrupted operation and pass name.
- `recording_scope.h`: executors authorize native work while recording a compiled work pass.
  Native draws, dispatches, copies, clears, resolves and acceleration builds reject calls
  outside that boundary. Export passes cannot authorize work.

Allocation identity and generation canonicalise imported views. Handles belong to
one builder recording. Compilation rejects foreign handles and handles saved across
`Graph::clear()`, including when a plan cache entry matches. Graph composition validates
source ownership before remapping handles. Imports declare initial access and
whether contents are defined. Complete write-only accesses initialise their declared range.
Attachment loads require defined contents; clear defines them and store-discard
invalidates them. Read/write storage uses one combined declaration. Overlapping
writes in one pass are rejected.

Work passes can opt into culling with `Pass::cull_if_unused` when all outputs and effects
are visible in the current graph. Exports and other work are roots. Compilation validates
all declarations, then follows the producers of live image subresources and buffer ranges.
The executable plan omits culled passes' commands, barriers and lifetimes while preserving
declaration indices and execution order. Keep readbacks, history updates and work with
external side effects as roots. GTAO uses this for Hi-Z levels beyond the viewport's
reachable sampling range.

`GraphExecution::create_transient_colour` declares an undefined, single-sample RGBA8
image for complete owned transfers. After validation and culling, the Vulkan executor
assigns backing images in first-use order. Equal extents can share an image when one
resource's last pass precedes the other's first pass. The handoff orders the previous
transfer accesses before the new lifetime's first write, and the frame fence governs
retirement. Imported images and resources exported outside the recording use separate
ownership. The graph reports logical image count, backing image count and allocation bytes
in the frame statistics.

Buffer imports distinguish ordinary data, acceleration-structure storage and build scratch.
Their read/write dependencies share the planner; Vulkan lowers each role to its required access flags.

Export passes establish read-only boundary states without GPU work.
`shader_read_export(name, source, stages)` exports exact producer/input ranges,
removes attachment operations and selects sampled layouts. Discarded contents
cannot be exported for reading. Executors implement attachment clears and stores.

## Depends on

`common/result.h`, `common/breadcrumbs.h` and the standard library. Target: `openrac_renderer_graph`.
The planner and its validation tests need no Vulkan, SDL or graphics context.

## State and threads

Builders and plans have one owner. The pass plan retains declarations; the execution owns
commands and allocation bindings. Each Vulkan frame slot's `SubmissionLifetime` pins imported
image owners and owns deferred destruction. It releases them at the existing fence/idle
completion boundary, including owners used by failed callbacks. The baker retains buffer
owners through each submitted graph's fence and keeps persistent scene owners until teardown.

Compatibility raster draws, HUD, clears and compute dispatches build immediate graph scopes
from their effective attachments, reflected inputs and native buffer allocations. Image
uploads, readbacks, copies, blits and resolves enter through graph transfer recording.
Mip generation builds one pass per mip and exports the complete chain for shader sampling.
Vulkan mip chains and native array images retain layout, prior access and defined contents
between graphs. Imports carry per-range overrides when subresources differ. Uniform images
use inline state, and sampled declarations match the descriptor's mip view. Partial uploads
preserve existing contents without defining a previously unwritten mip. Packed depth and
stencil share Vulkan layout transitions while their contents are tracked independently on
ranged images.
Early-lighting outputs and stable buffers bound to reflected read-only graphics storage
blocks reuse recorded shader-read visibility across scopes in the same main Vulkan
command buffer. A later import reuses visibility only when it covers the full byte range
and all requested shader stages. Writes invalidate the overlapping bytes.
Vulkan batches independent image and buffer barriers at each pass boundary when
synchronization2 is available. Each barrier keeps its original stage masks and range.
Successive barriers for overlapping ranges retain their command order.
Existing shared surface chains keep their larger scopes and frozen compute packets.

The submission scheduler retains upload-before-render-before-presentation dependencies.
A separate presentation stream waits for acquisition at Transfer; HDR and combined streams
retain the conservative acquisition wait. A presentation request must consume a semaphore
signalled by its compiled producer schedule. Private readbacks and baker submissions use
the same queue executor. Backend object creation, command-pool management, driver waits and
diagnostic instrumentation remain native implementation details; destruction requests are
retired through graph lifetime ownership.

`renderer/frame/surface_chain.h` collects compatible work on the render thread. Allocations
and bindings are prepared first; compilation finishes before draws or dispatches execute.
Compute packets retain descriptors and uniform offsets. Prepared raster callbacks
retain their own bindings. Ordered bucket callbacks inherit the preceding bucket's
state and publish their outgoing state. Borrowed frame/provider data outlives the scope;
effects retain owned draw metadata through execution.

Known producers and consumers share plans in their existing order:

- Shadows: full clears, partial static/scrolling-cache preservation, cache copies,
  dynamic drawing and depth combination, followed by sampling exports.
- Sky: current shadows, atmosphere transport, authored sky and fog overlay.
- Surfaces: depth/normals, depth reuse, GTAO, tiled probe/light selection, early
  lighting and contiguous opaque/cutout world batches. MSAA preserves per-sample depth,
  averages normals and supplies sample-zero depth/stencil to single-sample GTAO.
- Water: window/history preparation, alternating solver buffers, wave-map transfer,
  deformation, lighting, authored UVs and floating poses. Moves preserve existing
  torus slots; resets overwrite local metadata.
- Colour: ordered opaque/blended buckets, independent refraction colour/depth
  snapshots and effects. Snapshot allocation precedes imports; exports follow consumers.
- Temporal: motion/depth/reactive inputs, native FSR/ASR, blur, AgX and presentation.
  Complete outputs discard old contents; overlays and partial gathers preserve them.

Shortlist prefixes, tile ranges, secondary-camera inputs and read-only ring views
retain exact buffer ranges and shared allocation identities. Shader reads match
bound descriptors; writable storage needs stable allocations. Graphics inputs are
captured at shared shader setup rather than per material draw.

Optional Hi-Z reductions share the surface plan with GTAO and early lighting.
When their lighting inputs are declared, adjacent prepass and world-colour scopes compile
together in their existing order, including an eligible temporal tail.
GTAO, probe selection and early lighting mark their sampled input handoffs as continuations.
The joined plan elides a marked export when later work declares a read or complete overwrite
covering that image range. Section boundaries retain unresolved handoffs and required exports.
Unknown hooks and providers without preparation support use separate scopes.
When GTAO needs copied depth, the shared scope borrows a depth/stencil scratch
target. Vulkan retains one matching allocation across scopes and retires replaced
allocations behind the frame fence.
Water refraction can borrow the same depth target in a separate scope, with an
independent RGBA8 snapshot. Overlapping leases use the existing owned targets.
Blend boundaries retain opaque batch order. `run_surface_setup` finishes pending work
and suspends collection for synchronous provider setup, including cold water simulation.
Native SDK decisions flush before their result is consumed; private histories and
internal barriers remain SDK-owned. Pose/history commits follow successful recording.
HUD drawing and acquired-image presentation remain explicit boundaries.

GPU-driven materials, vertex streams and indirect batches use backend-managed bindings.
See the [Vulkan backend](../vulkan/README.md) for its import and transfer contracts.

## Errors

Imports and compilation return `Result`. Invalid ranges, unwritten reads, inconsistent
aliases, incompatible accesses and conflicting declarations fail before execution.
Crash breadcrumbs cover compilation, scheduling, pass recording, command execution, queue
submission/presentation and retirement. Successful events use the active main/render frame;
failures are retained even outside a frame. They add no routine log output or GPU waits.
A completed submit means driver acceptance; only retirement follows confirmed GPU completion.

Errors identify the pass or resource. SDK rejection can select the existing reconstruction
fallback before recording resumes.

## Configuration

Vulkan always uses graph planning for the migrated passes. OpenGL keeps its compatibility
recording. Hi-Z is independently configurable and defaults off.

Frame capture is opt-in: `OPENRAC_RENDER_GRAPH_DUMP=1` logs the first nonempty presented
capture once, skipping presentation-only startup frames. `vkgl::capture_frame_graph()`
starts capture before frame work; `vkgl::frame_graph_description()` reads it after
presentation. Normal recording does not copy diagnostic declarations.

Capture canonicalises identities across scopes, derives lifetimes/dependencies and
records nested completion events and exports. Native command gaps become conservative
opaque import producers. Failed imports or callbacks invalidate capture; a later import
cannot revive discarded contents without an explicit opaque boundary. Diagnostic
barriers are never applied to the GPU and cannot justify barrier removal or memory reuse.

## Tests

| Tests | Coverage |
|---|---|
| `graph_breadcrumbs` | Operation order, named failures, exception unwinding, bounded owned labels and frame context. |
| `frame_graph`, `frame_context` | Hazards, all readers, subresources, partial writes, aliases, handles, attachment operations, exports and deterministic cross-scope diagnostics. |
| `shadow_graph`, `depth_combine` | Fresh/partial/cached/refreshed/recreated maps, wrapped offsets, depth/stencil inputs and later sampling against arithmetic. |
| `surface_chain`, `sky_chain`, `sky_chain_separate`, `sky_chain_scroll` | Complete output, AO scales, MSAA, resizing, shadows, visible sky/fog, draw order, shared consumers and failure recovery. |
| `colour_graph`, `effect_material` | Refraction snapshots, depth bits, graphics mip/range capture, rejected feedback, inherited blend/mask state, real water/particles and retained metadata. |
| `graph_compute`, `vulkan_failure` | Ring aliases, storage promotion, cross-scope buffer visibility, transactional input capture, invalid ranges/layouts and rejection before dispatch. |
| `graph_immediate_work` | Transfer resource/state/range enforcement, depth/stencil copy masks, repeated VBO draw batching, staged immutable uploads and compute mip-chain sampling. |
| `graph_attachment` | Undeclared and read-only attachment rejection, writable draws/clears, read-only depth, masked clears and stencil-aspect contracts. |
| `graph_state_translation` | Host geometry uploads, build-to-query visibility, scratch reuse and allocation-role alias validation without a GPU. |
| `gtao_depth`, `gtao_vulkan` | CPU depth reductions, odd extents, Hi-Z on/off, normals, copies and MSAA. |
| `temporal_graph`, `frame_capture_environment`, `fsr3_frame` | Imports, undefined history, provider rejection, capture activation, unchanged output/counters, reconstruction and native HUD. |

Early-lighting, probe/environment selection, local-light and water tests cover their
kernels and graph consumers. Run `frame_graph_test --dump` for a synthetic dependency dump.

## Further reading

- [Vulkan backend](../vulkan/README.md)
- [Rendering controls](../../../docs/port/RENDER_PERF.md)
