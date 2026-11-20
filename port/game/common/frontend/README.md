# Game frontend

The native game's window, scene capture, input and frame pacing. Game memory is read while
capturing a tick; rendering consumes the captured scene and does not read live game memory.

## Interface

- `../frontend.h`: window lifecycle, scene collection, interpolation and image transfers.
- `state.h`: internal scene, presentation and lifetime state shared by the frontend files.
- `render_worker.h`: persistent worker with one accepted job, including the running job.
- `scene_frame.cpp`: world target setup, lighting and temporal reconstruction.
- `scene_interpolation.cpp`: camera, object, joint and shadow interpolation.
- `scene_effects.cpp`: captured strip, callback and particle interpolation.
- `effect_attachment.h`: transform history for attachments, including objects with no mesh.
- `scenery_capture.h`: owned scenery records; camera visibility leaves shadow casters resident.
- `sleeping_mobys.h`: far mobys the game leaves asleep, animated in the picture only.
- `scene_hud.cpp`: captured HUD data, offscreen menu objects and retained backgrounds.

Physical effect attachments use the parent's displayed transform. Directly captured quads start
at its current tick; marked strips in a delayed display list start at its previous tick. Callback
ownership alone leaves world-space trails and beams unchanged.
Explicit particle position anchors also apply before a new slot has interpolation history.
Rigid sprites retain their world offsets and orientation while following the displayed anchor.

## Depends on

The common, runtime, platform, scene, renderer and enhanced libraries. Window events use SDL.
The worker uses the standard threading library and the port's `JoiningThread`. It captures the
producer's floating-point environment before thread startup and restores it before every job,
including rounding and denormal handling on the supported toolchains.

## State and threads

The window thread captures game callbacks into `Kept`. The optional Vulkan worker shares that
immutable snapshot and owns a copy of the graphics settings. The level and renderer stay pinned
until completion, and the producer checks exclusive snapshot ownership before capturing again.

At most one render job is accepted at once. The next simulation tick and CPU camera/object/pose
capture can overlap world recording. One optional `PreparedTick` owns that next capture; `frame()`
consumes it after the previous job completes. CPU preparation reads stable game memory and
immutable level models. Loading and space-flight scenes use synchronous capture.

A mutex and condition variable publish job completion before the window thread touches graphics
state again. Debug UI, final composition and presentation run on the window thread. GPU queue
submission and its two frame slots are unchanged. OpenGL records everything synchronously on
the window thread.

Level changes, texture uploads, image readbacks, display changes, pause redraws and shutdown
respect this boundary. Upload callbacks during simulation retain owned bytes until the next
capture. Readbacks finish and present the pending frame first. Interpolated redraws finish before
returning to pacing; they cannot accumulate a queue. A pending tick frame can be presented one
simulation tick later, trading bounded latency for overlapping CPU work.

## Errors

The worker catches render exceptions, reports the message at the next handoff and rejects later
jobs. The frontend catches snapshot allocation and thread startup failures, logs them and stops
the game. Shutdown finishes and joins the worker before releasing graphics owners.

## Configuration

Vulkan uses the render worker by default. `OPENRAC_RENDER_THREAD=0` selects synchronous rendering
for debugging and comparisons; OpenGL always stays synchronous. Graphics
settings are copied with each snapshot; player-facing settings are unchanged. `render snapshot ms`
and `render thread wait ms` in the debug dump measure snapshot preparation and producer wait.
`render preparation ms` measures the next tick's CPU camera/object/pose capture before that wait;
it overlaps the previous frame's render sections and is not part of the current frame's scene lap.
See [debugging](../../../../docs/port/DEBUGGING.md) for run and capture switches.

## Tests

`frontend_geometry` checks attachment transforms, rotation, nonuniform scale and singular axes.
`effect_attachment` checks meshless anchors, missing and reused history, newborn sprites,
orientation and rigid/nonrigid quad blending. `rac2_effect_attachment` checks intact, floating
and invalid callback owners; `rac2_moby_view` checks nanotech positions and particle owners for
all three releases. `effect_quad` checks that RAC1 quads leave physical attachments unset.

`render_worker` checks owned input, slot reuse, publication, captured-owner release, exception
reporting, inherited rounding and joining at destruction. `stats`, `frame_counter`, `settings`
and `env` cover profiling and configuration. `scenery_capture` covers offscreen caster residency,
persistent hiding, draw-distance limits and owned matrix capture. `sleeping_mobys` covers the
game's activity test, which sequences loop and the sleeping mobys' looped keys. Game integration checks need locally extracted data:
compare matching views with Vulkan validation, including menu readbacks, render-size changes
and quitting with a frame pending.

## Further reading

[Port design](../../../../docs/port/DESIGN.md) and
[Vulkan backend](../../../renderer/vulkan/README.md).
