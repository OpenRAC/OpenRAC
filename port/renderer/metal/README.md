# renderer/metal

The port's Metal renderer for macOS (Apple Silicon and Intel), built with `OPENRAC_METAL` (on by
default on Apple's systems). It is being written in steps; today it holds the shaders and the
core of its GL.

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
  stencil) and its present to a window's `CAMetalLayer`, state, queries and whole clears.
- [mtlgl_resources.mm](mtlgl_resources.mm): textures (with mips and BC6H/ASTC where the GPU has
  them), samplers, buffers, framebuffers, multisampled renderbuffers, `glReadPixels` (RGBA8, and
  RGBA16F as floats or bytes), `glCopyTexSubImage2D` and `glBlitFramebuffer`. A buffer updated
  while the GPU may still read it is copied first, so nothing waits for the GPU.

Some operations are refused with `GL_INVALID_OPERATION` until there are draws to do them with: a
scissored or masked clear, a copy between formats, a scaled or flipped blit, and a resolve of part
of an image or of depth.

[tests/mtlgl_test.cpp](../../tests/mtlgl_test.cpp) (`ctest -R mtlgl`) opens it without a window.
It clears and reads back, uploads and reads textures through framebuffers, resolves a multisampled
clear, and copies from the default framebuffer. It is skipped where there is no Metal device.

## Still to come

- The rest of `renderer/gl.h`: programs, draws and compute, then the window choosing it with
  `OPENRAC_GL_API=metal` and falling back to Vulkan, then OpenGL.
- The renderers' picture tests run on it, as their `_vulkan` variants do.
- The shader compiler moved out of `../vulkan`, so Metal no longer needs `OPENRAC_VULKAN`.
- macOS 12 as the deployment target, an Intel and universal build, iOS, Xcode projects.
