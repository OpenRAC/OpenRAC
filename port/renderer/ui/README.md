# renderer/ui

What the game programs draw over the game's picture with the renderer's GL (`renderer/gl.h`): Dear
ImGui's draw lists on a window whose GL is the Vulkan renderer's, the on-screen pad, the frame-rate
counter, the CPU-made picture of the developer level menu, and the logo these show. Library `openrac_renderer_ui`,
namespace `openrac::renderer::ui`. It holds the GL half of these overlays only: what they show and
when (the debug overlay's panels, the pad's layout, the menu's levels and keys) stays with the
program in `game/common` and `platform/`. The files came from `game/common` (restructure package
P2-06), which keeps no GL call of these overlays.

## Interface

| Header | Declares |
|---|---|
| [`imgui_gl.h`](imgui_gl.h) | `imgui_gl::init`, `render`, `shutdown`: ImGui's draw data drawn through the renderer's GL, for the window where ImGui's own OpenGL backend cannot load a system GL (the Vulkan renderer's) |
| [`touch_overlay.h`](touch_overlay.h) | `touch_overlay::draw`, `release`: the on-screen pad of `platform/touch_pad.h` drawn over the window, one glass per control |
| [`logo.h`](logo.h) | `logo::draw`, `Mask`: the OpenRAC logo (the launcher's paths) as a coverage mask made on the CPU, the mark alone or with the name, for the pad's options button and the drawer's header |
| [`fps_counter.h`](fps_counter.h) | `fps_counter::frame_drawn`, `draw`, `release`: the player's `show_fps` counter, counted over the frames shown, drawn in the picture's top left corner |
| [`canvas.h`](canvas.h) | `Canvas`, `rgba`: an RGBA8 picture drawn on the CPU, filled rectangles blended by alpha and text in a 5 by 7 font, for the debug menu's level list and the mods' menu pages (`game/common/mod_menu.cpp`, interface 1.5); plain memory, no GL |
| [`overlay_list.h`](overlay_list.h) | `OverlayList`, `overlay_project`, `overlay_fade`: what mods draw over the game (`openrac_overlay`), a frame's rectangles and text as triangles in the 512 x 448 picture, and a point of the level found on it; no GL |
| [`overlay_draw.h`](overlay_draw.h) | `OverlayDraw`: those triangles blended over a framebuffer with the renderer's GL, so on OpenGL and on the Vulkan renderer's alike |
| [`picture_blit.h`](picture_blit.h) | `PictureBlit`, `PictureView`, `PictureRect`: an RGBA8 picture made on the CPU, uploaded and blitted with nearest filtering into a rectangle of a framebuffer (the debug menu's level list, `game/common/debug_menu.cpp`) |

## Depends on

- `renderer` (the GL functions, `renderer/shader.h`), below it on the same layer.
- `platform` (`platform/touch_pad.h`, the pad the overlay draws), a lower layer.
- `common` (the log and asynchronous frame statistics).
- Dear ImGui (`openrac_imgui`), privately: `imgui_gl.h` names only `ImDrawData`, by a forward
  declaration.

## State and threads

The host passes the selected `fps_counter::Mode` after each presentation. Basic mode shows FPS
and elapsed frame time. Detailed mode shows elapsed CPU time outside the swap call and completed
GPU pass timings; the two values overlap and are not added together. The counter does not read
game settings or wait for GPU results.

- Each overlay keeps its GL objects (programs, buffers, textures) in state of its own file, made at
  the first draw (`imgui_gl::init` for ImGui) and freed by its `release` or `shutdown`.
  `fps_counter` also keeps the frames counted since its last update. A `PictureBlit` belongs to its
  user and holds its texture and read framebuffer until `release`; its destructor frees nothing,
  as the context may be gone by then.
- Everything is called on the thread that holds the graphics context, the program's main thread,
  with the context current.

## Errors

- `imgui_gl::init` returns `false` and says why in its `std::string& error` when its program does
  not build; the debug overlay then logs a warning and stays off.
- The touch overlay logs a warning and draws nothing when its shader does not build.
- The other draws cannot fail; a GL error is the renderer's to report (`renderer/gl_stats.h`).

## Configuration

None of its own: the frame-rate counter is shown when the player's `show_fps` setting is on
(`game/common/settings.h`), the touch pad while `platform/touch_pad.h` says it is visible, and the
debug menu's picture while the menu is open (`OPENRAC_DEVELOPER`, `OPENRAC_DEBUG_MENU`;
[docs/port/DEBUGGING.md](../../../docs/port/DEBUGGING.md)).

## Tests

| ctest | Proves |
|---|---|
| `imgui_gl`, `imgui_gl_es`, `imgui_gl_vulkan` | a window's filled background drawn through `imgui_gl` lands where ImGui put it, in its colour, and nothing outside it, on OpenGL, OpenGL ES and the Vulkan renderer |
| `touch_overlay`, `touch_overlay_es`, `touch_overlay_vulkan` | the pad's shader builds, nothing is drawn before the screen is touched, and a held button is drawn brighter than one let go |
| `ui_overlay_list` | what mods draw, on the CPU: rectangles, text that keeps its shape, bad numbers left out, a level's point on the picture with a reversed-depth camera |
| `overlay_draw`, `overlay_draw_es`, `overlay_draw_vulkan` | those triangles read back from a frame's kind of target: in place, the right way up, blended, the frame's alpha kept |
| `ui_canvas` | `Canvas` clips rectangles to the picture and blends them by alpha, and its text lights a glyph's pixels and nothing else, at a size, over lines |

All of them but `ui_canvas` need a graphics context and report themselves skipped (exit 77) where none can be made.
`fps_counter` and `PictureBlit` have no test of their own: what they draw is checked by the
maintainers' regression comparison of the game programs
([docs/port/REGRESSION.md](../../../docs/port/REGRESSION.md)).

## Further reading

- [docs/port/DEBUGGING.md](../../../docs/port/DEBUGGING.md): the debug overlay (F3) and the
  developer menu these overlays draw for.
- [docs/port/ANDROID.md](../../../docs/port/ANDROID.md): the on-screen pad.
