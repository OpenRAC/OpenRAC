// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include "renderer/framebuffer.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "tests/check.h"

namespace openrac::test {
// Observe the real prepass without changing its geometry, then read its completed attachments.
class PrepassCapture final : public renderer::BucketRenderer {
public:
    PrepassCapture() : BucketRenderer("prepass comparison", renderer::Bucket::Particles) {}

    bool init(renderer::RenderState&, std::string& error) override {
        gl::glGenVertexArrays(1, &m_vao);
        return m_shader.build(
            "prepass attachment readback",
            R"(#version 430 core
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0,1);})",
            R"(#version 430 core
uniform sampler2D tex_T0;
uniform int depth_bits;
layout(location=0) out vec4 result;
void main(){vec4 v=texelFetch(tex_T0,ivec2(gl_FragCoord.xy),0);
if(depth_bits!=0){uint b=floatBitsToUint(v.r);v=vec4(float(b&255u),float((b>>8)&255u),float((b>>16)&255u),float(b>>24))/255.;}
result=v;})",
            error
        );
    }

    void render_depth(const renderer::FrameInput&, renderer::RenderState& state) override {
        m_normal = state.target.normal_texture;
        m_depth = state.target.depth_texture;
    }

    void render(const renderer::FrameInput&, renderer::RenderState& state) override {
        const auto& target = state.target;
        if (m_output.width() != target.width || m_output.height() != target.height) {
            std::string error;
            CHECK(m_output.create(target.width, target.height, error));
        }
        const auto read = [&](unsigned texture, bool depth) {
            if (!texture) {
                return std::vector<std::uint8_t>{};
            }
            gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, m_output.id());
            gl::glViewport(0, 0, target.width, target.height);
            gl::glDisable(gl::GL_DEPTH_TEST);
            gl::glDisable(gl::GL_STENCIL_TEST);
            gl::glDisable(gl::GL_BLEND);
            gl::glDisable(gl::GL_CULL_FACE);
            gl::glDisable(gl::GL_SCISSOR_TEST);
            gl::glColorMask(gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
            m_shader.use();
            gl::glUniform1i(m_shader.uniform("tex_T0"), 0);
            gl::glUniform1i(m_shader.uniform("depth_bits"), depth ? 1 : 0);
            gl::glActiveTexture(gl::GL_TEXTURE0);
            gl::glBindTexture(gl::GL_TEXTURE_2D, texture);
            gl::glBindVertexArray(m_vao);
            gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);
            return m_output.read_rgba();
        };
        normals = read(m_normal, false);
        depths = read(m_depth, true);
        gl::glBindVertexArray(0);
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, target.framebuffer);
        gl::glViewport(0, 0, target.width, target.height);
    }

    void release() override {
        m_shader.release();
        m_output.release();
        gl::glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    std::vector<std::uint8_t> normals, depths;

private:
    renderer::FrameBuffer m_output;
    renderer::Shader m_shader;
    unsigned m_vao = 0, m_normal = 0, m_depth = 0;
};
}  // namespace openrac::test
