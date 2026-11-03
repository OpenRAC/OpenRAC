// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Private preview graphics resources, consumed only after the game's render worker has finished.
#include "scene/object_preview.h"

#include <algorithm>
#include <cstddef>

#include "renderer/gl.h"
#include "scene_shaders.h"

namespace openrac::scene {
using namespace gl;

namespace {
// Optional lighting is supplied without making the general preview depend on enhanced rendering.
std::string preview_fragment(const PreviewLighting* lighting) {
    std::string source = shaders::object_preview_frag;
    const auto code = lighting ? lighting->source()
                               : "vec3 preview_illumination(vec3 a,vec3 n,vec3 p,vec3 v,float d) "
                                 "{return preview_neutral(a,n);}";
    source.insert(source.find("// PREVIEW_LIGHTING"), code);
    return source;
}
}  // namespace

ObjectPreview::~ObjectPreview() {
    release();
}

Result<void> ObjectPreview::select(PreviewMesh mesh, std::unique_ptr<PreviewLighting> lighting) {
    release();

    // Empty selections must not allocate graphics resources.
    if (mesh.vertices.empty()) {
        return unexpected(Error{"no triangles are available for the object viewer"});
    }

    std::string error;

    if (!m_shader.build(
            "object viewer", shaders::object_preview_vert, preview_fragment(lighting.get()), error
        )) {
        release();
        return unexpected(Error{error});
    }

    m_lighting = std::move(lighting);
    m_mesh = std::move(mesh);
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vertices);
    upload_vertices();
    upload_images();
    return {};
}

void ObjectPreview::upload_vertices() {
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        GLsizeiptr(m_mesh.vertices.size() * sizeof(scene::Vertex)),
        m_mesh.vertices.data(),
        GL_DYNAMIC_DRAW
    );
    constexpr std::array<int, 4> kSizes{3, 3, 2, 4};
    constexpr std::array<std::size_t, 4> kOffsets{
        offsetof(scene::Vertex, position),
        offsetof(scene::Vertex, normal),
        offsetof(scene::Vertex, uv),
        offsetof(scene::Vertex, colour)
    };

    for (unsigned i = 0; i < kSizes.size(); ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(
            i,
            kSizes[i],
            GL_FLOAT,
            GL_FALSE,
            sizeof(scene::Vertex),
            // GL encodes an array-buffer byte offset as a pointer, never a host address.
            reinterpret_cast<const void*>(kOffsets[i])  // NOLINT(performance-no-int-to-ptr)
        );
    }

    glBindVertexArray(0);
}

void ObjectPreview::upload_images() {
    m_images.resize(m_mesh.images.size() + 1);
    glGenTextures(GLsizei(m_images.size()), m_images.data());
    const std::array<std::uint8_t, 4> white{255, 255, 255, 255};
    glActiveTexture(GL_TEXTURE0);

    for (std::size_t i = 0; i < m_images.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, m_images[i]);
        const bool fallback = i == m_mesh.images.size();
        const int width = fallback ? 1 : m_mesh.images[i].width;
        const int height = fallback ? 1 : m_mesh.images[i].height;
        const void* pixels = fallback ? white.data() : m_mesh.images[i].pixels.data();
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels
        );
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    m_mesh.images.clear();  // GPU textures own the snapshot; do not retain another image copy.
}

Result<void> ObjectPreview::pose(
    const scene::Instance& instance, std::span<const renderer::Mat4> palette
) {
    auto result = preview_pose(m_mesh, instance, palette);

    if (result) {
        upload_vertices();
    }

    return result;
}

Result<void> ObjectPreview::bind_pose() {
    auto instance = m_mesh.instance;
    instance.palette =
        -1;  // Unskinned source vertices and bind frames describe the original rest pose.
    return pose(instance, {});
}

