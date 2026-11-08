# renderer/metal

The port's Metal renderer for macOS (Apple Silicon and Intel), built with `OPENRAC_METAL` (on by
default on Apple's systems). It is being written in steps; today it holds the shaders and its GL
up to draws.

The target is macOS 12 on the oldest Macs the port supports (2015's MacBook Pros, AMD GPUs,
`MTLGPUFamilyMac2`): MSL 2.4, no Metal 3. Macs without Metal keep the OpenGL 4.1 renderer, and
MoltenVK under the Vulkan renderer stays as it is.

## Shaders

The renderers' GLSL is not rewritten for Metal. glslang makes SPIR-V of it exactly as for Vulkan
([../vulkan/shader_compiler.h](../vulkan/shader_compiler.h)), and SPIRV-Cross (fetched by
`cmake/Dependencies.cmake`) writes that SPIR-V as MSL. A shader therefore has one source for every
API, and its forms cannot drift apart.

Programs compile without fast math, but with a multiply fused into a later add even across
statements, as Apple's and AMD's OpenGL compile them (`shader_translate.cpp`). Apple's fuses none
in a vertex stage whose position is invariant, except in a matrix times a vector, which it adds up
column by column from the first, each column fused: that is written out.

Division follows the OpenGL beside it (`shader_translate.cpp`). Apple's divides as `fast::divide`
does, not exactly. AMD's multiplies by a constant divisor's rounded reciprocal, so that is written
out; `fast::divide` there would fold the reciprocal into an earlier multiply by a constant. Some
differences remain, each found by bits written out of a shader on both APIs:

- AMD's OpenGL divides by a variable as fast math does. Metal does so only with fast math for
  the whole program, and then AMD's compiler reorders `a * 200.0 * (1.0 / 255.0)` into
  `a * (200.0 * (1.0 / 255.0))` (even through `volatile`) and an invariant stage's matrix product
  rounds differently, so the quotient's last bit may differ there. It hardly shows: on an
  R9 M370X, `remote-mac.sh`'s level 1 differs in as many pixels with fast math as without.
- AMD's OpenGL ignores `gl_SampleMask`, where Metal keeps only the samples named. The 2D renderer
  (`../direct.cpp`) therefore discards a pixel failing the alpha test on Apple's systems, which
  drops all of its samples on either API.
- AMD's OpenGL filters 8-bit textures in eighths of a level, Metal in sixty-fourths, so a filtered
  channel may differ by 1. No function of Metal's sample gives OpenGL's: between texels 0 and 255,
  weights 28/256 and 29/256 both come out an eighth above a step on Metal (223.125 and 231.125
  eighths), and OpenGL rounds one up and the other down (224 and 231).
- AMD's OpenGL gives a pixel whose centre is on an edge to the other triangle than Metal does,
  interpolates with other low bits (every varying in `mtlgl_test`, by about 1,900 in the last
  place, up to 8,171), and leaves a new image as its memory held it.
- Apple's OpenGL folds a chain of multiplies by constants into one (`a * 200.0 * (1.0 / 255.0)`
  is `a * (200.0 / 255.0)`); Metal multiplies in the written order. The renderers' shaders fold
  such constants themselves.
- Blending with the constant colour's alpha may round a result exactly halfway between two bytes
  the other way: on an M1, 28 channels in 186,000 of every equation and factor over every byte.
  Both APIs pass the same constant, and no order of float operations fits Apple's OpenGL's choices
  better than Metal's.

- [shader_translate.h](shader_translate.h), [.cpp](shader_translate.cpp): a compiled program as
  MSL, with the Metal slot of each of its bindings. The comment at the top of the header gives the
  conventions: rows flipped in the vertex stage so viewports and reads keep GL's numbers, depth by
  the clip control function constant, and how bindings become buffer, texture and sampler slots.

[tests/metal_shader_test.mm](../../tests/metal_shader_test.mm) (`ctest -R metal_shader`) translates
each program the game builds and compiles it on the Metal device for MSL 2.4, with the clip
constant both ways, and checks that every binding has its slot. It is skipped where there is no
Metal device.

## GL

