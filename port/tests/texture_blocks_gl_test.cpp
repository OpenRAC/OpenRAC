// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A texture pack KTX2 kept on the GPU in blocks (TexturePool::set_blocks): BC7 or ASTC 4x4 where
// the context reads them, RGBA8 where it does not, each mip level of the file drawn as it is.
// Run on OpenGL, ES (texture_blocks_gl_es), the Vulkan renderer (texture_blocks_gl_vulkan) and a
// context whose block formats are hidden (texture_blocks_gl_rgba8). Skipped (exit 77) where no
// context can be made.
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/textures/pack_pictures.h"
#include "renderer/textures/texture_pool.h"
#include "renderer_gl_context.h"

namespace {

using namespace openrac;
using namespace openrac::gl;
using namespace openrac::renderer;
namespace fs = std::filesystem;

const fs::path kData = PACK_PICTURES_DATA;
constexpr int kSide = 8;

constexpr auto kVertex = R"(#version 410 core
out vec2 v_uv;
void main() {
    vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    v_uv = corner;
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr auto kFragment = R"(#version 410 core
uniform sampler2D image;
uniform float lod;
in vec2 v_uv;
out vec4 colour;
void main() {
    colour = textureLod(image, v_uv, lod);
}
)";

std::vector<std::uint8_t> read(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

using Rgba = std::array<int, 4>;

bool near(const Rgba& a, const Rgba& b, int tolerance) {
    for (int c = 0; c < 4; ++c) {
        if (std::abs(a[c] - b[c]) > tolerance) {
            return false;
        }
    }

    return true;
}

// `texture` drawn over the 8x8 target at mip level `lod`: its four corners show the picture's
// four quarters (red, green, blue, see-through) at every level of the chain.
bool draws_quadrants(const Shader& shader, unsigned texture, float lod) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    shader.use();
    glUniform1i(shader.uniform("image"), 0);
    glUniform1f(shader.uniform("lod"), lod);
    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::array<std::uint8_t, kSide * kSide * 4> pixels{};
    glReadPixels(0, 0, kSide, kSide, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const auto at = [&](int x, int y) {
        const std::size_t i = (static_cast<std::size_t>(y) * kSide + x) * 4;
        return Rgba{pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]};
    };
    constexpr int kTolerance = 40;
    const bool ok = near(at(0, 0), {255, 0, 0, 255}, kTolerance)
                    && near(at(7, 0), {0, 255, 0, 255}, kTolerance)
                    && near(at(0, 7), {0, 0, 255, 255}, kTolerance) && at(7, 7)[3] <= kTolerance;

    if (!ok) {
        std::printf(
            "level %.0f: %d,%d,%d,%d %d,%d,%d,%d %d,%d,%d,%d %d,%d,%d,%d\n",
            static_cast<double>(lod),
            at(0, 0)[0],
            at(0, 0)[1],
            at(0, 0)[2],
            at(0, 0)[3],
            at(7, 0)[0],
            at(7, 0)[1],
            at(7, 0)[2],
            at(7, 0)[3],
            at(0, 7)[0],
            at(0, 7)[1],
            at(0, 7)[2],
            at(0, 7)[3],
            at(7, 7)[0],
            at(7, 7)[1],
            at(7, 7)[2],
            at(7, 7)[3]
        );
    }

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    test::GlContext context;
    std::string why;

    if (!context.open(why, kSide * 8, kSide * 8)) {
        std::printf("SKIP: no OpenGL context: %s\n", why.c_str());
        return test::kSkip;
    }

    const BlockFormat format = TexturePool::gpu_block_format();
    const bool blocks = format != BlockFormat::None;
    std::printf(
        "blocks: %s\n",
        format == BlockFormat::Bc7       ? "BC7"
        : format == BlockFormat::Astc4x4 ? "ASTC 4x4"
                                         : "none (RGBA8)"
    );

    // An argument names the format the context must offer (bc7, astc, none), so that a CI driver
    // that loses one is noticed; without one, whatever the context has is tried.
    if (argc > 1) {
        const std::string wanted = argv[1];
        CHECK(
            (wanted == "bc7" && format == BlockFormat::Bc7)
            || (wanted == "astc" && format == BlockFormat::Astc4x4)
            || (wanted == "none" && format == BlockFormat::None)
        );
    }

    std::string error;
    Shader shader;
    CHECK(shader.build("texture blocks", kVertex, kFragment, error));

    if (!error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
    }

    FrameBuffer target;
    CHECK(target.create_colour(kSide, kSide, error));
    glBindFramebuffer(GL_FRAMEBUFFER, target.id());
    glViewport(0, 0, kSide, kSide);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    unsigned vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // A chain made by basisu -mipmap (8x8 down to 1x1), as a world texture (a mip chain) and as
    // a 2D layer's (one level).
    const auto mips =
        pack_pictures::decode_ktx2(read(kData / "quadrants-uastc-mips.ktx2"), format, why);
    CHECK(mips && (mips->blocks != nullptr) == blocks);

    if (!mips) {
        return test::result();
    }

    TexturePool pool;
    const TextureHandle world = pool.add(mips->image, AlphaScale::Full, "world", true);
    pool.set_blocks(world, mips->blocks);
    const TextureHandle flat = pool.add(mips->image, AlphaScale::Gs, "flat");
    pool.set_blocks(flat, mips->blocks);
    const std::array handles{world, flat};
    const PreparedTextures made = pool.prepare(handles);
    CHECK(pool.has_blocks(world) == blocks && pool.has_blocks(flat) == blocks);
    // Blocks: every level of the file for the world's (64 + 3 x 16 bytes), the first for the
    // 2D layer's; else RGBA8 (8x8 with its chain, 8x8 alone).
    CHECK(made.resident_bytes == (blocks ? 112u + 64u : 4u * (64 + 16 + 4 + 1) + 4u * 64));

    // Each level of the world's is the file's own, the four quarters at every size.
    for (const float lod : {0.0f, 1.0f, 2.0f}) {
        CHECK(draws_quadrants(shader, pool.gl_texture(world), lod));
    }

    CHECK(draws_quadrants(shader, pool.gl_texture(flat), 0.0f));

    // The image stays for what the CPU reads.
    CHECK(pool.image(world).width == kSide && pool.image(world).pixels.size() == 8u * 8 * 4);

    // Refused, RGBA8 instead: blocks of another size than the texture, and a world texture
    // whose file has one level (the GPU cannot make a chain from blocks).
    const TextureHandle other = pool.add(Rgba8Image(4, 4), AlphaScale::Full, "other", true);
    pool.set_blocks(other, mips->blocks);
    CHECK(pool.gl_texture(other) != 0 && !pool.has_blocks(other));
    const auto single =
        pack_pictures::decode_ktx2(read(kData / "quadrants-uastc.ktx2"), format, why);
    CHECK(single && (single->blocks != nullptr) == blocks);

    if (single) {
        const TextureHandle lone = pool.add(single->image, AlphaScale::Full, "lone", true);
        pool.set_blocks(lone, single->blocks);
        CHECK(!pool.has_blocks(lone));
        CHECK(draws_quadrants(shader, pool.gl_texture(lone), 0.0f));
        // A 2D layer's texture takes them: one level is all it draws.
        const TextureHandle hud = pool.add(single->image, AlphaScale::Gs, "hud");
        pool.set_blocks(hud, single->blocks);
        CHECK(
            draws_quadrants(shader, pool.gl_texture(hud), 0.0f) && pool.has_blocks(hud) == blocks
        );
    }

    // After the context's textures are let go, the blocks are uploaded again.
    pool.release_gl();
    CHECK(
        draws_quadrants(shader, pool.gl_texture(world), 1.0f) && pool.has_blocks(world) == blocks
    );

    CHECK(glGetError() == GL_NO_ERROR);
    pool.release_gl();
    glDeleteVertexArrays(1, &vao);
    return test::result();
}
