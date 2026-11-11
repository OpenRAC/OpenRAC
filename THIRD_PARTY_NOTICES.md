# Third-party notices

Third-party code and data used by OpenRAC's shared components. Each game
directory lists its own in its `THIRD_PARTY_NOTICES.md` and source comments:

- [games/rac1/decomp/THIRD_PARTY_NOTICES.md](games/rac1/decomp/THIRD_PARTY_NOTICES.md):
  functions adapted from Lombyte, ReRAC's notes in `config/overlays/rerac_notes.tsv`.
- [games/rac1/ntsc-decomp/THIRD_PARTY_NOTICES.md](games/rac1/ntsc-decomp/THIRD_PARTY_NOTICES.md):
  libgcc and soft-float, newlib, David Gay's dtoa, code ported from rac1-decomp,
  the patched EE-GCC.
- rac2 and rac3 credit their sources in their docs
  ([games/rac2/decomp/docs/COMPILER-NOTES.md](games/rac2/decomp/docs/COMPILER-NOTES.md),
  [games/rac3/decomp/docs/wiki/Cross-Repository-Resources.md](games/rac3/decomp/docs/wiki/Cross-Repository-Resources.md)).

`port/audio/sound_player.h` follows the behaviour of OpenGOAL's 989snd
(`game/sound/989snd`; ISC License, Copyright (c) 2020-2026 OpenGOAL Team),
as do the grain type names in `port/assets/sound/sound_bank.h`.

[docs/port/OPENGOAL_NOTES.md](docs/port/OPENGOAL_NOTES.md) describes how
[OpenGOAL](https://github.com/open-goal/jak-project) (ISC, Copyright (c)
2020-2026 OpenGOAL Team) works, and the launcher's disc set-up and
`tools/extractor.py` follow how OpenGOAL's launcher and extractor behave
(their steps, folder layout and error numbers); no code from OpenGOAL is in
OpenRAC. The launcher lists its own notices in
[launcher/THIRD_PARTY_NOTICES.md](launcher/THIRD_PARTY_NOTICES.md).

## Microsoft Visual C++ runtime

The Windows installers carry `msvcp140.dll`, `msvcp140_2.dll`, `vcruntime140.dll`
and `vcruntime140_1.dll` beside the game programs, unmodified, so a player needs
no Visual C++ Redistributable. They are Microsoft's, redistributable under the
Visual Studio license terms, and are not part of OpenRAC.

## Zig and mingw-w64

The Windows installers carry [Zig](https://ziglang.org)'s program (MIT; Clang and
lld of LLVM in it, Apache-2.0 with LLVM exceptions), unmodified, and the headers of
Clang and [mingw-w64](https://www.mingw-w64.org) from its release (the system
headers are in the public domain), to build the part of Going Commando that comes
from the player's disc (`tools/asm_part.py`). Their licenses are carried in
`port/build/release/licenses/zig`.

## Embree