[mtlgl.h](mtlgl.h): the GL calls of `renderer/gl.h` carried out with Metal, as `../vulkan` does
with Vulkan. The header gives the conventions.

- [mtlgl_internal.h](mtlgl_internal.h): what the files share, and the command stream: one command
  buffer per frame, plus an upload one that runs before it for textures the frame has not used yet,
  so that uploads do not split its render passes.
- [mtlgl.mm](mtlgl.mm): the device, the default framebuffer (RGBA8 colour, 32-bit float depth with
  stencil) and its present to a window's `CAMetalLayer`, state, queries and clears (a scissored or masked one is drawn).
- [mtlgl_resources.mm](mtlgl_resources.mm): textures (with mips and BC6H/ASTC where the GPU has
  them), samplers, buffers, framebuffers, multisampled renderbuffers, `glReadPixels` (RGBA8, and
  RGBA16F as floats or bytes), `glCopyTexSubImage2D` (a copy between float or normalised formats
  is drawn) and `glBlitFramebuffer` (a scaled, flipped or converting blit of colour, integers or depth is
  drawn). A colour resolve is drawn too, rounding as the OpenGL beside it does: Apple's sums
  the samples' floats in order and scales the sum, AMD's rounds the average of 8-bit colour
  toward zero as a float before rounding it to a byte. A buffer updated while the GPU may still
  read it is copied first, so nothing waits for the GPU.
- [mtlgl_draw.mm](mtlgl_draw.mm): shaders and programs (linked by `vkgl::compile_program`, then
  translated and compiled for Metal), uniforms, vertex arrays and draws, with their pipeline,
  depth/stencil and sampler states cached. An input the vertex array does not feed reads GL's
  (0, 0, 0, 1). A texture a draw both samples and writes reads as an empty one.

Metal has no triangle fans, 8-bit indices or unnormalised float inputs from integers: draws with
them are refused (`GL_INVALID_ENUM`, then `GL_INVALID_OPERATION`), as no renderer uses them.

Some operations are refused with `GL_INVALID_OPERATION` until they are done with draws: a copy
between integer formats, a scaled or flipped blit of stencil, a resolve of part of depth, and part of one
multisampled image copied into another. GL itself refuses a multisampled blit that scales or, from
a multisampled image, converts.

[tests/mtlgl_test.cpp](../../tests/mtlgl_test.cpp) (`ctest -R mtlgl`) opens it without a window.
It clears and reads back, uploads and reads textures through framebuffers, resolves a multisampled
clear, blits flipped and scaled, copies from the default framebuffer, and draws an indexed quad
with a uniform. Then, in hidden windows, it draws the same cases (clears, copies and blits
included) with OpenGL and with Metal and compares the pictures and errors. It is skipped where
there is no Metal device.

## Choosing it

`OPENRAC_GL_API=metal` (`platform/window.cpp`). The window opens a Metal window and reaches mtlgl
through `platform::metal_backend()`, which the platform declares and
[mtlgl_backend.cpp](mtlgl_backend.cpp) defines, so the platform never includes the renderer. Its
open calls `mtlgl::open`, then asks `mtlgl::get_proc` for every function `gl::load` needs. If
Metal cannot open, or one is missing, the log says why and the window draws with OpenGL: not
Vulkan, which on Apple's systems is MoltenVK on the same Metal.

