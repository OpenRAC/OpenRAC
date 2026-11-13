// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Included by vkgl_gl.cpp, using the private gl_program.h shader/uniform entry points.
// A fullscreen draw broadcasts a single-sample source into every sample of the target.

// Restores the caller's complete GL state even if shader creation or recording fails.
class BlitState {
public:
    BlitState() : m_saved(g->state) {}

    ~BlitState() { g->state = m_saved; }

    BlitState(const BlitState&) = delete;
    BlitState& operator=(const BlitState&) = delete;

private:
    State m_saved;  // The graphics owner is exclusive throughout this synchronous draw.
};

// Builds one ordinary backend-owned program, reused across framebuffer and sample-count changes.
GLuint make_multisample_blit_program() {
    constexpr const char* kVertex = R"(#version 410 core
uniform vec4 copy_region;
out vec2 uv;
void main() {
    vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
    uv = copy_region.xy + corner * copy_region.zw;
}
)";
    constexpr const char* kFragment = R"(#version 410 core
uniform sampler2D copy_source;
in vec2 uv;
layout(location = 0) out vec4 result;
void main() { result = texture(copy_source, uv); }
)";
    const GLuint program = create_program();
    const std::array<GLuint, 2> shaders{
        create_shader(GL_VERTEX_SHADER), create_shader(GL_FRAGMENT_SHADER)
    };
    shader_source(shaders[0], 1, &kVertex, nullptr);
    shader_source(shaders[1], 1, &kFragment, nullptr);
    for (const GLuint shader : shaders) {
        compile_shader(shader);
        attach_shader(program, shader);
    }
    link_program(program);
    for (const GLuint shader : shaders) {
        delete_shader(shader);
    }
    // Failed internal programs remain owned by the context, but cannot be used for a copy.
    if (!g->programs.find(program)->linked) {
        throw std::runtime_error(g->programs.find(program)->log);
    }
    return program;
}

// Draws a clamped source rectangle into every sample, including scaled or flipped colour copies.
void blit_into_multisample(
    Image& source, Image& destination, const VkImageBlit& region, VkFilter filter
) {
    const BlitState restore;
    end_rendering(PassEnd::Copy);
    const GLuint framebuffer = g->state.draw_framebuffer;
    // These private names use the context's existing deferred resource ownership.
    if (g->multisample_blit_program == 0) {
        const GLuint program = make_multisample_blit_program();
        const GLuint texture = g->textures.add();
        g->multisample_blit_program = program;
        g->multisample_blit_texture = texture;
    }
    auto& texture = *g->textures.find(g->multisample_blit_texture);
    retire(std::move(texture.image));
    texture.image = g->state.read_framebuffer == 0 ? g->default_colour
                                                   : colour_owner(g->state.read_framebuffer);
    texture.params.min = texture.params.mag = filter == VK_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
    texture.params.wrap_s = texture.params.wrap_t = GL_CLAMP_TO_EDGE;
    auto first = region.srcOffsets[0];
    auto last = region.srcOffsets[1];
    // A Vulkan viewport has positive size; reversed destination axes reverse the source UVs.
    if (region.dstOffsets[0].x > region.dstOffsets[1].x) {
        std::swap(first.x, last.x);
    }
    if (region.dstOffsets[0].y > region.dstOffsets[1].y) {
        std::swap(first.y, last.y);
    }
    g->state = State{};
    g->state.draw_framebuffer = framebuffer;
    g->state.program = g->multisample_blit_program;
    g->state.unit_2d[0] = g->multisample_blit_texture;
    g->state.viewport[0] = std::min(region.dstOffsets[0].x, region.dstOffsets[1].x);
    g->state.viewport[1] = std::min(region.dstOffsets[0].y, region.dstOffsets[1].y);
    g->state.viewport[2] = std::abs(region.dstOffsets[1].x - region.dstOffsets[0].x);
    g->state.viewport[3] = std::abs(region.dstOffsets[1].y - region.dstOffsets[0].y);
    uniform_4f(
        get_uniform_location(g->multisample_blit_program, "copy_region"),
        float(first.x) / float(source.width),
        float(first.y) / float(source.height),
        float(last.x - first.x) / float(source.width),
        float(last.y - first.y) / float(source.height)
    );
    draw_arrays(GL_TRIANGLES, 0, 3);
    destination.discarded = 0;
}
