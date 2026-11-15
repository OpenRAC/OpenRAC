// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <charconv>
#include <string>
#include <string_view>

namespace openrac::renderer {
// Arithmetic only: never apply these aliases to uniforms, stage interfaces or storage fields.
// The caller may request FP16 only on a Vulkan device created with shaderFloat16 enabled.
inline std::string shader_precision_source(std::string_view source, bool float16) {
    if (source.empty() || source.find("OPENRAC_PRECISION_DECLARED") != std::string_view::npos
        || (source.find("mp_float") == std::string_view::npos
            && source.find("mp_vec") == std::string_view::npos)) {
        return std::string(source);
    }
    std::string preamble = "#define OPENRAC_PRECISION_DECLARED 1\n";
    if (float16) {
        preamble += "#define OPENRAC_USE_FP16 1\n"
                    "#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require\n";
    }

    preamble += "#ifdef OPENRAC_USE_FP16\n"
                "#define mp_float float16_t\n#define mp_vec2 f16vec2\n"
                "#define mp_vec3 f16vec3\n#define mp_vec4 f16vec4\n"
                "#else\n#define mp_float float\n#define mp_vec2 vec2\n"
                "#define mp_vec3 vec3\n#define mp_vec4 vec4\n#endif\n";
    std::string result(source);
    const auto version_start = result.find("#version");
    auto version_end = result.find('\n', version_start);
    if (float16 && version_start != std::string::npos) {
        // The pinned glslang exposes half-precision built-in overloads only from desktop GLSL 450.
        // Older versions can silently promote half arguments to float/double despite the extension.
        const auto number_at = result.find_first_of("0123456789", version_start + 8);
        unsigned version = 0;
        if (number_at != std::string::npos) {
            std::from_chars(
                result.data() + number_at,
                result.data() + (version_end == std::string::npos ? result.size() : version_end),
                version
            );
            if (version < 450) {
                result.replace(version_start, version_end - version_start, "#version 450 core");
                version_end = result.find('\n', version_start);
            }
        }
    }
    if (version_start != std::string::npos && version_end == std::string::npos) {
        result += '\n';
        version_end = result.size() - 1;
    }
    result.insert(version_end == std::string::npos ? 0 : version_end + 1, preamble);
    return result;
}
}  // namespace openrac::renderer
