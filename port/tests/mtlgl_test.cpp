// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// renderer/metal/mtlgl.h without a window: the context says what it is, clears the default
// framebuffer and reads it back with GL's pack alignment; textures, framebuffers, a multisample
// resolve, copies and buffers; a program's indexed draw. Then cases the renderers need, drawn
// through renderer/gl.h in a hidden window on OpenGL and on Metal, read back and compared.
// Skipped (exit 77) where there is no Metal device; the comparison where either window cannot open.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "check.h"
#include "platform/window.h"
#include "renderer/gl.h"
#include "renderer/metal/mtlgl.h"

namespace {

using namespace openrac::gl;

template <typename Fn>
Fn proc(const char* name) {
    const auto p = openrac::mtlgl::get_proc(name);
    CHECK(p != nullptr);
    return reinterpret_cast<Fn>(p);
}

// ---- The same calls on OpenGL and on Metal, through renderer/gl.h in a hidden window ----

constexpr int kSide = 16;
constexpr GLenum kRgba32ui = 0x8D70, kRgbaInteger = 0x8D99, kUnsignedInt = 0x1405,
                 kReplace = 0x1E01, kFrontAndBack = 0x0408;

GLuint program_of(const char* fragment) {
    const char* vertex = R"(#version 410 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;
out vec4 v_colour;
void main() {
    v_colour = colour;
    gl_Position = vec4(position, 1);
}
)";
    const GLuint program = glCreateProgram();
    for (const auto& [type, source] : {std::pair{GL_VERTEX_SHADER, vertex}, {GL_FRAGMENT_SHADER, fragment}}) {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    CHECK(linked == GL_TRUE);
    glUseProgram(program);
    return program;
}

// Triangles of (x, y, z, r, g, b, a) vertices.
void draw(std::initializer_list<float> vertices) {
    GLuint array = 0, buffer = 0;
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 std::data(vertices), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 7));
    glDeleteBuffers(1, &buffer);
    glDeleteVertexArrays(1, &array);
}

// The whole target at depth z in one colour.
void quad(float z, float r, float g, float b) {
    draw({-1, -1, z, r, g, b, 1, 1, -1, z, r, g, b, 1, 1, 1, z, r, g, b, 1,
          -1, -1, z, r, g, b, 1, 1, 1, z, r, g, b, 1, -1, 1, z, r, g, b, 1});
}

constexpr const char* kColourFragment = R"(#version 410 core
in vec4 v_colour;
layout(location = 0) out vec4 result;
void main() { result = v_colour; }
)";

GLuint texture(GLenum internal, int side) {
    GLuint name = 0;
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    const bool integer = internal == kRgba32ui;
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), side, side, 0,
                 integer ? kRgbaInteger : GL_RGBA, integer ? kUnsignedInt : GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return name;
}

// A framebuffer of a colour texture and, given a format, a depth renderbuffer, bound for drawing
// and reading, its viewport the whole of it.
GLuint framebuffer(GLuint colour, int side, GLenum depth = 0) {
    GLuint name = 0;
    glGenFramebuffers(1, &name);
    glBindFramebuffer(GL_FRAMEBUFFER, name);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour, 0);
    if (depth != 0) {
        GLuint buffer = 0;
        glGenRenderbuffers(1, &buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, depth, side, side);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  depth == GL_DEPTH24_STENCIL8 ? GL_DEPTH_STENCIL_ATTACHMENT
                                                               : GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, buffer);
    }
    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glViewport(0, 0, side, side);
    return name;
}

// A multisampled colour renderbuffer's framebuffer, bound for drawing and reading.
GLuint multisampled(int side) {
    GLuint buffer = 0, name = 0;
    glGenRenderbuffers(1, &buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, buffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, side, side);
    glGenFramebuffers(1, &name);
    glBindFramebuffer(GL_FRAMEBUFFER, name);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, buffer);
    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    return name;
}

