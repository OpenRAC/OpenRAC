# renderer/graph

A CPU-only, fixed-order planner. Passes declare image subresources and buffer ranges;
the planner validates contents and produces dependencies, barriers and inclusive
resource lifetimes. It does not allocate resources, reorder passes, alias memory or
submit commands.

## Interface

- `frame_graph.h`: `Graph`, typed handles, imports, pass accesses and compiled `Plan`.
- `Plan::describe()`: deterministic text without logging or file I/O.
- `frame_context.h`: frame-wide composition for diagnostics, not GPU scheduling.

Allocation identity and generation canonicalise imported views. Handles belong to
their builder; unbound handles are rejected. Imports declare initial access and
whether contents are defined. Write-only accesses initialise their complete range.
Attachment loads require defined contents; clear defines them and store-discard
invalidates them. Read/write storage uses one combined declaration. Overlapping
writes in one pass are rejected.

Export passes establish read-only boundary states without GPU work.
`shader_read_export(name, source, stages)` exports exact producer/input ranges,
removes attachment operations and selects sampled layouts. Discarded contents
cannot be exported for reading. Executors implement attachment clears and stores.

## Depends on

`common/result.h` and the standard library. Target: `openrac_renderer_graph`.
The planner and its validation tests need no Vulkan, SDL or graphics context.

## State and threads

Builders and plans have one owner. Plans retain declarations, not GPU allocations
or callbacks. Resource owners and executors handle allocation lifetime and frame-slot
retirement. Vulkan imports are marked used before recording, including failed callbacks.

`renderer/surface_chain.h` collects compatible work on the render thread. Allocations
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

The Vulkan executor's import, transfer and retained-state contracts are documented
in [renderer/vulkan](../vulkan/README.md). GPU-driven material details, vertex streams,
indirect batches and immutable uploads remain backend-managed. This is scoped graph
execution, not a complete frame scheduler or transient allocator.

## Errors

Imports and compilation return `Result`. Invalid ranges, unwritten reads, inconsistent
aliases, incompatible accesses and conflicting declarations fail before execution.
Errors identify the pass or resource. Failed preparation does not retry a partially
recorded frame. An SDK rejection may select the existing complete reconstruction fallback.

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
| `frame_graph`, `frame_context` | Hazards, all readers, subresources, partial writes, aliases, handles, attachment operations, exports and deterministic cross-scope diagnostics. |
| `shadow_graph`, `depth_combine` | Fresh/partial/cached/refreshed/recreated maps, wrapped offsets, depth/stencil inputs and later sampling against arithmetic. |
| `surface_chain`, `sky_chain`, `sky_chain_separate`, `sky_chain_scroll` | Complete output, AO scales, MSAA, resizing, shadows, visible sky/fog, draw order, shared consumers and failure recovery. |
| `colour_graph`, `effect_material` | Refraction snapshots, depth bits, graphics mip/range capture, rejected feedback, inherited blend/mask state, real water/particles and retained metadata. |
| `graph_compute`, `vulkan_failure` | Ring aliases, storage promotion, transactional input capture, invalid ranges/layouts and rejection before dispatch. |
| `gtao_depth`, `gtao_vulkan` | CPU depth reductions, odd extents, Hi-Z on/off, normals, copies and MSAA. |
| `temporal_graph`, `frame_capture_environment`, `fsr3_frame` | Imports, undefined history, provider rejection, capture activation, unchanged output/counters, reconstruction and native HUD. |

Early-lighting, probe/environment selection, local-light and water tests cover their
kernels and graph consumers. Run `frame_graph_test --dump` for a synthetic dependency dump.

## Further reading

- [Vulkan backend](../vulkan/README.md)
- [Rendering controls](../../../docs/port/RENDER_PERF.md)
