// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Error shader lookup is independent of glslang and writable development caches.
#include "renderer/vulkan/pipelines/error_shader.h"

#include <algorithm>
#include <utility>

#include "renderer/vulkan/pipelines/error_shader_sources.h"
#include "renderer/vulkan/pipelines/spirv_bindings.h"

namespace openrac::vkgl {
namespace {
// Matches the raw diagnostic recipe rather than the missing program's source key.
ShaderKey generic_error_key() {
    return shader_key(
        "program", {error_shader_sources::kVertex, error_shader_sources::kFragment, {}}
    );
}
}  // namespace

bool error_shader_available() {
    return !builtin_shader_package().find(generic_error_key()).empty();
}

bool load_error_shader(std::string_view vertex, CompiledProgram& out) {
    CompiledProgram generic;

    if (!decode_program(builtin_shader_package().find(generic_error_key()), generic)) {
        return false;
    }

    if (decode_program(builtin_shader_package().find(shader_key("error vertex", {vertex})), out)) {
        out.fragment = std::move(generic.fragment);
        const auto bindings = spirv_set0_bindings(out.vertex);
        const auto active = [&](int binding) {
            return binding >= 0
                   && std::find(
                          bindings.begin(), bindings.end(), static_cast<std::uint32_t>(binding)
                      ) != bindings.end();
        };

        // Keep vertex offsets and CPU uniform locations, but never consume the removed stage's
        // textures or buffers. Graph capture and descriptors both follow these active bindings.
        if (!active(out.default_binding)) {
            out.default_binding = -1;
        }

        for (auto& uniform : out.uniforms) {
            if (uniform.sampler && !active(uniform.binding)) {
                uniform.binding = -1;
            }
        }

        std::erase_if(out.blocks, [&](const auto& block) { return !active(block.binding); });
        std::erase_if(out.storage, [&](const auto& block) { return !active(block.binding); });
        out.fallback = GraphicsFallback::Vertex;
        return true;
    }

    out = std::move(generic);
    out.fallback = GraphicsFallback::Fullscreen;
    return true;
}
}  // namespace openrac::vkgl
