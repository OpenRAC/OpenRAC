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
  is drawn) and `glBlitFramebuffer` (a scaled, flipped or converting colour blit is drawn). A buffer updated while the GPU may still read it is copied
  first, so nothing waits for the GPU.
- [mtlgl_draw.mm](mtlgl_draw.mm): shaders and programs (linked by `vkgl::compile_program`, then
  translated and compiled for Metal), uniforms, vertex arrays and draws, with their pipeline,
  depth/stencil and sampler states cached. An input the vertex array does not feed reads GL's
  (0, 0, 0, 1). A texture a draw both samples and writes reads as an empty one.

Metal has no triangle fans, 8-bit indices or unnormalised float inputs from integers: draws with
them are refused (`GL_INVALID_ENUM`, then `GL_INVALID_OPERATION`), as no renderer uses them.

Some operations are refused with `GL_INVALID_OPERATION` until they are done with draws: a copy
between integer formats, a scaled or flipped blit of depth, of integers
or of a multisampled image, and a resolve of part of an image or of depth.

[tests/mtlgl_test.cpp](../../tests/mtlgl_test.cpp) (`ctest -R mtlgl`) opens it without a window.
It clears and reads back, uploads and reads textures through framebuffers, resolves a multisampled
clear, blits flipped and scaled, copies from the default framebuffer, and draws an indexed quad
with a uniform. Then, in hidden windows, it draws the same cases (clears, copies and blits
included) with OpenGL and with Metal and compares the pictures and errors. It is skipped where
there is no Metal device.

## Choosing it

`OPENRAC_GL_API=metal` (`platform/window.cpp`). The window opens a Metal window and `mtlgl::open`,
then asks `mtlgl::get_proc` for every function `gl::load` needs. If Metal cannot open, or one is
missing, the log says why and the window draws with OpenGL: not Vulkan, which on Apple's systems is
MoltenVK on the same Metal.

It draws what OpenGL draws. The enhanced renderer (shadows, water, FSR3, HDR, frame
interpolation) runs only on Vulkan's render graph, so under Metal the game looks as it does on
OpenGL.

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

Each game log's `graphics:` line names the GPU it ran on. A MacBook Pro with two GPUs gives
Metal its discrete one; for the integrated one, run `sudo pmset -a gpuswitch 0` on it first
(`sudo pmset -a gpuswitch 2` switches back). A GPU older than `MTLGPUFamilyMac2` (Haswell's Iris
Pro, for one) is not used: the game says so and draws with OpenGL.

## Still to come

- The rest of `renderer/gl.h`: compute.
- The renderers' picture tests (`render_equivalence`, `texture_filter`) run on it.
- The shader compiler moved out of `../vulkan`, so Metal no longer needs `OPENRAC_VULKAN`.
- iOS.
