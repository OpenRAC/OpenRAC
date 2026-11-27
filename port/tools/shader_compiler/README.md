# openrac-shader-compiler

Compiles and embeds built-in Vulkan shader variants during the build, without a window or GPU.
Vulkan still creates GPU-specific pipelines at runtime and keeps its pipeline cache and prewarming.
OpenGL/ES compilation and caches, and the probe baker's independent compiler, are unchanged.

## Interface

CMake runs the tool automatically. To build and run it directly:

```sh
cmake --build port/build/release --target openrac-shader-compiler
port/build/release/openrac-shader-compiler --output port/build/release/generated/shader_package
```

Outputs are `shader_package_data.cpp` (built into `libopenrac-shaders`), `shaders.bin`, `shaders.json`
(variant names, keys and sizes), and `shader-compiler.log`. SPIR-V and reflection share LZ4-compressed
16 KiB blocks, restarting at vertex, fragment, compute and reflection boundaries so different stage
pairings share identical blocks. Lookup decompresses one requested program. Artifacts and compilation
caches stay in the build directory. `--version` prints the compiler version and package fingerprint.

### Recipes and variants

Recipes are C++, beside the owning renderer. Shared assembly covers scene uploads, enhanced
lighting, water, GTAO, FSR and inline UI/backend sources. Add new shaders and source-changing choices
to these recipes, preserving runtime's definition/include order. `scene/shader_recipes.h` shares
scene assembly; the tool's `scene_recipes.cpp` connects its lighting provider.

Variants cover enhanced rendering, FP32/FP16, clip distance, material buffers, draw uniforms, clamped
colour, lighting/shadow quality, water optics, texture wrapping and supported FSR permutations.
Feature modes and bindless texture capacity remain pipeline specialization constants. Arm ASR
already supplies precompiled shaders. Source, recipe, include or compiler changes regenerate the
package; successful CPU compilations are cached independently of recipe changes.

FSR recipes share runtime's four supported Lanczos/sharpening combinations, retaining FP32 and the
supported FP16 history paths. Modern-shadow and rigid-prepass recipes require enhanced rendering.

### Build summary

Each invocation prints and saves total wall time, generation/output time, recipe requests, unique
graphics/compute/native variants, compiler cache hits, fresh compilations and reused requests.
Family rows cover Renderer, Scene, Enhanced, Native and FSR. Shared programs count once; stage
validation and vertex fallback references are separate from executable variants. The summary also
reports package size. Failures save partial counts and their error. Timings exclude building the
tool and compiling generated C++.

## Depends on

Shared renderer source assembly, glslang compilation/reflection, SPIRV-Tools performance optimization
and LZ4 compression. Optimization validates each module and preserves descriptor bindings and
specialization constants. Offline and development-time compilation use the same pass list for each
stage. Passes inline helpers, propagate constants, remove dead code and eliminate redundant loads.
Fragment shaders omit local multi-store elimination and expression simplification, which caused
a Lavapipe hang and pixel mismatches in the mixed water and particle test. Unlimited scalar
replacement and loop expansion are excluded to keep variant build costs bounded. The tool links no
GPU backend or windowing code. Its source, recipe and package libraries also serve the renderer.

## State and threads

Each invocation owns its recipes, variants, includes and report, and compiles recipes on worker threads.
The runtime package borrows the shader library's immutable bytes and decompresses into owned buffers;
native shader lookups can run concurrently.

## Errors

Compilation, fingerprint and publication failures return 1 and save the error when the output
directory is writable. Invalid CLI usage returns 2. Files are published atomically; unchanged
output keeps its timestamp. Missing or damaged compilation cache entries are compiled again.

Missing graphics variants use embedded magenta/black SPIR-V, including development builds, before
any writable cache. Known vertices retain their geometry, transforms and uniform offsets; removed
fragment resources are excluded from descriptors and graph reads. Unknown vertices draw a viewport
triangle within the scissor, overriding blend/depth/stencil/cull/colour masks for that draw only.
They skip passes without writable colour attachments, including depth prepasses on combined targets.
Missing keys and substitutions are logged once per context; renderer diagnostics include the shader
name. Depth-only targets cannot show colour. Compute shaders keep their compilation/failure policy.

## Configuration

- `OPENRAC_SHADER_RUNTIME_COMPILER=ON` permits compute compilation and custom tools opting into
  `WindowConfig::compile_missing_shaders`. Normal game windows use the error shader. Standalone and
  Android presets set it `OFF`, removing glslang from the runtime path while retaining the fallback.
- `OPENRAC_SHADER_PACKAGE_STRICT=1` fails on missing entries, bypassing the checkerboard and writable
  SPIR-V cache. Coverage tests use this mode. See [debugging](../../../docs/port/DEBUGGING.md).
- `OPENRAC_SHADER_PACKAGE=OFF` builds host tools or development-only shaders without a package or
  error shader; runtime compilation must be enabled to render.

- `OPENRAC_SHARED_SHADER_PACKAGE=ON` (default) builds the package as one shared library,
  `libopenrac-shaders` (`.so`, `.dll`, `.dylib`), beside the programs. The package is the same for
  every game and version, so each install carries it once. Programs find it by a path relative to
  themselves. iOS, or `OFF`, builds it into each program.

Cross builds create a native compiler in `shader-compiler-host`, reusing pinned dependency sources.
macOS uses a host compiler when the selected target architectures exclude the host; universal builds
can run their own compiler. `OPENRAC_SHADER_COMPILER=/path/to/tool` supplies an external executable.
Its fingerprint must match the target source tree and configuration.

## Tests

`shader_compiler_report` checks summaries. `shader_package` checks archive/reflection round trips
and corrupt input without a GPU. `shader_package_vulkan` loads production recipes through Vulkan
with strict lookup and disk caches disabled; its FP32 variant also disables material buffers and
draw-uniform optimization. FSR tests load shipped native permutations. `shader_error` checks both
fallback pixel paths, restored masks, depth prepasses, removed fragment reads and zero runtime
compilations; `shader_error_strict` checks refusal. Custom GLSL fixtures require a compiler-enabled
build and opt in explicitly.

## Further reading

[Architecture diagram](../../../docs/port/shader-compiler-architecture.svg) and
[Vulkan renderer](../../renderer/vulkan/README.md).