void clear_to(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

// A fresh kSide² RGBA8 target, cleared to transparent black.
GLuint target(GLenum depth = 0) {
    const GLuint name = framebuffer(texture(GL_RGBA8, kSide), kSide, depth);
    clear_to(0, 0, 0, 0);
    return name;
}

// The read framebuffer's kSide² pixels, then the first error since the case began.
std::vector<std::uint8_t> pixels() {
    std::vector<std::uint8_t> out(kSide * kSide * 4 + 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, kSide, kSide, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
    out.back() = static_cast<std::uint8_t>(glGetError() & 0xFF);
    return out;
}

struct Case {
    const char* name;
    std::vector<std::uint8_t> (*run)();
    // AMD's OpenGL filters in eighths of a level, so channels may differ by 1 there; Apple's
    // OpenGL runs on Metal and agrees exactly.
    bool filtered = false;
};

const Case kCases[] = {
    {"gradient",
     [] {
         target();
         program_of(kColourFragment);
         draw({-1, -1, 0, 1, 0, 0, 1, 1, -1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1,
               -1, -1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, -1, 1, 0, 1, 1, 1, 0.5f});
         return pixels();
     }},
    {"edges",
     [] {
         target();
         program_of(kColourFragment);
         draw({-0.9f, -0.93f, 0, 1, 1, 1, 1, 0.71f, -0.2f, 0, 1, 1, 1, 1, -0.33f, 0.87f, 0, 1, 1, 1, 1,
               0.5f, 0.5f, 0, 1, 0, 0, 1, 0.75f, 0.5f, 0, 1, 0, 0, 1, 0.5f, 0.75f, 0, 1, 0, 0, 1});
         return pixels();
     }},
    {"linear texture",
     [] {
         target();
         const GLuint image = texture(GL_RGBA8, 3);
         const std::uint8_t texels[36] = {255, 0, 0, 255, 0, 255, 0, 200, 0, 0, 255, 255,
                                          17, 99, 3, 0, 250, 250, 250, 255, 64, 32, 16, 8,
                                          1, 2, 3, 4, 128, 129, 130, 131, 0, 255, 0, 255};
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 3, 3, GL_RGBA, GL_UNSIGNED_BYTE, texels);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glBindTexture(GL_TEXTURE_2D, image);
         program_of(R"(#version 410 core
in vec4 v_colour;
uniform sampler2D image;
layout(location = 0) out vec4 result;
void main() { result = texture(image, v_colour.xy); }
)");
         draw({-1, -1, 0, -0.1f, -0.2f, 0, 0, 1, -1, 0, 1.1f, -0.2f, 0, 0, 1, 1, 0, 1.1f, 0.9f, 0, 0,
               -1, -1, 0, -0.1f, -0.2f, 0, 0, 1, 1, 0, 1.1f, 0.9f, 0, 0, -1, 1, 0, -0.1f, 0.9f, 0, 0});
         return pixels();
     },
     true},
    {"scaled multisampled blit",
     [] {
         const GLuint from = multisampled(8);
         clear_to(1, 0, 0, 1);
         const GLuint into = target();
         glBindFramebuffer(GL_READ_FRAMEBUFFER, from);
         glBlitFramebuffer(0, 0, 8, 8, 0, 0, kSide, kSide, GL_COLOR_BUFFER_BIT, GL_LINEAR);
         glBindFramebuffer(GL_FRAMEBUFFER, into);
         return pixels();
     }},
};

// Each case's pixels drawn through `api`, and its renderer's name; empty when no window of it
// opens.
std::vector<std::vector<std::uint8_t>> draw_cases(openrac::platform::GlApi api, std::string& renderer) {
    openrac::platform::Platform sdl(openrac::platform::kVideo);
    openrac::platform::WindowConfig config;
    config.width = kSide;
    config.height = kSide;
    config.hidden = true;
    config.vsync = false;
    config.resizable = false;
    config.gl_api = api;
    std::string why;
    const auto window = openrac::platform::Window::open(config, why);
    std::string missing;
    if (!sdl.ok() || !window || window->gl_api() != api
        || !load(openrac::platform::Window::gl_loader(), missing)) {
        std::printf("no window: %s%s\n", why.c_str(), missing.c_str());
        return {};
    }
    renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    std::vector<std::vector<std::uint8_t>> drawn;
    for (const Case& c : kCases) {
        drawn.push_back(c.run());
    }
    return drawn;
}

}  // namespace

