// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/gl/shader.h"

#include <algorithm>
#include <format>
#include <span>
#include <utility>
#include <vector>

#include "renderer/gl/gl.h"
#include "renderer/gl/gl_warmup.h"
#include "renderer/gl/program_cache.h"
#include "renderer/gl/shader_modes.h"
#include "renderer/gl/shader_precision.h"
#include "renderer/gl/shader_variants.h"
#include "renderer/textures/texture_filter.h"
#include "renderer_shaders.h"

namespace openrac::renderer {

using namespace openrac::gl;

namespace {

constexpr int kSamplerUnits = 8;

}  // namespace

std::string shader_constants(std::string source, std::initializer_list<ShaderIntConstant> values) {
    for (const auto& [name, value] : values) {
        const std::string declaration = "uniform int " + std::string(name) + ";";
        const std::string constant =
            "const int " + std::string(name) + " = " + std::to_string(value) + ";";
        for (auto at = source.find(declaration); at != std::string::npos;
             at = source.find(declaration, at + constant.size())) {
            source.replace(at, declaration.size(), constant);
        }
    }
    return source;
}

std::string es_source(std::string_view source, bool noperspective, bool clip_distance) {
    // The version line, and the first line after it that is not a comment, are where the ES
    // preamble goes; a source without "#version 410 core" first is given as it is.
    constexpr std::string_view kDesktop = "#version 410 core";
    if (source.substr(0, kDesktop.size()) != kDesktop) {
        return std::string(source);
    }
    std::string body(source.substr(kDesktop.size()));
    const auto replace_all = [&body](std::string_view from, std::string_view to) {
        for (std::size_t at = body.find(from); at != std::string::npos;
             at = body.find(from, at + to.size())) {
            body.replace(at, from.size(), to);
        }
    };
    std::string out = "#version 320 es\n";
    if (noperspective) {
        out += "#extension GL_NV_shader_noperspective_interpolation : require\n";
    } else {
        // Interpolated with perspective instead: Gouraud colours and fog differ a little across a
        // large triangle seen at an angle, and nothing else.
        replace_all("noperspective ", "");
    }
    if (body.find("gl_ClipDistance") != std::string::npos) {
        if (clip_distance) {
            out += "#extension GL_EXT_clip_cull_distance : require\n";
        } else {
            // No user clip planes: the distances are written to a variable nobody reads, and the
            // renderer leaves the planes off (gl::has_extension).
            replace_all("gl_ClipDistance", "openrac_clip_distance");
            out += "float openrac_clip_distance[8];\n";
        }
    }
    // ES has no default precision for floats in a fragment shader or for the sampler types beyond
    // sampler2D and samplerCube; the desktop's is high everywhere.
    out += "precision highp float;\nprecision highp int;\nprecision highp sampler2D;\n"
           "precision highp sampler2DArray;\nprecision highp usampler2D;\nprecision highp "
           "isampler2D;\n"
           "precision highp samplerBuffer;\nprecision highp usamplerBuffer;\nprecision highp "
           "isamplerBuffer;\n";
    // Keep the source's own line numbers in compiler logs.
    out += "#line 1\n";
    out += body;
    return out;
}

namespace {

// The source handed to the driver for a stage written for the desktop's GL: in ES's words on an ES
// context, with the enhanced rendering's switch, and the precision aliases.
std::string stage_source(std::string_view desktop_source) {
    std::string translated;
    std::string_view source = desktop_source;
    if (is_es()) {
        translated = es_source(
            desktop_source,
            has_extension("GL_NV_shader_noperspective_interpolation"),
            has_extension("GL_EXT_clip_cull_distance")
        );
        source = translated;
    }
    // Give shader stages the same single opt-in as the C++ resource gates.
    if (enhanced_rendering_available()) {
        translated = source;
        translated.insert(translated.find('\n') + 1, "#define OPENRAC_ENHANCED_RENDERING 1\n");
        source = translated;
    }
    // Desktop GL and ES retain the same arithmetic sources with ordinary FP32 aliases.
    if (!is_vulkan()) {
        return renderer::shader_precision_source(source, false);
    }
    return std::string(source);
}

struct ShaderStage {
    GLuint object;
    GLenum type;
};

// GL can compile stages in parallel before linking. Vulkan parses them during program linking.
ShaderStage create_stage(GLenum type, const std::string& source) {
    const GLuint shader = glCreateShader(type);
    const GLchar* text = source.data();
    const auto length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &text, &length);
    if (!is_vulkan()) {
        glCompileShader(shader);
    }
    return {shader, type};
}

// Waits for the stage and reports its compiler diagnostics on failure.
bool check_stage(ShaderStage stage, std::string_view name, std::string& error) {
    GLint ok = 0;
    glGetShaderiv(stage.object, GL_COMPILE_STATUS, &ok);
    if (ok != 0) {
        return true;
    }

    GLint size = 0;
    glGetShaderiv(stage.object, GL_INFO_LOG_LENGTH, &size);
    std::string log(static_cast<std::size_t>(std::max(size, 1)), '\0');
    glGetShaderInfoLog(stage.object, size, nullptr, log.data());
    const char* label = stage.type == GL_VERTEX_SHADER     ? "vertex shader"
                        : stage.type == GL_FRAGMENT_SHADER ? "fragment shader"
                                                           : "compute shader";
    error = std::format("{}: {}: {}", name, label, log);
    return false;
}

// Owns these stages until compilation and linking have finished.
GLuint build_stages(
    std::string_view name,
    std::string_view step,
    std::span<const ShaderStage> stages,
    std::string& error
) {
    struct ReleaseStages {
        std::span<const ShaderStage> stages;

        ~ReleaseStages() {
            for (const auto& stage : stages) {
                glDeleteShader(stage.object);
            }
        }
    } release{stages};

    const bool vulkan = is_vulkan();
    if (!vulkan && !std::ranges::all_of(stages, [&](auto stage) {
            return check_stage(stage, name, error);
        })) {
        return 0;
    }
    const GLuint program = glCreateProgram();
    program_cache::prepare(program);

    for (const auto& stage : stages) {
        glAttachShader(program, stage.object);
    }

    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);

