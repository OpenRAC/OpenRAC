// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Replaces marked feature uniforms with pipeline specialization constants across graphics stages.

#pragma once
#include "renderer/gl/shader_modes.h"
#include "renderer/vulkan/pipelines/material_shader_sources.h"
#include "renderer/vulkan/pipelines/shader_specialization.h"

namespace openrac::vkgl::shader_transforms {
struct ModeSources {
    std::string vertex, fragment;
    std::vector<std::string> names;
    std::string error;  // A variant declaration that cannot be represented.
};

// Rewrites marked feature uniforms as constants fixed by every graphics pipeline variant.
// Per-draw material data stays in its material buffer; CPU mode values are placed by place_modes.
inline ModeSources mode_sources(
    const std::string& vertex, const std::string& fragment, bool materials
) {
    ModeSources out;
    std::vector<std::string> marked;
    renderer::shader_modes::append_names(vertex, marked);
    renderer::shader_modes::append_names(fragment, marked);

    if (marked.empty()) {
        return out;
    }

    out.vertex = vertex;
    out.fragment = fragment;

    for (const std::string& name : marked) {
        // Material fields vary within one draw and are not global pipeline features.
        if (materials && material_takes(vertex, name)) {
            continue;
        }

        // Exceeding the key's capacity must not silently leave a feature dynamic.
        if (out.names.size() == detail::kModeConstants) {
            out.error = "too many shader feature modes for a pipeline variant";
            return out;
        }

        const std::string id = std::to_string(detail::kModeConstantBase + out.names.size());
        bool declared = false;

        for (std::string* source : {&out.vertex, &out.fragment}) {
            const auto [from, to] = renderer::shader_modes::declaration(*source, name);

            if (from == std::string::npos) {
                continue;
            }

            // On the declaration's own line, so that the lines after it keep their numbers.
            source->replace(
                from, to - from, "layout(constant_id = " + id + ") const int " + name + " = 0;"
            );
            declared = true;
        }

        if (declared) {
            out.names.push_back(name);
        }
    }

    return out;
}

}  // namespace openrac::vkgl::shader_transforms