int main() {
    std::string error;
    if (!openrac::mtlgl::open({.width = 5, .height = 3}, error)) {
        std::printf("%s\n", error.c_str());
        return error == "no Metal device" ? 77 : 1;
    }
    const auto get_string = proc<PFN_glGetString>("glGetString");
    const auto get_error = proc<PFN_glGetError>("glGetError");
    const auto clear_colour = proc<PFN_glClearColor>("glClearColor");
    const auto clear = proc<PFN_glClear>("glClear");
    const auto enable = proc<PFN_glEnable>("glEnable");
    const auto disable = proc<PFN_glDisable>("glDisable");
    const auto pixel_store = proc<PFN_glPixelStorei>("glPixelStorei");
    const auto read_pixels = proc<PFN_glReadPixels>("glReadPixels");
    CHECK(openrac::mtlgl::get_proc("glNotAFunction") == nullptr);

    const std::string_view version = reinterpret_cast<const char*>(get_string(GL_VERSION));
    CHECK(version.starts_with("4.1 OpenRAC on Metal"));

    clear_colour(1.0f, 0.5f, 0.0f, 1.0f);
    clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK(get_error() == GL_NO_ERROR);

    // Rows of 5 pixels padded to 8 bytes' multiples: 20 bytes become 24.
    pixel_store(GL_PACK_ALIGNMENT, 8);
    std::vector<std::uint8_t> pixels(24 * 3, 0xAA);
    read_pixels(0, 0, 5, 3, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    for (int row = 0; row < 3; ++row) {
        for (int x = 0; x < 5; ++x) {
            const std::uint8_t* p = &pixels[static_cast<std::size_t>(row * 24 + x * 4)];
            CHECK(p[0] == 255 && p[1] >= 127 && p[1] <= 128 && p[2] == 0 && p[3] == 255);
        }
        CHECK(pixels[static_cast<std::size_t>(row * 24 + 20)] == 0xAA);
    }

    // Partly outside: only the pixel inside is written.
    std::uint8_t corner[8] = {};
    pixel_store(GL_PACK_ALIGNMENT, 4);
    read_pixels(4, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, corner);
    CHECK(corner[0] == 255 && corner[4] == 0);

    enable(GL_SCISSOR_TEST);
    clear(GL_COLOR_BUFFER_BIT);
    CHECK(get_error() == 0x0502);  // GL_INVALID_OPERATION
    disable(GL_SCISSOR_TEST);

    read_pixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, pixels.data());
    CHECK(get_error() == 0x0500);  // GL_INVALID_ENUM

    const auto gen_textures = proc<PFN_glGenTextures>("glGenTextures");
    const auto bind_texture = proc<PFN_glBindTexture>("glBindTexture");
    const auto tex_image = proc<PFN_glTexImage2D>("glTexImage2D");
    const auto tex_sub_image = proc<PFN_glTexSubImage2D>("glTexSubImage2D");
    const auto copy_tex_sub_image = proc<PFN_glCopyTexSubImage2D>("glCopyTexSubImage2D");
    const auto gen_framebuffers = proc<PFN_glGenFramebuffers>("glGenFramebuffers");
    const auto bind_framebuffer = proc<PFN_glBindFramebuffer>("glBindFramebuffer");
    const auto framebuffer_texture = proc<PFN_glFramebufferTexture2D>("glFramebufferTexture2D");
    const auto framebuffer_renderbuffer =
        proc<PFN_glFramebufferRenderbuffer>("glFramebufferRenderbuffer");
    const auto check_status = proc<PFN_glCheckFramebufferStatus>("glCheckFramebufferStatus");
    const auto gen_renderbuffers = proc<PFN_glGenRenderbuffers>("glGenRenderbuffers");
    const auto bind_renderbuffer = proc<PFN_glBindRenderbuffer>("glBindRenderbuffer");
    const auto renderbuffer_storage_multisample =
        proc<PFN_glRenderbufferStorageMultisample>("glRenderbufferStorageMultisample");
    const auto blit_framebuffer = proc<PFN_glBlitFramebuffer>("glBlitFramebuffer");
    const auto gen_buffers = proc<PFN_glGenBuffers>("glGenBuffers");
    const auto bind_buffer = proc<PFN_glBindBuffer>("glBindBuffer");
    const auto buffer_data = proc<PFN_glBufferData>("glBufferData");
    const auto buffer_sub_data = proc<PFN_glBufferSubData>("glBufferSubData");

    // A texture's rows read back as uploaded, row 0 first, through a framebuffer.
    GLuint textures[4] = {};
    gen_textures(4, textures);
    GLuint framebuffers[3] = {};
    gen_framebuffers(3, framebuffers);
    const std::uint8_t texels[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    bind_texture(GL_TEXTURE_2D, textures[0]);
    tex_image(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);
    bind_framebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    CHECK(check_status(GL_FRAMEBUFFER) == 0x8CD7);  // GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
    framebuffer_texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);
    CHECK(check_status(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    std::uint8_t read[16] = {};
    read_pixels(0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, read);
    CHECK(std::equal(read, read + 16, texels));

    // An upload after a clear in the same frame lands after it.
    clear_colour(0.0f, 1.0f, 0.0f, 1.0f);
    clear(GL_COLOR_BUFFER_BIT);
    const std::uint8_t blue[4] = {0, 0, 255, 255};
    tex_sub_image(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, blue);
    read_pixels(0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, read);
    CHECK(read[0] == 0 && read[1] == 255 && read[2] == 0);
    CHECK(read[12] == 0 && read[13] == 0 && read[14] == 255);
    CHECK(get_error() == GL_NO_ERROR);

    // A half float frame read as floats.
    const float hdr[4] = {2.5f, -1.0f, 0.25f, 1.0f};
    bind_texture(GL_TEXTURE_2D, textures[1]);
    tex_image(GL_TEXTURE_2D, 0, 0x881A /* GL_RGBA16F */, 1, 1, 0, GL_RGBA, GL_FLOAT, hdr);
    framebuffer_texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
    float floats[4] = {};
    read_pixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, floats);
    CHECK(std::equal(floats, floats + 4, hdr));

    // A multisampled clear resolved into a texture of its size.
    GLuint renderbuffer = 0;
    gen_renderbuffers(1, &renderbuffer);
    bind_renderbuffer(GL_RENDERBUFFER, renderbuffer);
    renderbuffer_storage_multisample(GL_RENDERBUFFER, 4, GL_RGBA8, 4, 4);
    bind_framebuffer(GL_FRAMEBUFFER, framebuffers[1]);
    framebuffer_renderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
    CHECK(check_status(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    clear_colour(1.0f, 0.0f, 0.0f, 1.0f);
    clear(GL_COLOR_BUFFER_BIT);
    bind_texture(GL_TEXTURE_2D, textures[2]);
    tex_image(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    bind_framebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[2]);
    framebuffer_texture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[2], 0);
    blit_framebuffer(0, 0, 4, 4, 0, 0, 4, 4, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    CHECK(get_error() == GL_NO_ERROR);
    blit_framebuffer(0, 0, 4, 4, 0, 0, 2, 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    CHECK(get_error() == 0x0502);  // a scaled resolve: not yet
    bind_framebuffer(GL_READ_FRAMEBUFFER, framebuffers[2]);
    read_pixels(3, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, read);
    CHECK(read[0] == 255 && read[1] == 0 && read[2] == 0 && read[3] == 255);

    // A flipped, doubled blit, drawn: the 2x2 texture's blue (1, 1) lands on rows 0-1, columns 2-3.
    bind_framebuffer(GL_READ_FRAMEBUFFER, framebuffers[0]);
    framebuffer_texture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);
    blit_framebuffer(0, 0, 2, 2, 0, 4, 4, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    CHECK(get_error() == GL_NO_ERROR);
    bind_framebuffer(GL_READ_FRAMEBUFFER, framebuffers[2]);
    std::uint8_t doubled[64] = {};
    read_pixels(0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, doubled);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const std::uint8_t* p = doubled + (row * 4 + column) * 4;
            const bool blue_here = row < 2 && column >= 2;
            CHECK(p[0] == 0 && p[1] == (blue_here ? 0 : 255) && p[2] == (blue_here ? 255 : 0));
        }
    }

    // The default framebuffer's orange copied into a texture.
    bind_framebuffer(GL_FRAMEBUFFER, 0);
    bind_texture(GL_TEXTURE_2D, textures[3]);
    tex_image(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    copy_tex_sub_image(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 2, 2);
    bind_framebuffer(GL_FRAMEBUFFER, framebuffers[2]);
    framebuffer_texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[3], 0);
    read_pixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, read);
    CHECK(read[0] == 255 && read[1] >= 127 && read[1] <= 128 && read[2] == 0);
    bind_framebuffer(GL_FRAMEBUFFER, 0);

    GLuint buffer = 0;
    gen_buffers(1, &buffer);
    bind_buffer(GL_ARRAY_BUFFER, buffer);
    buffer_data(GL_ARRAY_BUFFER, 8, texels, GL_STATIC_DRAW);
    buffer_sub_data(GL_ARRAY_BUFFER, 4, 4, texels);
    CHECK(get_error() == GL_NO_ERROR);
    buffer_sub_data(GL_ARRAY_BUFFER, 6, 4, texels);
    CHECK(get_error() == 0x0501);  // GL_INVALID_VALUE

    // An indexed quad on the bottom left quarter, in a uniform's colour plus an input the vertex
    // array does not feed, which reads GL's (0, 0, 0, 1).
    const auto create_shader = proc<PFN_glCreateShader>("glCreateShader");
    const auto shader_source = proc<PFN_glShaderSource>("glShaderSource");
    const auto compile_shader = proc<PFN_glCompileShader>("glCompileShader");
    const auto get_shader_iv = proc<PFN_glGetShaderiv>("glGetShaderiv");
    const auto create_program = proc<PFN_glCreateProgram>("glCreateProgram");
    const auto attach_shader = proc<PFN_glAttachShader>("glAttachShader");
    const auto link_program = proc<PFN_glLinkProgram>("glLinkProgram");
    const auto get_program_iv = proc<PFN_glGetProgramiv>("glGetProgramiv");
    const auto get_program_info_log = proc<PFN_glGetProgramInfoLog>("glGetProgramInfoLog");
    const auto use_program = proc<PFN_glUseProgram>("glUseProgram");
    const auto get_uniform_location = proc<PFN_glGetUniformLocation>("glGetUniformLocation");
    const auto uniform_4f = proc<PFN_glUniform4f>("glUniform4f");
    const auto gen_vertex_arrays = proc<PFN_glGenVertexArrays>("glGenVertexArrays");
    const auto bind_vertex_array = proc<PFN_glBindVertexArray>("glBindVertexArray");
    const auto enable_attrib = proc<PFN_glEnableVertexAttribArray>("glEnableVertexAttribArray");
    const auto attrib_pointer = proc<PFN_glVertexAttribPointer>("glVertexAttribPointer");
    const auto draw_elements = proc<PFN_glDrawElements>("glDrawElements");
    const auto draw_arrays = proc<PFN_glDrawArrays>("glDrawArrays");
    const auto viewport = proc<PFN_glViewport>("glViewport");

    const char* sources[2] = {R"(#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 tint;
out vec4 v_tint;
void main() {
    v_tint = tint;
    gl_Position = vec4(position, 0, 1);
}
)",
                              R"(#version 410 core
uniform vec4 colour;
in vec4 v_tint;
layout(location = 0) out vec4 result;
void main() { result = colour + v_tint; }
)"};
    const GLuint program = create_program();
    for (int i = 0; i < 2; ++i) {
        const GLuint shader = create_shader(i == 0 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        shader_source(shader, 1, &sources[i], nullptr);
        compile_shader(shader);
        GLint compiled = 0;
        get_shader_iv(shader, GL_COMPILE_STATUS, &compiled);
        CHECK(compiled == GL_TRUE);
        attach_shader(program, shader);
    }
    link_program(program);
    GLint linked = 0;
    get_program_iv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[1024] = {};
        get_program_info_log(program, sizeof log, nullptr, log);
        std::printf("%s\n", log);
    }
    CHECK(linked == GL_TRUE);
    use_program(program);
    const GLint colour = get_uniform_location(program, "colour");
    CHECK(colour >= 0);
    uniform_4f(colour, 1.0f, 0.0f, 0.0f, 0.0f);

    GLuint vertex_array = 0;
    gen_vertex_arrays(1, &vertex_array);
    bind_vertex_array(vertex_array);
    GLuint quad[2] = {};
    gen_buffers(2, quad);
    const float corners[8] = {-1, -1, 0, -1, 0, 0, -1, 0};
    bind_buffer(GL_ARRAY_BUFFER, quad[0]);
    buffer_data(GL_ARRAY_BUFFER, sizeof corners, corners, GL_STATIC_DRAW);
    enable_attrib(0);
    attrib_pointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    const std::uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
    bind_buffer(GL_ELEMENT_ARRAY_BUFFER, quad[1]);
    buffer_data(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

    GLuint target = 0;
    gen_textures(1, &target);
    bind_texture(GL_TEXTURE_2D, target);
    tex_image(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    bind_framebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    framebuffer_texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
    viewport(0, 0, 4, 4);
    clear_colour(0.0f, 0.0f, 0.0f, 0.0f);
    clear(GL_COLOR_BUFFER_BIT);
    draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    CHECK(get_error() == GL_NO_ERROR);
    std::uint8_t drawn[64] = {};
    read_pixels(0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, drawn);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            const std::uint8_t* p = &drawn[(y * 4 + x) * 4];
            const bool inside = x < 2 && y < 2;
            CHECK(p[0] == (inside ? 255 : 0) && p[1] == 0 && p[2] == 0 && p[3] == (inside ? 255 : 0));
        }
    }

    // What Metal cannot draw is refused.
    draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
    CHECK(get_error() == 0x0500);  // GL_INVALID_ENUM
    draw_arrays(0x0006 /* GL_TRIANGLE_FAN */, 0, 4);
    CHECK(get_error() == 0x0500);
    bind_framebuffer(GL_FRAMEBUFFER, 0);

    openrac::mtlgl::present();
    openrac::mtlgl::close();
    CHECK(!openrac::mtlgl::active());

    std::string renderer;
    const auto opengl = draw_cases(openrac::platform::GlApi::Desktop, renderer);
    const auto metal = draw_cases(openrac::platform::GlApi::Metal, renderer);
    if (opengl.empty() || metal.empty()) {
        return openrac::test::result();
    }
    const bool apple = renderer.starts_with("Apple");
    for (std::size_t i = 0; i < std::size(kCases); ++i) {
        const auto& a = opengl[i];
        const auto& b = metal[i];
        int differ = 0, worst = 0;
        for (std::size_t n = 0; n + 1 < a.size(); ++n) {
            const int d = std::abs(a[n] - b[n]);
            differ += d != 0 ? 1 : 0;
            worst = std::max(worst, d);
        }
        std::printf("%s on %s: OpenGL error 0x%02X, Metal 0x%02X; %d channels differ, by up to %d\n",
                    kCases[i].name, renderer.c_str(), a.back(), b.back(), differ, worst);
        CHECK(a.back() == b.back());
        CHECK(worst <= (kCases[i].filtered && !apple ? 1 : 0));
    }
    return openrac::test::result();
}