    if (ok == 0) {
        GLint size = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        std::string log(static_cast<std::size_t>(std::max(size, 1)), '\0');
        glGetProgramInfoLog(program, size, nullptr, log.data());
        glDeleteProgram(program);
        error = std::format("{}: {}: {}", name, step, log);
        // Recover the stage-specific error when a Vulkan link failed to parse its source.
        if (vulkan) {
            for (const auto& stage : stages) {
                glCompileShader(stage.object);
                if (!check_stage(stage, name, error)) {
                    break;
                }
            }
        }
    }

    return ok != 0 ? program : 0;
}

// Compiles and links a graphics program from final stage sources, or returns 0 with an error.
GLuint compile_program(
    std::string_view name,
    const std::string& vertex,
    const std::string& fragment,
    std::string& error
) {
    const ShaderStage stages[] =
        {create_stage(GL_VERTEX_SHADER, vertex), create_stage(GL_FRAGMENT_SHADER, fragment)};
    return build_stages(name, "link", stages, error);
}

// Lists declared feature modes; Vulkan and MoltenVK select them in their pipeline backend.
std::vector<std::string> feature_modes(const std::string& vertex, const std::string& fragment) {
    std::vector<std::string> modes;

    if (!is_vulkan()) {
        shader_modes::append_names(vertex, modes);
        shader_modes::append_names(fragment, modes);
        std::erase_if(modes, [&](const std::string& mode) {
            return shader_modes::declaration(vertex, mode).first == std::string::npos
                   && shader_modes::declaration(fragment, mode).first == std::string::npos;
        });
    }

    return modes;
}

// Loads an exact native binary or compiles its identical sources through the supplied linker.
template <typename Link>
GLuint cached_program(const program_cache::Key& key, Link link, std::string& error) {
    GLuint program = program_cache::load(key);

    // Rejected or missing binaries are rebuilt without changing feature values.
    if (program == 0) {
        program = link(error);

        if (program == 0) {
            return 0;
        }

        program_cache::store(key, program);
    }

    gl_warmup::note_program(program, key);
    return program;
}

// Owns graphics templates so later feature combinations cannot outlive their source strings.
shader_variants::Compiler graphics_variants(
    std::string name, std::string vertex, std::string fragment, std::vector<std::string> modes
) {
    return [name = std::move(name),
            vertex = std::move(vertex),
            fragment = std::move(fragment),
            modes = std::move(modes)](const std::vector<int>& values, std::string& error) {
        const auto vs = shader_variants::specialise(vertex, modes, values);
        const auto fs = shader_variants::specialise(fragment, modes, values);
        return cached_program(
            program_cache::key(vs, fs),
            [&](std::string& why) { return compile_program(name, vs, fs, why); },
            error
        );
    };
}

// Gives native compute GLSL its ES version and precision aliases before feature replacement.
std::string compute_source(std::string_view source) {
    std::string es;

    // GLSL ES compute starts at version 310, independently of desktop graphics' 410 baseline.
    if (is_es()) {
        es = "#version 310 es\nprecision highp float;\nprecision highp int;\n"
             "precision highp sampler2D;\n";
        es += source.substr(source.find('\n') + 1);
        source = es;
    }

    return stage_source(source);
}

