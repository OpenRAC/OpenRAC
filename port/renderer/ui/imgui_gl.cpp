// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What ImGui's OpenGL 3 backend (backends/imgui_impl_opengl3.cpp; MIT License, Copyright (c)
// 2014-2025 Omar Cornut) does, with the calls renderer/gl/gl.h has: one program, one vertex array,
// the font's texture, a scissor per command.

#include "renderer/ui/imgui_gl.h"

#include <algorithm>
#include <cstdint>
#include <imgui.h>
#include <span>
#include <vector>

#include "renderer/frame/colour_transfer.h"
#include "renderer/frame/raster_pass.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/renderer.h"
#include "renderer/ui/imgui_gl_sources.h"

namespace openrac::renderer::ui::imgui_gl {
namespace {

using namespace openrac::gl;

using shader_sources::renderer_ui_imgui_gl::kVertex;

using shader_sources::renderer_ui_imgui_gl::kFragment;

struct State {
    renderer::Shader shader;
    GLuint vao = 0;
    GLuint vertices = 0;
    GLuint indices = 0;
    GLuint font = 0;
};

State* s = nullptr;

// ImGui can start a new 16-bit index range within the same vertex buffer.
void set_vertex_offset(unsigned first) {
    constexpr auto stride = static_cast<GLsizei>(sizeof(ImDrawVert));
    const auto base = static_cast<std::uintptr_t>(first) * sizeof(ImDrawVert);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(base + offsetof(ImDrawVert, pos))
    );
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(base + offsetof(ImDrawVert, uv))
    );
    glVertexAttribPointer(
        2,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<const void*>(base + offsetof(ImDrawVert, col))
    );
}

void set_state(const ImDrawData* data, int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    s->shader.use();
    // Display points to device coordinates, y down.
    const float l = data->DisplayPos.x;
    const float r = data->DisplayPos.x + data->DisplaySize.x;
    const float t = data->DisplayPos.y;
    const float b = data->DisplayPos.y + data->DisplaySize.y;
    const float projection[16] = {
        2.0f / (r - l), 0.0f, 0.0f, 0.0f, 0.0f, 2.0f / (t - b), 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        (r + l) / (l - r), (t + b) / (b - t), 0.0f, 1.0f,
    };
    glUniformMatrix4fv(s->shader.uniform("projection"), 1, GL_FALSE, projection);
    glUniform1i(s->shader.uniform("tex_T0"), 0);
    glBindVertexArray(s->vao);
    glBindBuffer(GL_ARRAY_BUFFER, s->vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->indices);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glActiveTexture(GL_TEXTURE0);
    glBindSampler(0, 0);
}

// The font atlas as ImGui has it now, into a texture of its own.
void upload_font() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    glGenTextures(1, &s->font);
    glActiveTexture(GL_TEXTURE0);
    constexpr std::size_t kChannels = 4;
    upload_rgba8(
        s->font, width, height, {pixels, static_cast<std::size_t>(width) * height * kChannels}
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    io.Fonts->SetTexID(static_cast<ImTextureID>(s->font));
}