The offline probe baker (`openrac-enhanced-baker`: `port/enhanced/lighting/bake.cpp`,
built with `OPENRAC_PROBE_BAKER`) uses [Embree](https://github.com/RenderKit/embree)
4.4.1, Apache License 2.0, Copyright Intel Corporation. It is never linked into a
game. `tools/build_port.py --probe-baker` downloads that release's source
(checked against its SHA-256) and builds it as a static library with Embree's own
task scheduler, so the baker carries no oneTBB; on ARM it includes SSE2NEON (MIT
License), which Embree's `third-party-programs.txt` lists. The launcher's
installers carry this baker, with Embree's `LICENSE.txt` and
`third-party-programs.txt` beside it in `port/build/release/licenses/embree/`
([tools/bundle_standalone.py](tools/bundle_standalone.py)). A baker built against
a system Embree package (CI's `port-enhanced` job) follows that package's
license and notices, including those of its oneTBB. The upstream license is
[LICENSE.txt](https://github.com/RenderKit/embree/blob/v4.4.1/LICENSE.txt).

## decomp-permuter and rac3-decomp's permuter tools

[`tools/permuter`](tools/permuter/README.md) drives
[decomp-permuter](https://github.com/simonlindholm/decomp-permuter) (MIT, Copyright (c) 2019 Simon
Lindholm and contributors), fetched at a pinned commit into a cache, never copied into OpenRAC. Its
two patches, `tools/permuter/patches/*.patch`, change decomp-permuter's own files and are under its
MIT license; they are rac3-decomp's `tools/decomp-permuter-asm-passes.patch` and
`tools/decomp-permuter-aligned-scorer.patch`, rebased on the pin. `tools/permuter/engine.py` and
`tools/permuter/score.py` are adapted from rac3-decomp's `tools/permuter_engine.py` and
`tools/permuter_scorer.py` (GPL-3.0, like `games/rac3/decomp`).

## ReRAC

[ReRAC](https://github.com/re-rac/rerac), a native PC port of the US build,
documents the game's formats and systems. The following adapt its format code
and notes:

- `editor/mobys.py`: the moby instance record and what the level loader does
  with each field, from `crates/rc-formats/src/gameplay.rs` and
  `docs/plan/moby_render_notes.md`; described in [ASSETS.md](games/rac1/decomp/docs/ASSETS.md) ("Mobys").
- `editor/moby_class.py`: the moby class header and model format (packets,
  vertex cache, skinning slots, normals, untextured faces), from
  `docs/formats/moby_rac1.md` §1-§2 and `crates/rc-formats/src/moby.rs`;
  described in [ASSETS.md](games/rac1/decomp/docs/ASSETS.md) ("Moby classes").
- `editor/moby_anim.py`: moby skeletons and animation sequences, from
  `docs/formats/moby_rac1.md` §3-§4, `docs/plan/moby_animation.md` and
  `crates/rc-formats/src/moby_anim.rs`.
- `editor/collision.py`: the collision block (cell tree, packed vertices,
  faces and surface bytes), from `docs/formats/collision_rac1.md`,
  `docs/plan/collision_queries.md` and `crates/rc-formats/src/collision.rs`;
  described in [ASSETS.md](games/rac1/decomp/docs/ASSETS.md) ("Collision").
- `editor/lighting.py`: the terrain and tie lighting passes (`LightTfrags`,
  `LightTies`) and the PS2 float arithmetic they run on, from
  `crates/rc-formats/src/tfrag_light.rs` and `tie_light.rs`; their checks from
  ReRAC's unit tests. The tie light slots in `editor/ties.py` after
  `crates/rc-formats/src/tie.rs`.
- `port/assets/disc/`: the disc readers (ISO 9660, table of contents, WAD
  compression, level data, overlays, scenes, saves, messages, volumes, the
  front end), converted to C++ from `crates/rc-formats` and its specs in
  `docs/formats`; each file names its source.
- `port/assets/geometry/`: the readers of what the renderer draws (textures,
  terrain, ties, shrubs, the sky, moby classes and animation, collision and
  shadow blocks, gadgets) and the lighting passes, converted to C++ from
  `crates/rc-formats` and its specs; their tests from ReRAC's unit tests.
- `port/assets/world/`: the gameplay file, collision and its queries, occlusion,
  cameras, fonts, the HUD and PIF pictures, built on `crates/rc-formats` and
  ReRAC's specs; `port/assets/image.h`, shared by the parts.
- `port/common/ps2_float.h`: the PS2's float arithmetic the lighting passes,
  the animation evaluator and the collision queries run on, converted to C++
  from `crates/rc-formats/src/tfrag_light.rs` (module `ps2`) and
  `moby_anim.rs` (`ftoi`).
- `port/assets/sound/`, `port/media/`, `port/audio/`: sound banks, VAG, PSS and
  ADPCM from `crates/rc-formats`; the MPEG-2 decoder from `crates/rc-video`;
  the voices, envelope and reverb from `crates/rc-game/src/audio`.
- `port/renderer/world/`, `port/renderer/shaders/world/`: the world renderers
  (terrain and tie drawing and their levels of detail, the GS blend and fog
  state, the draw data the renderers take, the VU float arithmetic they
  replay), converted to C++ from `crates/rc-engine` and `crates/rc-formats`;
  the shaders converted to GLSL from `crates/rc-engine/assets/shaders`.
- `port/extract/`: `openrac-extractor`'s JSON-lines output, crash-safe writes,
  free-space check and decompile step, after `crates/rc-extract`.

ISC License

Copyright (c) 2026 ReRAC contributors

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## Lombyte

[Lombyte](https://github.com/lombyte-project/Lombyte), the US decompilation, now
`games/rac1/ntsc-decomp`:

- `editor/moby_classes.tsv`: the moby class names the editor shows, from its
  `config/overlays/us/names/level-NN.json` ("moby-class-record" evidence),
  which joins each level's class dispatch table with the class names of
  [Wrench](https://github.com/chaoticgd/wrench)'s moby class unpack.

MIT License

Copyright (c) 2026 Mateusz Kłysz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Wrench and Replanetizer

Consulted for what formats and fields mean; no code from them is included
([editor/README.md](editor/README.md#credits)):
[Wrench](https://github.com/chaoticgd/wrench) (GPL-3.0-or-later) and
[Replanetizer](https://github.com/RatchetModding/Replanetizer).

The editor's reading of Going Commando's, Up Your Arsenal's and Deadlocked's
levels (editor/wads.py, editor/games.py, the per-game paths in the readers)
follows Wrench's `src/iso/table_of_contents.cpp`, `wad_identifier.cpp`,
`src/wrenchbuild/level/`, `src/engine/` and `src/instancemgr/` (read at
commit `46e8043`), re-implemented in Python; Deadlocked's pixel order
(editor/level.py `unswizzle`) is its `map_pixel_index_rac4`
(`src/core/texture.cpp`) written out again. The later games' level names
in editor/games.py and launcher/src/lib/labels.ts are those of its
`data/underlay/{gc,uya,dl}_levels.asset`. Wrench is GPL-3.0-or-later, as
OpenRAC's top level is.

## The native port's libraries

The port ([port/](port/README.md)) builds with these libraries. CMake fetches
each at a pinned version ([port/cmake/Dependencies.cmake](port/cmake/Dependencies.cmake),
[port/cmake/Luau.cmake](port/cmake/Luau.cmake)); none is copied into OpenRAC.

| Library | Version | License | Used by |
|---|---|---|---|
| [SDL](https://github.com/libsdl-org/SDL) | 3.4.18 (Android: 3.2.24) | zlib | the platform layer: window, OpenGL context, input, audio |
| [cgltf](https://github.com/jkuhlmann/cgltf) | 1.15 | MIT | the level viewer: reading the editor's glTF meshes |
| [stb_image](https://github.com/nothings/stb) | commit 2c980bb | MIT or public domain | the level viewer: reading textures (PNG) |
| [xxHash](https://github.com/Cyan4973/xxHash) | 0.8.2 | BSD-2-Clause | PCSX2 texture replacement keys |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.9b | MIT | the games' debug overlay ([docs/port/DEBUGGING.md](docs/port/DEBUGGING.md)) |
| [nanosvg](https://github.com/memononen/nanosvg) | commit 239e102 | zlib | the on-screen pad's action icons: reading and drawing their SVGs ([port/renderer/ui/touch_icons.h](port/renderer/ui/touch_icons.h)) |
| [Luau](https://github.com/luau-lang/luau) | 0.741 (commit 2d88f9b) | MIT | script mods: its virtual machine and compiler ([docs/port/MODS.md](docs/port/MODS.md), section 11; [port/cmake/Luau.cmake](port/cmake/Luau.cmake)) |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | vulkan-sdk-1.4.350.1 | Apache-2.0 or MIT | the Vulkan renderer |
| [volk](https://github.com/zeux/volk) | vulkan-sdk-1.4.350.1 | MIT | the Vulkan renderer: the Vulkan loader, opened at run time |
| [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) | 3.3.0 | MIT | the Vulkan renderer: its memory |
| [glslang](https://github.com/KhronosGroup/glslang) | vulkan-sdk-1.4.350.1 | BSD-3-Clause and others (its LICENSE.txt) | the Vulkan renderer: compiling its shaders |
| [AMD FidelityFX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/tree/c6efa6bf7f2027b3ec94f28578bb5965eabb9e55) | 1.1.4, FSR 3.1.4 | MIT | enhanced Vulkan temporal upscaling; original GLSL, upscaler host code and Vulkan backend, prepared by `port/enhanced/fsr3/prepare_sdk.py` |
| [Arm Accuracy Super Resolution](https://github.com/arm/accuracy-super-resolution-generic-library/tree/0501f490bd9946a2e1806b5363d7ab8a9a6a5e0a) | 25.06 | MIT | enhanced Vulkan temporal upscaling on tile-based GPUs; its host code, Vulkan backend and prebuilt SPIR-V shaders, built by `port/cmake/ArmAsr.cmake` (off with `OPENRAC_ARM_ASR=OFF`) |
| [MoltenVK](https://github.com/KhronosGroup/MoltenVK) | 1.4.2 (Homebrew's `molten-vk`, or the Vulkan SDK's) | Apache-2.0 | the Vulkan renderer on macOS: Vulkan on Metal. Not fetched: its library is taken from the build machine and carried in each game's application bundle (`Contents/Frameworks/libMoltenVK.dylib`, its license in `Contents/Resources/MoltenVK-LICENSE.txt`; [port/cmake/MacApp.cmake](port/cmake/MacApp.cmake)) |

### xxHash license

Copyright (C) 2012-2021 Yann Collet

BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following disclaimer
     in the documentation and/or other materials provided with the
     distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### AMD FidelityFX SDK licence

Copyright (C) 2024 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

### Arm Accuracy Super Resolution licence

Copyright © 2023 Advanced Micro Devices, Inc.
Copyright © 2024-2025 Arm Limited.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

The renderer's conventions (colour 0x80 as 1.0, reversed depth, the double
draw for alpha-test fail modes, textures identified by the game's base
pointer) follow OpenGOAL's renderer as
[docs/port/OPENGOAL_NOTES.md](docs/port/OPENGOAL_NOTES.md) describes it; no
code from OpenGOAL or from any emulator is in the port.

## Tracy

Optional development builds (`OPENRAC_TRACY=ON`) link the Tracy profiler client,
version 0.14.1 at commit `30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c` from
[wolfpld/tracy](https://github.com/wolfpld/tracy). Ordinary builds do not fetch or
link it. Tracy is licensed under the 3-clause BSD license, Copyright (c)
2017-2026 Bartosz Taudul. Its [license](https://github.com/wolfpld/tracy/blob/30997d5ca6bb632cc10807a1da8a6d3de0aeeb3c/LICENSE)
applies to the client and Vulkan profiling headers. Binary redistributions of
profiling builds must include that license. OpenRAC's integration is in
`port/common/profiler.*` and `port/renderer/vulkan/tracy.cpp`.

## SphynxMotionBlurToolkit jump-flood velocity dilation

`port/enhanced/fsr3/shaders/blur_jump.frag` adapts the source-coordinate propagation,
velocity-space fitness and depth selection from
[jfp_simple.glsl](https://github.com/sphynx-owner/JFA_driven_motion_blur_demo/blob/d0c16fc1581eb757e993ff6eaf5877078c8d24cd/addons/SphynxMotionBlurToolkit/JumpFlood/ShaderFiles/jfp_simple.glsl),
by sphynx-owner, pinned at `d0c16fc1581eb757e993ff6eaf5877078c8d24cd`.
OpenRAC uses full render-resolution seeds, explicit invalid entries, retained centre
candidates, power-of-two jumps and reversed-depth selection. Godot integration, demo
assets, the experimental history filter and third-party shader helper snippets are
not copied. The adaptation is under the upstream MIT licence; the surrounding
renderer remains GPL-3.0-or-later. The notice below must accompany distributions.

MIT License

Copyright (c) 2024 sphynx-owner

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Python on iOS

The iOS application embeds CPython 3.14 through
[BeeWare's Python Apple Support 3.14-b14](https://github.com/beeware/Python-Apple-support/releases/tag/3.14-b14)
to run OpenRAC's existing local disc exporter. It uses the public Python embedding API;
the game remains native C/C++. Python's PSF license and its bundled-library notices are
carried in `python/lib/python3.14/LICENSE.txt` inside the application. No game assets or
user-supplied Python code are bundled or executed by the importer. The extension-framework
layout follows the [CPython iOS documentation](https://docs.python.org/3.14/using/ios.html).

## AgX colour conversion matrices

`port/enhanced/renderer/shaders/agx.frag` adapts the AgX colour conversion matrices from
[Google Filament's ToneMapper.cpp](https://github.com/google/filament/blob/be858f42ab95cf0ba6e224ac2dc3950bdb172080/filament/src/ToneMapper.cpp),
pinned at `be858f42ab95cf0ba6e224ac2dc3950bdb172080`. Copyright (C) 2021 The Android Open Source
Project, licensed under Apache-2.0. The shader retains that notice; distributions must include
[`agx-LICENSE.txt`](port/enhanced/renderer/agx-LICENSE.txt).

## Probe texture codecs

The enhanced baker uses Arm astc-encoder 5.4.0 at
`3ee54d103ca66a5b1f751ad49a4b8064235e2cf9` (Apache-2.0) and AMD Compressonator
at `9da0bc2be8d095adfab04ba9262e94842b48dac2` (MIT).
Only their portable HDR codec sources are built. The ASTC border fitter uses the pinned
symbolic block interface; codec sources are fetched by `port/cmake/ProbeCompression.cmake`.
Compressonator's checksum-verified source archive is extracted only for the BC6H kernel,
seven supporting headers and its core licence; its Git history and other tools are omitted.
See [Arm's license](port/enhanced/lighting/astc-LICENSE.txt) and
[AMD's license](port/enhanced/lighting/compressonator-LICENSE.txt).

The Compressonator build copy retains the high blue-Y endpoint bit when decoding BC6H mode 5.
The upstream source and licence remain intact; a Vulkan comparison covers the corrected mode.

## LZ4 bake asset compression

Optional lossless compression of baked GI and reflection assets uses LZ4 1.10.0,
including its bundled xxHash implementation, under the BSD-2-Clause license.
LZ4 is copyright (c) 2011-2020 Yann Collet; the bundled xxHash source is copyright
(C) 2012-2016 Yann Collet.

`port/cmake/BakeLz4.cmake` fetches the checksum-verified upstream archive and builds
the library sources when `OPENRAC_BAKE_LZ4` is enabled. It copies the library license
to `<build>/licenses/lz4/LICENSE.txt` for distribution with the binaries.
The source files retain their copyright and license notices.
See the [LZ4 library license](https://github.com/lz4/lz4/blob/v1.10.0/lib/LICENSE)
and the [bundled xxHash notice](https://github.com/lz4/lz4/blob/v1.10.0/lib/xxhash.c).