It draws what OpenGL draws, but for the differences under Shaders and one rounding of Apple's
GPUs. OpenGL turns z into window depth as `z / w * 0.5 + 0.5`, which Metal rounds the same only
given z as it is and a depth range of 0.5 to 1, and Metal clips or clamps depth to 0..1 before
that range. So a depth-tested draw is drawn twice (`mtlgl_draw.mm`'s `depth_form`): with -z and a
range of 0.5 to 0 for the far half, with z as it is for the near one; with depth clamped, each
draw drops the other half's fragments. Depth then comes out bit for bit as OpenGL's. That is on
Apple's GPUs: an AMD one draws nothing given a range of 0.5 to 0, so there a draw keeps
`(z + w) * 0.5`: on an R9 M370X, `mtlgl_test`'s depths then differ from its OpenGL's for 181 points
in 256, by up to 674 in the last place where depth nears 0.

What remains is which pixels a triangle covers: an Apple GPU covers a pixel whose centre is
within about a thousandth of a pixel of an edge, or on it, as the z of the edge's ends says, and
OpenGL gives the GPU a z (-w..w) that Metal's API cannot. Given `(z + w) * 0.5` instead, such a
pixel is covered as OpenGL covers it, but depth misses its last bit for about two points in
five, which shows far more where surfaces meet. So in `remote-mac.sh`'s levels about two pixels
in a hundred thousand differ on an M1, nearly all by 1. On an R9 M370X about one in ten differs,
nearly all by 1 (filtering, above), and three in ten thousand by more than 8.

The enhanced renderer (shadows, water, FSR3, HDR, frame interpolation) runs only on Vulkan's
render graph, so under Metal the game looks as it does on OpenGL.

The `_metal` tests (`ctest --preset release -R _metal`) run the drawing tests on it with Metal's
API validation, in hidden Cocoa windows: SDL's offscreen driver has no Metal.

## Trying it on your Mac

1. Build (only the Command Line Tools are needed: `xcode-select --install`), from `port/`:
   `cmake --preset release && cmake --build --preset release`.
2. Your disc as an ISO. A `.chd` is turned into one with MAME's `chdman` (`brew install
   rom-tools`): `chdman extractcd -i rac1.chd -o rac1.cue -ob rac1.iso`.
3. Set the game up once, into a folder of your choice:
   `./build/release/openrac-extractor ~/discs/rac1.iso --game rac1 --proj-path ~/OpenRAC/active/rac1/data`,
   then its levels, from the top of OpenRAC (about a quarter of an hour):
   `python3 editor/extract.py port ~/discs/rac1.iso ~/OpenRAC/active/rac1/data/port/ntsc --anywhere`.
   A folder the launcher has already set up (`<install>/active/rac1/data`) has both.
4. Play on Metal:
   `OPENRAC_GL_API=metal ./build/release/openrac-rac1-ntsc --data ~/OpenRAC/active/rac1/data --window`.
   The log's `graphics: <GPU>, Metal, the renderer's GL carried out with Metal` line says it
   is on Metal; a `Metal could not be used (...)` warning says why it is on OpenGL instead.
5. To compare, run the same command with `OPENRAC_GL_API=desktop` (OpenGL).

## Trying it on another Mac

[tools/remote-mac.sh](../../tools/remote-mac.sh) sends a build to another Mac over ssh, runs the
Metal tests there and the game on both APIs, and brings the logs and screenshots back to
`build/remote/<host>/`: `shots/<api>_<tick>.png` the game's frames, `shots/<api>_overlay_<tick>.png`
the window with the debug overlay, whose Renderer section names the GPU and the API. For an Intel
Mac, from `port/`:

1. On that Mac: System Settings, General, Sharing, Remote Login on; stay logged in at its screen
   (the tests open windows).
2. Here: `ssh-copy-id you@that-mac.local`, then
   `cmake --preset release-x86_64 && cmake --build --preset release-x86_64`.
3. `tools/remote-mac.sh you@that-mac.local --data ~/OpenRAC/active/rac1/data`.

Each game log's `graphics:` line names the GPU it ran on. Metal takes the GPU OpenGL would
(`mtlgl::open` asks CGL), so a MacBook Pro with two GPUs draws on its discrete one, or on its
integrated one after `sudo pmset -a gpuswitch 0` (`sudo pmset -a gpuswitch 2` switches back).
A GPU older than `MTLGPUFamilyMac2` (Haswell's Iris Pro, for one) is not used: the game says so
and draws with OpenGL, and `mtlgl_test` is skipped.

Compute (`glDispatchCompute`, `glMemoryBarrier`) is not handed out, so `gl::load` leaves it null as
it does on macOS's OpenGL 4.1, whose framework has neither function. What dispatches compute is
the enhanced renderer, behind `gl::is_vulkan()`.

## Still to come

- The shader compiler moved out of `../vulkan`, so Metal no longer needs `OPENRAC_VULKAN`.
- iOS.
