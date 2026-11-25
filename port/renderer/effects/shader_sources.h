// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared effect source assembly for ordinary, tested, fixed and material-lit draws.

#pragma once
#include "renderer/gl/shader.h"
#include "renderer_shaders.h"

namespace openrac::renderer::effect_sources {
inline std::string base_fragment(bool fixed) {
    std::string fragment(shaders::direct_frag);
    fragment.insert(
        fragment.find('\n') + 1,
        "#define OPENRAC_UPSCALED_TEXTURE_COLOURS 1\n#define OPENRAC_EFFECT_FOG 1\n"
    );

    // Effect packets skip HUD distance fields and alpha tests; authored strip fog remains dynamic.
    // Keep texture functions/tcc dynamic: they can differ between effect strips.
    if (fixed) {
        fragment.insert(fragment.find('\n') + 1, "#define OPENRAC_EFFECT_FIXED 1\n");
        fragment = shader_constants(
            std::move(fragment), {{"textured", 1}, {"distance_field", 0}, {"alpha_test", 0}}
        );
    }

    return fragment;
}

inline std::string vertex(std::string_view extension) {
    std::string out(shaders::effect_vert);

    if (!extension.empty()) {
        out.insert(out.find("void main()"), extension);
        out.insert(out.find('\n') + 1, "#define OPENRAC_EFFECT_POSITION 1\n");
    }

    return out;
}

inline std::string fragment(std::string out, std::string_view material) {
    if (!material.empty()) {
        out.insert(out.find("void main()"), material);
        out.insert(out.find('\n') + 1, "#define OPENRAC_EFFECT_MATERIAL 1\n");
    }

    return out;
}

// A shader source with `define` set (inserted after its #version line).
inline std::string with_define(std::string source, std::string_view define) {
    source.insert(source.find('\n') + 1, "#define " + std::string(define) + " 1\n");
    return source;
}

inline std::string untextured() {
    std::string out(shaders::direct_frag);
    out.insert(out.find('\n') + 1, "#define OPENRAC_UPSCALED_TEXTURE_COLOURS 1\n");
    return out;
}
}  // namespace openrac::renderer::effect_sources