void ObjectPreview::draw_parts(bool transparent) {
    glUniform1i(m_shader.uniform("material_transparent"), transparent);

    for (const auto& part : m_mesh.parts) {
        const auto& material = part.material;

        if (transparent ? (!material.blend && !material.cutout) : material.blend) {
            continue;
        }

        for (unsigned unit = 0; unit < 2; ++unit) {
            const int image = unit == 0 ? material.image : material.emission_image;
            const auto index = image >= 0 ? std::size_t(image) : m_images.size() - 1;
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, m_images[index]);
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, material.clamp_s ? GL_CLAMP_TO_EDGE : GL_REPEAT
            );
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, material.clamp_t ? GL_CLAMP_TO_EDGE : GL_REPEAT
            );
        }

        glUniform4fv(m_shader.uniform("material_colour"), 1, material.colour.data());
        glUniform3f(
            m_shader.uniform("material_emission"),
            material.emission[0],
            material.emission[1],
            material.emission[2]
        );
        glUniform1f(m_shader.uniform("material_diffuse_scale"), material.diffuse_scale);
        glUniform1f(m_shader.uniform("material_emission_contrast"), material.emission_contrast);
        glUniform1i(
            m_shader.uniform("material_emission_vertex_colour"), material.emission_vertex_colour
        );
        glUniform1i(m_shader.uniform("material_cutout"), material.cutout);
        glDrawArrays(GL_TRIANGLES, GLint(part.first), GLsizei(part.count));
    }
}

// Bind only private target state; the overlay restores its window target afterwards.
void ObjectPreview::bind_target(const renderer::Camera& camera, int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_target.id());
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(.025f, .03f, .04f, 1);
    glClearDepth(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_shader.use();
    const auto clip = renderer::multiply(camera.projection, camera.view);
    glUniformMatrix4fv(m_shader.uniform("clip"), 1, GL_FALSE, clip.data());
    glUniform3f(
        m_shader.uniform("eye"), camera.position[0], camera.position[1], camera.position[2]
    );
}

// The optional prepass supplies complete Forward+ masks without changing colour/alpha handling.
void ObjectPreview::draw_surfaces(const renderer::Camera& camera, int width, int height) {
    if (m_lighting) {
        m_lighting->bind(m_shader, camera, false);
    }

    if (m_lighting && m_lighting->wants_depth()) {
        glUniform1i(m_shader.uniform("preview_depth_only"), 1);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glBindVertexArray(m_vao);
        draw_parts(false);
        m_lighting->cull(m_target.depth_texture(), camera, width, height);
        m_shader.use();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    glUniform1i(m_shader.uniform("preview_depth_only"), 0);

    if (m_lighting) {
        m_lighting->bind(m_shader, camera, true);
    }

    // Depth culling is optional; opaque and transparent surfaces retain their original alpha split.
    glBindVertexArray(m_vao);
    draw_parts(false);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    if (m_lighting) {
        m_lighting->bind(m_shader, camera, false);
    }

    draw_parts(true);
    glDepthMask(GL_TRUE);
}

// Size resources and draw using either neutral shading or the registered optional provider.
Result<void> ObjectPreview::draw(int width, int height, const PreviewOrbit& orbit) {
    if (!m_vao) {
        return unexpected(Error{"no object is selected in the object viewer"});
    }

    std::string error;
    // Cap memory without changing aspect ratio on high-DPI displays.
    const float resize = std::min(1.f, 1024.f / float(std::max({width, height, 1})));
    width = std::max(64, int(float(width) * resize));
    height = std::max(64, int(float(height) * resize));

    if (m_target.width() != width || m_target.height() != height) {
        if (!m_target.create(width, height, error, 1, true)) {
            return unexpected(Error{error});
        }
    }

    const auto camera = orbit.camera(float(width) / float(height));

    if (m_lighting) {
        m_lighting->prepare(++m_frame);
    }

    bind_target(camera, width, height);
    draw_surfaces(camera, width, height);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return {};
}

void ObjectPreview::release() {
    // An unopened developer window has no graphics objects and needs no current context.
    if (m_vao || m_shader.id()) {
        m_target.release();
        m_shader.release();

        if (m_lighting) {
            m_lighting->release();
        }

        m_lighting.reset();
        glDeleteTextures(GLsizei(m_images.size()), m_images.data());
        glDeleteBuffers(1, &m_vertices);
        glDeleteVertexArrays(1, &m_vao);
    }

    m_images.clear();
    m_mesh = {};
    m_vao = 0;
    m_vertices = 0;
    m_frame = 0;
}
}  // namespace openrac::scene