// Links one native compute executable from final, already specialised stage source.
GLuint compile_compute(std::string_view name, const std::string& source, std::string& error) {
    const auto stage = create_stage(GL_COMPUTE_SHADER, source);
    return build_stages(name, "compute link", std::span(&stage, 1), error);
}

// Owns compute templates and caches each exact set of feature constants independently.
shader_variants::Compiler compute_variants(
    std::string name, std::string source, std::vector<std::string> modes
) {
    return [name = std::move(name),
            source = std::move(source),
            modes = std::move(modes)](const std::vector<int>& values, std::string& error) {
        const auto fixed = shader_variants::specialise(source, modes, values);
        return cached_program(
            program_cache::key(fixed, "OpenRAC compute variant v1"),
            [&](std::string& why) { return compile_compute(name, fixed, why); },
            error
        );
    };
}

// Sets the renderer's sampler contract through logical locations, including inactive samplers.
void initialise_samplers(GLuint program) {
    glUseProgram(program);

    for (int unit = 0; unit < kSamplerUnits; ++unit) {
        const std::string sampler = std::format("tex_T{}", unit);
        const GLint location = glGetUniformLocation(program, sampler.c_str());

        if (location >= 0) {
            glUniform1i(location, unit);
        }
    }

    glUniform1i(
        glGetUniformLocation(program, "upscaled_texture_colours"), kUpscaledTextureColoursUnit
    );
}

}  // namespace

Shader::~Shader() {
    // The program belongs to a context that may be gone by now; release()
    // frees it while the context is current.
}

Shader::Shader(Shader&& other) noexcept
    : m_program(std::exchange(other.m_program, 0)),
      m_locations(std::move(other.m_locations)) {}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, 0);
        m_locations = std::move(other.m_locations);
        other.m_locations.clear();
    }
    return *this;
}

bool Shader::build(
    std::string_view name, std::string_view vertex, std::string_view fragment, std::string& error
) {
    // Share exactly the same RGB-only reconstruction across world shader paths.
    constexpr std::string_view marker = "// OPENRAC_UPSCALED_TEXTURE_COLOURS_SOURCE";
    std::string assembled;
    if (const auto at = fragment.find(marker); at != std::string_view::npos) {
        assembled = fragment;
        assembled.replace(
            at,
            marker.size(),
            enhanced_rendering_available()
                ? shaders::upscaled_texture_colours_glsl
                : "vec4 sample_upscaled_texture_colours(vec2 uv) { return texture(tex_T0, uv); }\n"
        );
        fragment = assembled;
    }
    // The same sources as a program built before (this run, or an earlier one): the driver's
    // binary of it, without a compile (renderer/gl/program_cache.h).
    const std::string vertex_source = stage_source(vertex);
    const std::string fragment_source = stage_source(fragment);
    const auto modes = feature_modes(vertex_source, fragment_source);
    const auto make = graphics_variants(std::string(name), vertex_source, fragment_source, modes);

    const GLuint program = make(std::vector<int>(modes.size(), 0), error);

    // Initial zero-valued modes are an exact variant, never a generic executable.
    if (program == 0) {
        return false;
    }

    release();
    m_program = program;
    m_locations.clear();

    // Only programs with feature modes need logical uniform locations and selection hooks.
    if (!modes.empty()) {
        shader_variants::register_program(
            program, vertex_source + '\n' + fragment_source, modes, make
        );
    }

    initialise_samplers(m_program);
    return true;
}

bool Shader::build_compute(std::string_view name, std::string_view source, std::string& error) {
    const auto final_source = compute_source(source);
    const auto modes = feature_modes(final_source, {});
    const auto make = compute_variants(std::string(name), final_source, modes);
    const GLuint program = make(std::vector<int>(modes.size(), 0), error);

    // A failed exact link cannot become the Shader's executable.
    if (program == 0) {
        return false;
    }

    release();
    m_program = program;

    // Compute programs with feature modes use the same stable locations as graphics programs.
    if (!modes.empty()) {
        shader_variants::register_program(program, final_source, modes, make);
    }

    return true;
}

void Shader::use() const {
    glUseProgram(m_program);
}

int Shader::uniform(const char* name) const {
    const auto found = m_locations.find(name);
    if (found != m_locations.end() && found->second.name == name) {
        return found->second.location;
    }
    const int location = glGetUniformLocation(m_program, name);
    m_locations[name] = Location{name, location};
    return location;
}

void Shader::release() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_locations.clear();
}

}  // namespace openrac::renderer