// Records a consecutive run of draw commands from the uploaded ImGui list.
void draw_commands(
    std::span<const ImDrawCmd> commands,
    const ImDrawData& data,
    const RenderTarget& target,
    std::span<const RasterBuffer> geometry
) {
    if (std::ranges::none_of(commands, [](const ImDrawCmd& cmd) { return cmd.ElemCount != 0; })) {
        return;
    }
    std::vector<RasterTexture> textures;
    for (const ImDrawCmd& command : commands) {
        const auto name = static_cast<unsigned>(command.GetTexID());
        if (name && std::ranges::find(textures, name, &RasterTexture::name) == textures.end()) {
            textures.push_back({name});
        }
    }
    const ImVec2 origin = data.DisplayPos;
    const ImVec2 scale = data.FramebufferScale;
    constexpr GLenum kIndexType = sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    raster_pass("ImGui overlay", target, {geometry, textures}, [&] {
        unsigned vertex_offset = commands.front().VtxOffset;
        set_vertex_offset(vertex_offset);
        for (const ImDrawCmd& command : commands) {
            const float x0 = (command.ClipRect.x - origin.x) * scale.x;
            const float y0 = (command.ClipRect.y - origin.y) * scale.y;
            const float x1 = (command.ClipRect.z - origin.x) * scale.x;
            const float y1 = (command.ClipRect.w - origin.y) * scale.y;
            if (x1 <= x0 || y1 <= y0 || command.ElemCount == 0) {
                continue;
            }
            // Commands using the same index range share vertex state.
            if (vertex_offset != command.VtxOffset) {
                vertex_offset = command.VtxOffset;
                set_vertex_offset(vertex_offset);
            }
            glScissor(
                static_cast<GLint>(x0),
                static_cast<GLint>(static_cast<float>(target.height) - y1),
                static_cast<GLsizei>(x1 - x0),
                static_cast<GLsizei>(y1 - y0)
            );
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(command.GetTexID()));
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(command.ElemCount),
                kIndexType,
                reinterpret_cast<const void*>(
                    static_cast<std::uintptr_t>(command.IdxOffset) * sizeof(ImDrawIdx)
                )
            );
        }
    });
}

}  // namespace

bool init(std::string& error) {
    if (s != nullptr) {
        return true;
    }
    s = new State;
    if (!s->shader.build("imgui", kVertex, kFragment, error)) {
        delete s;
        s = nullptr;
        return false;
    }
    glGenVertexArrays(1, &s->vao);
    glGenBuffers(1, &s->vertices);
    glGenBuffers(1, &s->indices);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "openrac";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    upload_font();
    return true;
}

void reload_font() {
    if (s == nullptr) {
        return;
    }
    glDeleteTextures(1, &s->font);
    s->font = 0;
    upload_font();
}

void render(const ImDrawData* data) {
    if (s == nullptr || data == nullptr) {
        return;
    }
    const int width = static_cast<int>(data->DisplaySize.x * data->FramebufferScale.x);
    const int height = static_cast<int>(data->DisplaySize.y * data->FramebufferScale.y);
    if (width <= 0 || height <= 0) {
        return;
    }
    set_state(data, width, height);
    const RenderTarget target{0, width, height};
    for (const ImDrawList* list : data->CmdLists) {
        const std::size_t vertex_bytes =
            static_cast<std::size_t>(list->VtxBuffer.Size) * sizeof(ImDrawVert);
        const std::size_t index_bytes =
            static_cast<std::size_t>(list->IdxBuffer.Size) * sizeof(ImDrawIdx);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertex_bytes),
            list->VtxBuffer.Data,
            GL_STREAM_DRAW
        );
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(index_bytes),
            list->IdxBuffer.Data,
            GL_STREAM_DRAW
        );
        const RasterBuffer geometry[]{{s->vertices, vertex_bytes}, {s->indices, index_bytes}};
        const std::span commands(
            list->CmdBuffer.Data, static_cast<std::size_t>(list->CmdBuffer.Size)
        );
        std::size_t first = 0;
        // User callbacks may change bindings or issue external GPU work between owned draws.
        for (std::size_t index = 0; index < commands.size(); ++index) {
            const ImDrawCmd& command = commands[index];
            if (!command.UserCallback) {
                continue;
            }
            draw_commands(commands.subspan(first, index - first), *data, target, geometry);
            if (command.UserCallback == ImDrawCallback_ResetRenderState) {
                set_state(data, width, height);
            } else {
                command.UserCallback(list, &command);
            }
            first = index + 1;
        }
        draw_commands(commands.subspan(first), *data, target, geometry);
    }
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void shutdown() {
    if (s == nullptr) {
        return;
    }
    s->shader.release();
    glDeleteVertexArrays(1, &s->vao);
    glDeleteBuffers(1, &s->vertices);
    glDeleteBuffers(1, &s->indices);
    glDeleteTextures(1, &s->font);
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetTexID(0);
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendRendererName = nullptr;
    delete s;
    s = nullptr;
}

}  // namespace openrac::renderer::ui::imgui_gl
