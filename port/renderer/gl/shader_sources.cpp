// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared enhanced preamble and upscaled-colour marker substitution for GL and Vulkan sources.

#include "renderer/gl/shader_sources.h"

#include "renderer_shaders.h"

namespace openrac::renderer {
std::string shader_fragment_source(std::string_view source, bool enhanced) {
    constexpr std::string_view kMarker = "// OPENRAC_UPSCALED_TEXTURE_COLOURS_SOURCE";
    std::string out(source);

    if (const auto at = out.find(kMarker); at != std::string::npos) {
        out.replace(
            at,
            kMarker.size(),
            enhanced
                ? shaders::upscaled_texture_colours_glsl
                : "vec4 sample_upscaled_texture_colours(vec2 uv) { return texture(tex_T0, uv); }\n"
        );
    }

    return out;
}

std::string shader_enhanced_source(std::string_view source, bool enhanced) {
    std::string out(source);

    if (enhanced && !out.empty()) {
        out.insert(out.find('\n') + 1, "#define OPENRAC_ENHANCED_RENDERING 1\n");
    }

    return out;
}
}  // namespace openrac::renderer
