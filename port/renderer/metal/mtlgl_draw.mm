// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shaders, programs and their uniforms, vertex arrays, and draws, as gl_program.cpp and
// vkgl_draw.cpp do them for Vulkan. A program's GLSL is linked by vkgl::compile_program, as for
// Vulkan, and written as MSL by translate_program (shader_translate.h); each draw binds what the
// program's Metal slots name.

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string_view>

#include "common/log.h"
#include "renderer/metal/mtlgl_internal.h"

namespace openrac::mtlgl::detail {

using namespace openrac::gl;

namespace {

constexpr GLenum kGlByte = 0x1400;
constexpr GLenum kGlReplace = 0x1E01;
constexpr GLenum kGlIncr = 0x1E02;
constexpr GLenum kGlDecr = 0x1E03;
constexpr GLenum kGlInvert = 0x150A;
constexpr GLenum kGlSrcAlphaSaturate = 0x0308;
constexpr GLenum kGlMin = 0x8007;
constexpr GLenum kGlMax = 0x8008;
constexpr GLenum kGlActiveUniforms = 0x8B86;
constexpr GLenum kGlNearestMipmapNearest = 0x2700;
constexpr GLenum kGlLinearMipmapNearest = 0x2701;

// The bytes of `values`, one after another: a cache key.
template <typename... T>
std::string key_of(const T&... values) {
    std::string key;
    (key.append(reinterpret_cast<const char*>(&values), sizeof(values)), ...);
    return key;
}

void write_log(const std::string& log, GLsizei size, GLsizei* length, GLchar* out) {
    const GLsizei n = size > 0 ? std::min<GLsizei>(size - 1, static_cast<GLsizei>(log.size())) : 0;
    if (out != nullptr && size > 0) {
        std::memcpy(out, log.data(), static_cast<std::size_t>(n));
        out[n] = '\0';
    }
    if (length != nullptr) {
        *length = n;
    }
}

Program* current_program() {
    Program* p = g->programs.find(g->state.program);
    return p != nullptr && p->linked ? p : nullptr;
}

}  // namespace

VertexArray& vertex_array() {
    VertexArray* va = g->vertex_arrays.find(g->state.vertex_array);
    return va != nullptr ? *va : g->default_vertex_array;
}

#define MTLGL OPENRAC_GLAPI

// ---- Shaders and programs ----

GLuint MTLGL create_shader(GLenum type) {
    const GLuint name = g->shaders.add();
    g->shaders.find(name)->type = type;
    return name;
}

void MTLGL shader_source(GLuint name, GLsizei count, const GLchar* const* strings, const GLint* lengths) {
    Shader* sh = g->shaders.find(name);
    if (sh == nullptr) {
        return;
    }
    sh->source.clear();
    for (GLsizei i = 0; i < count; ++i) {
        if (lengths != nullptr && lengths[i] >= 0) {
            sh->source.append(strings[i], static_cast<std::size_t>(lengths[i]));
        } else {
            sh->source.append(strings[i]);
        }
    }
}

void MTLGL compile_shader(GLuint name) {
    if (Shader* sh = g->shaders.find(name)) {
        sh->log.clear();
        sh->compiled = sh->type == GL_COMPUTE_SHADER
                           ? vkgl::check_compute_stage(sh->source, sh->log)
                           : vkgl::check_stage(sh->type == GL_VERTEX_SHADER, sh->source, sh->log);
    }
}

void MTLGL get_shader_iv(GLuint name, GLenum what, GLint* out) {
    const Shader* sh = g->shaders.find(name);
    *out = 0;
    if (sh == nullptr) {
        return;
    }
    if (what == GL_COMPILE_STATUS) {
        *out = sh->compiled ? GL_TRUE : GL_FALSE;
    } else if (what == GL_INFO_LOG_LENGTH) {
        *out = sh->log.empty() ? 0 : static_cast<GLint>(sh->log.size() + 1);
    }
}

void MTLGL get_shader_info_log(GLuint name, GLsizei size, GLsizei* length, GLchar* log) {
    const Shader* sh = g->shaders.find(name);
    write_log(sh != nullptr ? sh->log : std::string(), size, length, log);
}

void MTLGL delete_shader(GLuint name) {
    g->shaders.objects.erase(name);
}

GLuint MTLGL create_program() {
    return g->programs.add();
}

void MTLGL attach_shader(GLuint program, GLuint shader) {
    if (Program* p = g->programs.find(program)) {
        p->shaders.push_back(shader);
    }
}

void MTLGL bind_attrib_location(GLuint, GLuint, const GLchar*) {
    // The renderer's shaders give their inputs locations of their own.
}

namespace {

id<MTLLibrary> library_of(Program& p, const std::string& source) {
    MTLCompileOptions* options = [MTLCompileOptions new];
    options.languageVersion = MTLLanguageVersion2_4;
    // OpenGL's shaders round as written; fast math changes the last bit of colours.
    options.fastMathEnabled = NO;
    NSError* error = nil;
    id<MTLLibrary> library = [g->device newLibraryWithSource:@(source.c_str())
                                                     options:options
                                                       error:&error];
    if (library == nil) {
        p.log = std::format("Metal: {}", error.localizedDescription.UTF8String);
    }
    return library;
}

// The vertex stage's depth, Program::functions' first index: 0 GL's -w..w taken to 0..w, 1 z as
// it is (glClipControl), 2 and 3 the far and near halves of a depth-tested draw, 4 and 5 those
// with depth clamped. OpenGL's window depth is the GPU's own z / w times 0.5 plus 0.5, which no
// shader computes: taking (z + w) * 0.5 to 0..1 misses its last bit for about two points in five.
// Metal rounds as OpenGL does when given z as it is and a range of 0.5 to 1, but it clamps or
// clips to that range first. So a depth-tested draw is drawn twice: given -z and 0.5 to 0 it keeps
// the far half exactly, which with reversed depth is all but the space within twice the near plane,
// and clamps z beyond the far plane to 0 as GL_DEPTH_CLAMP does; given z as it is and 0.5 to 1, the
// near half. Both in one draw, as amplified copies, would move triangles' edges on Apple's GPUs.
// Clamped depth clips neither half away, so there the fragment stage keeps each half's own. An
// AMD GPU draws nothing given a range from 0.5 to 0, so off Apple's GPUs a depth-tested draw keeps
// (z + w) * 0.5. The first draw's form; a form of 2 or more has the next form's draw too.
int depth_form(const State& s) {
    if (s.clip_zero_to_one) {
        return 1;
    }
    return !s.depth_test || ![g->device supportsFamily:MTLGPUFamilyApple1] ? 0 : s.depth_clamp ? 4 : 2;
}

id<MTLFunction> function_of(Program& p, id<MTLLibrary> library, const std::string& entry, int form) {
    const bool clip = form != 0, far = form == 2 || form == 4, clamped = form >= 4;
    MTLFunctionConstantValues* constants = [MTLFunctionConstantValues new];
    [constants setConstantValue:&clip type:MTLDataTypeBool atIndex:kClipConstant];
    [constants setConstantValue:&far type:MTLDataTypeBool atIndex:kFarHalfConstant];
    [constants setConstantValue:&clamped type:MTLDataTypeBool atIndex:kClampedConstant];
    NSError* error = nil;
    id<MTLFunction> function = [library newFunctionWithName:@(entry.c_str())
                                             constantValues:constants
                                                      error:&error];
    if (function == nil) {
        p.log = std::format("Metal: {}: {}", entry, error.localizedDescription.UTF8String);
    }
    return function;
}

}  // namespace

// Compute programs link (their stage compiles and translates) but have no dispatch yet.
void MTLGL link_program(GLuint name) {
    Program* p = g->programs.find(name);
    if (p == nullptr) {
        return;
    }
    std::string vs, fs, cs;
    for (GLuint s : p->shaders) {
        if (const Shader* sh = g->shaders.find(s)) {
            (sh->type == GL_VERTEX_SHADER ? vs : sh->type == GL_COMPUTE_SHADER ? cs : fs) = sh->source;
        }
    }
    std::vector<GLuint> shaders = std::move(p->shaders);
    *p = Program{};
    p->shaders = std::move(shaders);
    if (!vkgl::compile_program(vs, fs, p->compiled, p->log, cs)
        || !translate_program(p->compiled, [g->device supportsFamily:MTLGPUFamilyApple1], p->msl, p->log)) {
        return;
    }
    if (cs.empty()) {
        p->vertex = library_of(*p, p->msl.vertex);
        p->fragment = p->vertex != nil ? library_of(*p, p->msl.fragment) : nil;
        if (p->fragment == nil) {
            return;
        }
        for (int form = 0; form < 6; ++form) {
            __strong id<MTLFunction>* f = p->functions[form];
            f[0] = function_of(*p, p->vertex, p->msl.vertex_entry, form);
            f[1] = function_of(*p, p->fragment, p->msl.fragment_entry, form);
            if (f[0] == nil || f[1] == nil) {
                return;
            }
        }
    } else if (library_of(*p, p->msl.compute) == nil) {
        return;
    }
    // Uniforms start at zero, as GL's do.
    p->uniforms.assign(static_cast<std::size_t>(std::max(p->compiled.default_size, 0)), 0);
    p->units.assign(p->compiled.uniforms.size(), 0);
    p->block_bindings.assign(p->compiled.blocks.size(), 0);
    p->linked = true;
}

void MTLGL get_program_iv(GLuint name, GLenum what, GLint* out) {
    const Program* p = g->programs.find(name);
    *out = 0;
    if (p == nullptr) {
        return;
    }
    if (what == GL_LINK_STATUS) {
        *out = p->linked ? GL_TRUE : GL_FALSE;
    } else if (what == GL_INFO_LOG_LENGTH) {
        *out = p->log.empty() ? 0 : static_cast<GLint>(p->log.size() + 1);
    } else if (what == kGlActiveUniforms) {
        *out = 0;  // not listed, as on vkgl
    }
}

void MTLGL get_program_info_log(GLuint name, GLsizei size, GLsizei* length, GLchar* log) {
    const Program* p = g->programs.find(name);
    write_log(p != nullptr ? p->log : std::string(), size, length, log);
}

void MTLGL delete_program(GLuint name) {
    g->programs.objects.erase(name);
    if (g->state.program == name) {
        g->state.program = 0;
    }
}

void MTLGL use_program(GLuint name) {
    g->state.program = name;
}

// ---- Uniforms ----

namespace {

// A uniform's location: its entry in the program's list, and the element of an array.
constexpr int kElementBits = 12;

// Where a location's value goes in the current program's default block; null for none. A
// sampler's uniform is given in `sampler`, by its index in the program's list.
std::uint8_t* uniform_at(GLint location, std::size_t bytes, int* sampler = nullptr) {
    Program* p = current_program();
    if (p == nullptr || location < 0) {
        return nullptr;
    }
    const auto index = static_cast<std::size_t>(location) >> kElementBits;
    const int element = location & ((1 << kElementBits) - 1);
    if (index >= p->compiled.uniforms.size()) {
        return nullptr;
    }
    const vkgl::ProgramUniform& u = p->compiled.uniforms[index];
    if (u.sampler) {
        if (sampler != nullptr) {
            *sampler = static_cast<int>(index);
        }
        return nullptr;
    }
    const auto at = static_cast<std::size_t>(u.offset)
                    + static_cast<std::size_t>(element) * static_cast<std::size_t>(u.array_stride);
    if (u.offset < 0 || element >= u.array_size || at + bytes > p->uniforms.size()) {
        return nullptr;
    }
    return p->uniforms.data() + at;
}

void write_uniform(GLint location, const void* data, std::size_t bytes) {
    if (std::uint8_t* at = uniform_at(location, bytes)) {
        std::memcpy(at, data, bytes);
    }
}

}  // namespace

GLint MTLGL get_uniform_location(GLuint program, const GLchar* name) {
    const Program* p = g->programs.find(program);
    if (p == nullptr || !p->linked || name == nullptr) {
        return -1;
    }
    std::string_view n(name);
    int element = 0;
    if (const std::size_t bracket = n.find('['); bracket != std::string_view::npos) {
        element = std::atoi(std::string(n.substr(bracket + 1)).c_str());
        n = n.substr(0, bracket);
    }
    const auto& list = p->compiled.uniforms;
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].name == n && element >= 0 && element < list[i].array_size) {
            return static_cast<GLint>(i << kElementBits | static_cast<std::size_t>(element));
        }
    }
    return -1;
}

void MTLGL uniform_1i(GLint location, GLint v) {
    int sampler = -1;
    if (std::uint8_t* at = uniform_at(location, 4, &sampler)) {
        std::memcpy(at, &v, 4);
    } else if (sampler >= 0) {
        current_program()->units[static_cast<std::size_t>(sampler)] = v;
    }
}

void MTLGL uniform_1ui(GLint location, GLuint v) {
    write_uniform(location, &v, 4);
}

void MTLGL uniform_1f(GLint location, GLfloat v) {
    write_uniform(location, &v, 4);
}

void MTLGL uniform_2f(GLint location, GLfloat a, GLfloat b) {
    const float v[] = {a, b};
    write_uniform(location, v, 8);
}

void MTLGL uniform_3f(GLint location, GLfloat a, GLfloat b, GLfloat c) {
    const float v[] = {a, b, c};
    write_uniform(location, v, 12);
}

void MTLGL uniform_4f(GLint location, GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    const float v[] = {a, b, c, d};
    write_uniform(location, v, 16);
}

void MTLGL uniform_4fv(GLint location, GLsizei count, const GLfloat* v) {
    for (GLsizei i = 0; location >= 0 && i < count; ++i) {
        write_uniform(location + i, v + static_cast<std::size_t>(i) * 4, 16);
    }
}

void MTLGL uniform_matrix_4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* v) {
    for (GLsizei i = 0; location >= 0 && i < count; ++i) {
        const GLfloat* m = v + static_cast<std::size_t>(i) * 16;
        std::array<GLfloat, 16> columns;
        for (std::size_t c = 0; c < 16; ++c) {
            columns[c] = transpose != GL_FALSE ? m[(c % 4) * 4 + c / 4] : m[c];
        }
        write_uniform(location + i, columns.data(), 64);
    }
}

GLuint MTLGL get_uniform_block_index(GLuint program, const GLchar* name) {
    const Program* p = g->programs.find(program);
    if (p == nullptr || !p->linked || name == nullptr) {
        return wgl::GL_INVALID_INDEX;
    }
    for (std::size_t i = 0; i < p->compiled.blocks.size(); ++i) {
        if (p->compiled.blocks[i].name == name) {
            return static_cast<GLuint>(i);
        }
    }
    return wgl::GL_INVALID_INDEX;
}

void MTLGL uniform_block_binding(GLuint program, GLuint block, GLuint binding) {
    Program* p = g->programs.find(program);
    if (p != nullptr && block < p->block_bindings.size()) {
        p->block_bindings[block] = binding;
    }
}

// ---- Vertex arrays ----

void MTLGL gen_vertex_arrays(GLsizei n, GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        names[i] = g->vertex_arrays.add();
    }
}

void MTLGL delete_vertex_arrays(GLsizei n, const GLuint* names) {
    for (GLsizei i = 0; i < n; ++i) {
        g->vertex_arrays.objects.erase(names[i]);
        if (g->state.vertex_array == names[i]) {
            g->state.vertex_array = 0;
        }
    }
}

void MTLGL bind_vertex_array(GLuint name) {
    g->state.vertex_array = name;
}

void MTLGL enable_vertex_attrib_array(GLuint index) {
    if (index < kAttributes) {
        vertex_array().attributes[index].enabled = true;
    }
}

void MTLGL vertex_attrib_pointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                 GLsizei stride, const void* pointer) {
    if (index >= kAttributes) {
        return;
    }
    Attribute& a = vertex_array().attributes[index];
    a.size = std::clamp(size, 1, 4);
    a.type = type;
    a.normalized = normalized != GL_FALSE;
    a.integer = false;
    a.stride = stride;
    a.offset = reinterpret_cast<std::uintptr_t>(pointer);
    a.buffer = g->state.array_buffer;
}

void MTLGL vertex_attrib_i_pointer(GLuint index, GLint size, GLenum type, GLsizei stride,
                                   const void* pointer) {
    vertex_attrib_pointer(index, size, type, GL_FALSE, stride, pointer);
    if (index < kAttributes) {
        vertex_array().attributes[index].integer = true;
    }
}

void MTLGL vertex_attrib_divisor(GLuint index, GLuint divisor) {
    if (index < kAttributes) {
        vertex_array().attributes[index].divisor = divisor;
    }
}

// ---- Draws ----

namespace {

// The vertex format of an attribute; invalid for float data read unnormalised from integers,
// which Metal does not convert.
MTLVertexFormat vertex_format(const Attribute& a) {
    auto pick = [&](MTLVertexFormat one, MTLVertexFormat two, MTLVertexFormat three,
                    MTLVertexFormat four) {
        const MTLVertexFormat f[] = {one, two, three, four};
        return f[a.size - 1];
    };
    const bool plain = !a.integer && !a.normalized;
    switch (a.type) {
        case GL_FLOAT:
            return pick(MTLVertexFormatFloat, MTLVertexFormatFloat2, MTLVertexFormatFloat3,
                        MTLVertexFormatFloat4);
        case GL_HALF_FLOAT:
            return pick(MTLVertexFormatHalf, MTLVertexFormatHalf2, MTLVertexFormatHalf3,
                        MTLVertexFormatHalf4);
        case GL_UNSIGNED_BYTE:
            return plain ? MTLVertexFormatInvalid
                   : a.integer
                       ? pick(MTLVertexFormatUChar, MTLVertexFormatUChar2, MTLVertexFormatUChar3,
                              MTLVertexFormatUChar4)
                       : pick(MTLVertexFormatUCharNormalized, MTLVertexFormatUChar2Normalized,
                              MTLVertexFormatUChar3Normalized, MTLVertexFormatUChar4Normalized);
        case kGlByte:
            return plain ? MTLVertexFormatInvalid
                   : a.integer
                       ? pick(MTLVertexFormatChar, MTLVertexFormatChar2, MTLVertexFormatChar3,
                              MTLVertexFormatChar4)
                       : pick(MTLVertexFormatCharNormalized, MTLVertexFormatChar2Normalized,
                              MTLVertexFormatChar3Normalized, MTLVertexFormatChar4Normalized);
        case GL_UNSIGNED_SHORT:
            return plain ? MTLVertexFormatInvalid
                   : a.integer
                       ? pick(MTLVertexFormatUShort, MTLVertexFormatUShort2, MTLVertexFormatUShort3,
                              MTLVertexFormatUShort4)
                       : pick(MTLVertexFormatUShortNormalized, MTLVertexFormatUShort2Normalized,
                              MTLVertexFormatUShort3Normalized, MTLVertexFormatUShort4Normalized);
        case GL_SHORT:
            return plain ? MTLVertexFormatInvalid
                   : a.integer
                       ? pick(MTLVertexFormatShort, MTLVertexFormatShort2, MTLVertexFormatShort3,
                              MTLVertexFormatShort4)
                       : pick(MTLVertexFormatShortNormalized, MTLVertexFormatShort2Normalized,
                              MTLVertexFormatShort3Normalized, MTLVertexFormatShort4Normalized);
        case GL_INT:
            return a.integer ? pick(MTLVertexFormatInt, MTLVertexFormatInt2, MTLVertexFormatInt3,
                                    MTLVertexFormatInt4)
                             : MTLVertexFormatInvalid;
        case GL_UNSIGNED_INT:
            return a.integer ? pick(MTLVertexFormatUInt, MTLVertexFormatUInt2, MTLVertexFormatUInt3,
                                    MTLVertexFormatUInt4)
                             : MTLVertexFormatInvalid;
        default:
            return MTLVertexFormatInvalid;
    }
}

int type_bytes(GLenum type) {
    switch (type) {
        case GL_UNSIGNED_BYTE:
        case kGlByte:
            return 1;
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
        case GL_HALF_FLOAT:
            return 2;
        default:
            return 4;
    }
}

// An input's GL type as GL's default (0, 0, 0, 1): float 0, unsigned 1, signed 2.
int input_kind(std::uint32_t gl_type) {
    switch (gl_type) {
        case GL_UNSIGNED_INT:
        case 0x8DC6:  // GL_UNSIGNED_INT_VEC2..4
        case 0x8DC7:
        case 0x8DC8:
            return 1;
        case GL_INT:
        case 0x8B53:  // GL_INT_VEC2..4
        case 0x8B54:
        case 0x8B55:
            return 2;
        default:
            return 0;
    }
}

MTLPrimitiveType primitive_of(GLenum mode, bool& ok) {
    ok = true;
    switch (mode) {
        case GL_POINTS:
            return MTLPrimitiveTypePoint;
        case GL_LINES:
            return MTLPrimitiveTypeLine;
        case GL_LINE_STRIP:
            return MTLPrimitiveTypeLineStrip;
        case GL_TRIANGLES:
            return MTLPrimitiveTypeTriangle;
        case GL_TRIANGLE_STRIP:
            return MTLPrimitiveTypeTriangleStrip;
        default:
            ok = false;  // fans and loops: Metal has none, and no renderer draws them
            return MTLPrimitiveTypeTriangle;
    }
}

MTLBlendFactor blend_factor(GLenum f) {
    switch (f) {
        case GL_ZERO:
            return MTLBlendFactorZero;
        case GL_SRC_COLOR:
            return MTLBlendFactorSourceColor;
        case GL_ONE_MINUS_SRC_COLOR:
            return MTLBlendFactorOneMinusSourceColor;
        case GL_SRC_ALPHA:
            return MTLBlendFactorSourceAlpha;
        case GL_ONE_MINUS_SRC_ALPHA:
            return MTLBlendFactorOneMinusSourceAlpha;
        case GL_DST_ALPHA:
            return MTLBlendFactorDestinationAlpha;
        case GL_ONE_MINUS_DST_ALPHA:
            return MTLBlendFactorOneMinusDestinationAlpha;
        case GL_DST_COLOR:
            return MTLBlendFactorDestinationColor;
        case GL_ONE_MINUS_DST_COLOR:
            return MTLBlendFactorOneMinusDestinationColor;
        case GL_CONSTANT_COLOR:
            return MTLBlendFactorBlendColor;
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return MTLBlendFactorOneMinusBlendColor;
        case GL_CONSTANT_ALPHA:
            return MTLBlendFactorBlendAlpha;
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return MTLBlendFactorOneMinusBlendAlpha;
        case kGlSrcAlphaSaturate:
            return MTLBlendFactorSourceAlphaSaturated;
        default:
            return MTLBlendFactorOne;
    }
}

MTLBlendOperation blend_operation(GLenum e) {
    switch (e) {
        case GL_FUNC_SUBTRACT:
            return MTLBlendOperationSubtract;
        case GL_FUNC_REVERSE_SUBTRACT:
            return MTLBlendOperationReverseSubtract;
        case kGlMin:
            return MTLBlendOperationMin;
        case kGlMax:
            return MTLBlendOperationMax;
        default:
            return MTLBlendOperationAdd;
    }
}

// GL's GL_NEVER..GL_ALWAYS are Metal's compare functions in the same order.
MTLCompareFunction compare_of(GLenum f) {
    return static_cast<MTLCompareFunction>(std::clamp<GLenum>(f, GL_NEVER, GL_ALWAYS) - GL_NEVER);
}

MTLStencilOperation stencil_operation(GLenum op) {
    switch (op) {
        case GL_ZERO:
            return MTLStencilOperationZero;
        case kGlReplace:
            return MTLStencilOperationReplace;
        case kGlIncr:
            return MTLStencilOperationIncrementClamp;
        case kGlDecr:
            return MTLStencilOperationDecrementClamp;
        case kGlInvert:
            return MTLStencilOperationInvert;
        case GL_INCR_WRAP:
            return MTLStencilOperationIncrementWrap;
        case GL_DECR_WRAP:
            return MTLStencilOperationDecrementWrap;
        default:
            return MTLStencilOperationKeep;
    }
}

bool has_stencil(id<MTLTexture> depth) {
    return depth.pixelFormat == MTLPixelFormatDepth32Float_Stencil8;
}

// A vertex buffer slot: `buffer` from `offset`, the lowest offset of the attributes it feeds.
struct Stream {
    Buffer* buffer;
    std::uintptr_t offset;
    NSUInteger stride;
    GLuint divisor;
};

// The vertex inputs of a draw: the pipeline's descriptor and the buffers it reads.
struct Inputs {
    MTLVertexDescriptor* descriptor = [MTLVertexDescriptor vertexDescriptor];
    std::string key;
    std::vector<Stream> buffers;  // from slot kVertexBufferBase
    bool defaults = false;  // GL's (0, 0, 0, 1) bound after them
};

// False, with GL_INVALID_OPERATION, for an attribute Metal cannot read.
bool inputs_of(const Program& p, Inputs& out) {
    const VertexArray& va = vertex_array();
    struct Fed {
        int location;
        const Attribute* a;
        Buffer* b;
        MTLVertexFormat format;
    };
    std::vector<Fed> fed;
    for (const vkgl::CompiledProgram::Attribute& in : p.compiled.attributes) {
        if (in.location < 0 || in.location >= static_cast<int>(kAttributes)) {
            continue;
        }
        const Attribute& a = va.attributes[static_cast<std::size_t>(in.location)];
        Buffer* b = a.enabled ? g->buffers.find(a.buffer) : nullptr;
        MTLVertexAttributeDescriptor* attribute = out.descriptor.attributes[static_cast<NSUInteger>(in.location)];
        if (b != nullptr && b->buffer != nil) {
            const MTLVertexFormat format = vertex_format(a);
            if (format == MTLVertexFormatInvalid) {
                fail(kGlInvalidOperation);
                return false;
            }
            fed.push_back({in.location, &a, b, format});
        } else {
            const int kind = input_kind(in.gl_type);
            attribute.format = kind == 0 ? MTLVertexFormatFloat4
                               : kind == 1 ? MTLVertexFormatUInt4
                                           : MTLVertexFormatInt4;
            attribute.offset = kind == 0 ? 0 : 16;
            out.defaults = true;
            out.key += key_of(in.location, kind);
        }
    }
    // Interleaved attributes share their buffer's slot: one slot each would pass Metal's 31.
    std::ranges::stable_sort(fed, {}, [](const Fed& f) { return f.a->offset; });
    for (const Fed& f : fed) {
        const Attribute& a = *f.a;
        const auto bytes = static_cast<std::uintptr_t>(a.size * type_bytes(a.type));
        const auto stride = static_cast<NSUInteger>(a.stride != 0 ? a.stride : bytes);
        const auto shared = std::ranges::find_if(out.buffers, [&](const Stream& s) {
            return s.buffer == f.b && s.stride == stride && s.divisor == a.divisor
                   && a.offset + bytes <= s.offset + s.stride;
        });
        const auto index = static_cast<NSUInteger>(shared - out.buffers.begin());
        const NSUInteger slot = kVertexBufferBase + index;
        if (shared == out.buffers.end()) {
            if (slot >= 31) {
                fail(kGlInvalidOperation);  // Metal's 31 buffer slots
                return false;
            }
            out.buffers.push_back({f.b, a.offset, stride, a.divisor});
            out.descriptor.layouts[slot].stride = stride;
            out.descriptor.layouts[slot].stepFunction =
                a.divisor != 0 ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
            out.descriptor.layouts[slot].stepRate = std::max<NSUInteger>(a.divisor, 1);
        }
        MTLVertexAttributeDescriptor* attribute = out.descriptor.attributes[static_cast<NSUInteger>(f.location)];
        attribute.format = f.format;
        attribute.bufferIndex = slot;
        attribute.offset = a.offset - out.buffers[index].offset;
        out.key += key_of(f.location, f.format, slot, attribute.offset, stride, a.divisor);
    }
    if (kVertexBufferBase + out.buffers.size() + (out.defaults ? 1 : 0) > 31) {
        fail(kGlInvalidOperation);  // Metal's 31 buffer slots
        return false;
    }
    if (out.defaults) {
        const NSUInteger slot = kVertexBufferBase + out.buffers.size();
        for (const vkgl::CompiledProgram::Attribute& in : p.compiled.attributes) {
            MTLVertexAttributeDescriptor* attribute =
                out.descriptor.attributes[static_cast<NSUInteger>(std::max(in.location, 0))];
            if (attribute.format != MTLVertexFormatInvalid && attribute.bufferIndex == 0
                && in.location >= 0 && in.location < static_cast<int>(kAttributes)) {
                attribute.bufferIndex = slot;
            }
        }
        out.descriptor.layouts[slot].stride = 32;
        out.descriptor.layouts[slot].stepFunction = MTLVertexStepFunctionConstant;
        out.descriptor.layouts[slot].stepRate = 0;
    }
    return true;
}

id<MTLRenderPipelineState> pipeline_of(Program& p, const Target& t, const Inputs& in, int form) {
    const State& s = g->state;
    const MTLPixelFormat colour = t.colour != nil ? t.colour.pixelFormat : MTLPixelFormatInvalid;
    const MTLPixelFormat depth = t.depth != nil ? t.depth.pixelFormat : MTLPixelFormatInvalid;
    const NSUInteger samples = (t.colour != nil ? t.colour : t.depth).sampleCount;
    const bool blend = s.blend && colour != MTLPixelFormatInvalid && !is_integer(colour);
    std::string key = key_of(form, colour, depth, samples, blend, s.colour_mask);
    if (blend) {
        key += key_of(s.blend_src_rgb, s.blend_dst_rgb, s.blend_src_a, s.blend_dst_a, s.blend_equation);
    }
    key += in.key;
    if (const auto found = p.pipelines.find(key); found != p.pipelines.end()) {
        return found->second;
    }
    MTLRenderPipelineDescriptor* d = [MTLRenderPipelineDescriptor new];
    d.vertexFunction = p.functions[form][0];
    d.fragmentFunction = p.functions[form][1];
    d.vertexDescriptor = in.descriptor;
    d.rasterSampleCount = samples;
    d.depthAttachmentPixelFormat = depth;
    if (t.depth != nil && has_stencil(t.depth)) {
        d.stencilAttachmentPixelFormat = depth;
    }
    MTLRenderPipelineColorAttachmentDescriptor* c = d.colorAttachments[0];
    c.pixelFormat = colour;
    c.writeMask = (s.colour_mask[0] ? MTLColorWriteMaskRed : 0) | (s.colour_mask[1] ? MTLColorWriteMaskGreen : 0)
                  | (s.colour_mask[2] ? MTLColorWriteMaskBlue : 0) | (s.colour_mask[3] ? MTLColorWriteMaskAlpha : 0);
    if (blend) {
        c.blendingEnabled = YES;
        c.sourceRGBBlendFactor = blend_factor(s.blend_src_rgb);
        c.destinationRGBBlendFactor = blend_factor(s.blend_dst_rgb);
        c.sourceAlphaBlendFactor = blend_factor(s.blend_src_a);
        c.destinationAlphaBlendFactor = blend_factor(s.blend_dst_a);
        c.rgbBlendOperation = c.alphaBlendOperation = blend_operation(s.blend_equation);
    }
    NSError* error = nil;
    id<MTLRenderPipelineState> pipeline = [g->device newRenderPipelineStateWithDescriptor:d error:&error];
    if (pipeline == nil) {
        log::warn("metal: a pipeline: {}", error.localizedDescription.UTF8String);
    }
    p.pipelines.emplace(std::move(key), pipeline);
    return pipeline;
}

id<MTLDepthStencilState> depth_state_of(const Target& t) {
    const State& s = g->state;
    const bool depth = t.depth != nil && s.depth_test;
    const bool stencil = t.depth != nil && has_stencil(t.depth) && s.stencil_test;
    std::string key = key_of(depth, stencil);
    if (depth) {
        key += key_of(s.depth_func, s.depth_mask);
    }
    if (stencil) {
        key += key_of(s.stencil_func, s.stencil_value_mask, s.stencil_write_mask, s.stencil_front,
                      s.stencil_back);
    }
    if (const auto found = g->depth_states.find(key); found != g->depth_states.end()) {
        return found->second;
    }
    MTLDepthStencilDescriptor* d = [MTLDepthStencilDescriptor new];
    if (depth) {
        d.depthCompareFunction = compare_of(s.depth_func);
        d.depthWriteEnabled = s.depth_mask;
    }
    if (stencil) {
        auto face = [&](const StencilFace& f) {
            MTLStencilDescriptor* out = [MTLStencilDescriptor new];
            out.stencilCompareFunction = compare_of(s.stencil_func);
            out.stencilFailureOperation = stencil_operation(f.fail);
            out.depthFailureOperation = stencil_operation(f.depth_fail);
            out.depthStencilPassOperation = stencil_operation(f.pass);
            out.readMask = s.stencil_value_mask & 0xFF;
            out.writeMask = s.stencil_write_mask & 0xFF;
            return out;
        };
        d.frontFaceStencil = face(s.stencil_front);
        d.backFaceStencil = face(s.stencil_back);
    }
    id<MTLDepthStencilState> state = [g->device newDepthStencilStateWithDescriptor:d];
    g->depth_states.emplace(std::move(key), state);
    return state;
}

id<MTLSamplerState> sampler_of(const SamplerParams& p, int base_level, int max_level, bool nearest) {
    const std::string key = key_of(p.min, p.mag, p.wrap_s, p.wrap_t, p.anisotropy, base_level,
                                   max_level, nearest);
    if (const auto found = g->sampler_states.find(key); found != g->sampler_states.end()) {
        return found->second;
    }
    auto wrap = [](GLenum w) {
        return w == GL_CLAMP_TO_EDGE        ? MTLSamplerAddressModeClampToEdge
               : w == kGlMirroredRepeat ? MTLSamplerAddressModeMirrorRepeat
                                            : MTLSamplerAddressModeRepeat;
    };
    MTLSamplerDescriptor* d = [MTLSamplerDescriptor new];
    const bool min_linear = p.min == GL_LINEAR || p.min == kGlLinearMipmapNearest || p.min == GL_LINEAR_MIPMAP_LINEAR;
    d.minFilter = !nearest && min_linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    d.magFilter = !nearest && p.mag == GL_LINEAR ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    d.mipFilter = p.min == GL_NEAREST || p.min == GL_LINEAR ? MTLSamplerMipFilterNotMipmapped
                  : nearest || p.min == kGlNearestMipmapNearest || p.min == kGlLinearMipmapNearest
                      ? MTLSamplerMipFilterNearest
                      : MTLSamplerMipFilterLinear;
    d.sAddressMode = wrap(p.wrap_s);
    d.tAddressMode = wrap(p.wrap_t);
    d.maxAnisotropy = static_cast<NSUInteger>(std::clamp(p.anisotropy, 1.0f, 16.0f));
    d.lodMinClamp = static_cast<float>(base_level);
    d.lodMaxClamp = static_cast<float>(std::max(base_level, max_level));
    id<MTLSamplerState> state = [g->device newSamplerStateWithDescriptor:d];
    g->sampler_states.emplace(key, state);
    return state;
}

// A texture of `type` with nothing in it, for a unit with no texture.
id<MTLTexture> empty_texture(MTLTextureType type) {
    __strong id<MTLTexture>& t = g->empty_textures[type];
    if (t == nil) {
        if (type == MTLTextureTypeTextureBuffer) {
            id<MTLBuffer> zero = [g->device newBufferWithLength:256 options:MTLResourceStorageModeShared];
            MTLTextureDescriptor* d = [MTLTextureDescriptor
                textureBufferDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                 width:1
                                       resourceOptions:MTLResourceStorageModeShared
                                                 usage:MTLTextureUsageShaderRead];
            t = [zero newTextureWithDescriptor:d offset:0 bytesPerRow:256];
        } else {
            MTLTextureDescriptor* d = [MTLTextureDescriptor new];
            d.textureType = type;
            d.pixelFormat = MTLPixelFormatRGBA8Unorm;
            // Intel and AMD GPUs refuse shared textures, and a managed one starts undefined.
            d.storageMode = MTLStorageModeManaged;
            t = [g->device newTextureWithDescriptor:d];
            const std::uint32_t zero = 0;
            for (NSUInteger slice = 0; slice < (type == MTLTextureTypeCube ? 6 : 1); ++slice) {
                [t replaceRegion:MTLRegionMake3D(0, 0, 0, 1, 1, 1) mipmapLevel:0 slice:slice
                       withBytes:&zero bytesPerRow:4 bytesPerImage:4];
            }
        }
    }
    return t;
}

// The texels of GL_TEXTURE_BUFFER `t`; nil while it has no buffer or a format Metal lacks.
id<MTLTexture> texel_view(Texture& t) {
    Buffer* b = g->buffers.find(t.buffer);
    const Format f = format_of(t.buffer_format);
    if (b == nullptr || b->buffer == nil || f.pixel == MTLPixelFormatInvalid) {
        return nil;
    }
    b->used = g->serial;
    if (t.buffer_view == nil || t.buffer_view.buffer != b->buffer) {
        const NSUInteger width = b->size / static_cast<std::size_t>(f.bytes);
        const NSUInteger align = [g->device minimumLinearTextureAlignmentForPixelFormat:f.pixel];
        MTLTextureDescriptor* d = [MTLTextureDescriptor
            textureBufferDescriptorWithPixelFormat:f.pixel
                                             width:std::max<NSUInteger>(width, 1)
                                   resourceOptions:MTLResourceStorageModeShared
                                             usage:MTLTextureUsageShaderRead];
        const NSUInteger row = (std::max<NSUInteger>(width, 1) * static_cast<NSUInteger>(f.bytes) + align - 1) / align * align;
        t.buffer_view = [b->buffer newTextureWithDescriptor:d offset:0 bytesPerRow:row];
    }
    return t.buffer_view;
}

// The bytes of `size` at `slot` in both stages: `data`, padded with zeros past `length`.
void set_bytes(id<MTLRenderCommandEncoder> pass, NSUInteger slot, const void* data, std::size_t length,
               std::size_t size) {
    std::vector<std::uint8_t> bytes(std::max<std::size_t>(size, 16), 0);
    std::memcpy(bytes.data(), data, std::min(length, size));
    if (bytes.size() <= 4096) {
        [pass setVertexBytes:bytes.data() length:bytes.size() atIndex:slot];
        [pass setFragmentBytes:bytes.data() length:bytes.size() atIndex:slot];
    } else {
        id<MTLBuffer> b = [g->device newBufferWithBytes:bytes.data()
                                                 length:bytes.size()
                                                options:MTLResourceStorageModeShared];
        [pass setVertexBuffer:b offset:0 atIndex:slot];
        [pass setFragmentBuffer:b offset:0 atIndex:slot];
    }
}

void set_buffer(id<MTLRenderCommandEncoder> pass, NSUInteger slot, Buffer* b, std::size_t size) {
    if (b == nullptr || b->buffer == nil || b->size < size) {
        set_bytes(pass, slot, b != nullptr && b->buffer != nil ? b->buffer.contents : nullptr,
                  b != nullptr ? b->size : 0, size);
        return;
    }
    b->used = g->serial;
    [pass setVertexBuffer:b->buffer offset:0 atIndex:slot];
    [pass setFragmentBuffer:b->buffer offset:0 atIndex:slot];
}

// The program's blocks and textures at their Metal slots.
void bind_resources(id<MTLRenderCommandEncoder> pass, Program& p, const Target& t) {
    const State& s = g->state;
    const vkgl::CompiledProgram& c = p.compiled;
    // MSL rounds a block's struct up to its alignment, at most 16 bytes, past the GL size.
    const auto msl_size = [](std::size_t size) { return (size + 15) / 16 * 16; };
    for (std::size_t slot = 0; slot < p.msl.buffers.size(); ++slot) {
        const int binding = p.msl.buffers[slot];
        if (binding == c.default_binding) {
            set_bytes(pass, slot, p.uniforms.data(), p.uniforms.size(), msl_size(p.uniforms.size()));
            continue;
        }
        for (std::size_t i = 0; i < c.blocks.size(); ++i) {
            if (c.blocks[i].binding == binding) {
                const GLuint unit = p.block_bindings[i];
                set_buffer(pass, slot,
                           unit < kUniformBindings ? g->buffers.find(s.uniform_bindings[unit]) : nullptr,
                           msl_size(static_cast<std::size_t>(c.blocks[i].size)));
            }
        }
        for (const vkgl::ProgramBlock& block : c.storage) {
            if (block.binding == binding) {
                set_buffer(pass, slot,
                           static_cast<GLuint>(binding) < kUniformBindings
                               ? g->buffers.find(s.storage_bindings[binding])
                               : nullptr,
                           static_cast<std::size_t>(block.size));
            }
        }
    }
    for (std::size_t slot = 0; slot < p.msl.textures.size(); ++slot) {
        const MslTexture& mt = p.msl.textures[slot];
        const auto u = std::ranges::find_if(c.uniforms, [&](const vkgl::ProgramUniform& u) {
            return u.sampler && u.binding == mt.binding;
        });
        if (u == c.uniforms.end()) {
            continue;
        }
        const GLuint unit = static_cast<GLuint>(
            std::clamp<GLint>(p.units[static_cast<std::size_t>(u - c.uniforms.begin())], 0, kUnits - 1));
        id<MTLTexture> texture;
        MTLTextureType type = MTLTextureType2D;
        const SamplerParams* params = nullptr;
        int base = 0, max = 1000;
        Texture* tex = nullptr;
        switch (u->kind) {
            case vkgl::SamplerKind::Buffer:
                type = MTLTextureTypeTextureBuffer;
                if ((tex = g->textures.find(s.unit_buffer[unit])) != nullptr) {
                    texture = texel_view(*tex);
                }
                break;
            case vkgl::SamplerKind::Texture2DArray:
                type = MTLTextureType2DArray;
                tex = g->textures.find(s.unit_array[unit]);
                break;
            default:
                tex = g->textures.find(s.unit_2d[unit]);
                break;
        }
        if (tex != nullptr && type != MTLTextureTypeTextureBuffer) {
            texture = tex->texture;
            params = &tex->params;
            base = tex->base_level;
            max = tex->max_level;
        }
        // A texture the draw also writes reads as none: GL leaves that read undefined.
        if (texture == nil || texture == t.colour || texture == t.depth) {
            texture = empty_texture(type);
        } else {
            tex->used = g->serial;
        }
        [pass setVertexTexture:texture atIndex:slot];
        [pass setFragmentTexture:texture atIndex:slot];
        if (mt.sampler >= 0) {
            if (const SamplerParams* bound = g->samplers.find(s.unit_sampler[unit])) {
                params = bound;
            }
            static const SamplerParams kDefault;
            id<MTLSamplerState> sampler = sampler_of(params != nullptr ? *params : kDefault, base,
                                                     max, is_integer(texture.pixelFormat));
            [pass setVertexSamplerState:sampler atIndex:static_cast<NSUInteger>(mt.sampler)];
            [pass setFragmentSamplerState:sampler atIndex:static_cast<NSUInteger>(mt.sampler)];
        }
    }
}

// The pass a draw on `t` goes into: the open one when it is on `t`.
id<MTLRenderCommandEncoder> pass_on(const Target& t) {
    if (g->pass == nil || g->pass_target.colour != t.colour || g->pass_target.depth != t.depth) {
        begin_pass(t, 0);
    }
    return g->pass;
}

void draw(GLenum mode, GLint first, GLsizei count, GLsizei instances, bool indexed, GLenum index_type,
          std::uintptr_t index_offset) {
    Program* p = current_program();
    const State& s = g->state;
    bool known = false;
    const MTLPrimitiveType primitive = primitive_of(mode, known);
    if (!known) {
        fail(kGlInvalidEnum);
        return;
    }
    if (p == nullptr || !p->msl.compute.empty() || count <= 0 || instances <= 0
        || s.viewport[2] <= 0 || s.viewport[3] <= 0) {
        return;
    }
    const bool triangles = primitive == MTLPrimitiveTypeTriangle || primitive == MTLPrimitiveTypeTriangleStrip;
    if (s.cull && s.cull_face == GL_FRONT_AND_BACK && triangles) {
        return;
    }
    const Target t = target_of(s.draw_framebuffer);
    if (t.colour == nil && t.depth == nil) {
        return;
    }
    Buffer* elements = nullptr;
    if (indexed) {
        elements = g->buffers.find(vertex_array().elements);
        if (index_type != GL_UNSIGNED_SHORT && index_type != GL_UNSIGNED_INT) {
            fail(kGlInvalidEnum);  // 8-bit indices: Metal has none, and no renderer draws them
            return;
        }
        if (elements == nullptr || elements->buffer == nil) {
            return;
        }
    }
    Inputs inputs;
    if (!inputs_of(*p, inputs)) {
        return;
    }
    const int form = depth_form(s);
    id<MTLRenderPipelineState> pipeline = pipeline_of(*p, t, inputs, form);
    id<MTLRenderPipelineState> near = form >= 2 ? pipeline_of(*p, t, inputs, form + 1) : nil;
    if (pipeline == nil || (form >= 2 && near == nil)) {
        fail(kGlInvalidOperation);
        return;
    }
    id<MTLRenderCommandEncoder> pass = pass_on(t);
    const id<MTLTexture> size = t.colour != nil ? t.colour : t.depth;
    const auto width = static_cast<GLint>(size.width), height = static_cast<GLint>(size.height);
    [pass setRenderPipelineState:pipeline];
    [pass setDepthStencilState:depth_state_of(t)];
    [pass setStencilReferenceValue:static_cast<std::uint32_t>(s.stencil_ref) & 0xFF];
    [pass setBlendColorRed:s.blend_colour[0] green:s.blend_colour[1] blue:s.blend_colour[2] alpha:s.blend_colour[3]];
    [pass setCullMode:!s.cull || !triangles     ? MTLCullModeNone
                      : s.cull_face == GL_FRONT ? MTLCullModeFront
                                                : MTLCullModeBack];
    // The vertex stage turns rows over (shader_translate.h), which turns windings over with them.
    [pass setFrontFacingWinding:s.front_face == GL_CCW ? MTLWindingClockwise : MTLWindingCounterClockwise];
    [pass setTriangleFillMode:s.polygon_mode == GL_LINE ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    [pass setDepthClipMode:s.depth_clamp ? MTLDepthClipModeClamp : MTLDepthClipModeClip];
    const bool halves = form >= 2;
    MTLViewport viewport{static_cast<double>(s.viewport[0]), static_cast<double>(s.viewport[1]),
                         static_cast<double>(s.viewport[2]), static_cast<double>(s.viewport[3]),
                         halves ? 0.5 : 0.0, halves ? 0.0 : 1.0};
    [pass setViewport:viewport];
    GLint x0 = 0, y0 = 0, x1 = width, y1 = height;
    if (s.scissor_test) {
        x0 = std::clamp(s.scissor[0], 0, width);
        y0 = std::clamp(s.scissor[1], 0, height);
        x1 = std::clamp(s.scissor[0] + s.scissor[2], x0, width);
        y1 = std::clamp(s.scissor[1] + s.scissor[3], y0, height);
    }
    if (x1 == x0 || y1 == y0) {
        return;  // Metal's scissor cannot be empty
    }
    [pass setScissorRect:{static_cast<NSUInteger>(x0), static_cast<NSUInteger>(y0),
                          static_cast<NSUInteger>(x1 - x0), static_cast<NSUInteger>(y1 - y0)}];
    bind_resources(pass, *p, t);
    for (std::size_t i = 0; i < inputs.buffers.size(); ++i) {
        const Stream& stream = inputs.buffers[i];
        stream.buffer->used = g->serial;
        [pass setVertexBuffer:stream.buffer->buffer offset:stream.offset atIndex:kVertexBufferBase + i];
    }
    if (inputs.defaults) {
        if (g->attribute_defaults == nil) {
            const float floats[4] = {0, 0, 0, 1};
            const std::uint32_t integers[4] = {0, 0, 0, 1};
            std::uint8_t bytes[32];
            std::memcpy(bytes, floats, 16);
            std::memcpy(bytes + 16, integers, 16);
            g->attribute_defaults = [g->device newBufferWithBytes:bytes length:32 options:MTLResourceStorageModeShared];
        }
        [pass setVertexBuffer:g->attribute_defaults offset:0 atIndex:kVertexBufferBase + inputs.buffers.size()];
    }
    if (indexed) {
        elements->used = g->serial;
    }
    for (int half = 0; half < (halves ? 2 : 1); ++half) {
        if (half == 1) {
            [pass setRenderPipelineState:near];
            viewport.znear = 0.5;
            viewport.zfar = 1.0;
            [pass setViewport:viewport];
        }
        if (!indexed) {
            [pass drawPrimitives:primitive
                     vertexStart:static_cast<NSUInteger>(std::max(first, 0))
                     vertexCount:static_cast<NSUInteger>(count)
                   instanceCount:static_cast<NSUInteger>(instances)];
            continue;
        }
        [pass drawIndexedPrimitives:primitive
                         indexCount:static_cast<NSUInteger>(count)
                          indexType:index_type == GL_UNSIGNED_SHORT ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32
                        indexBuffer:elements->buffer
                  indexBufferOffset:index_offset
                      instanceCount:static_cast<NSUInteger>(instances)];
    }
}

}  // namespace

void MTLGL draw_arrays(GLenum mode, GLint first, GLsizei count) {
    draw(mode, first, count, 1, false, 0, 0);
}

void MTLGL draw_arrays_instanced(GLenum mode, GLint first, GLsizei count, GLsizei instances) {
    draw(mode, first, count, instances, false, 0, 0);
}

void MTLGL draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    draw(mode, 0, count, 1, true, type, reinterpret_cast<std::uintptr_t>(indices));
}

void MTLGL draw_elements_instanced(GLenum mode, GLsizei count, GLenum type, const void* indices,
                                   GLsizei instances) {
    draw(mode, 0, count, instances, true, type, reinterpret_cast<std::uintptr_t>(indices));
}

#undef MTLGL

bool is_integer(MTLPixelFormat f) {
    return f == MTLPixelFormatR32Uint || f == MTLPixelFormatRGBA32Uint;
}

}  // namespace openrac::mtlgl::detail
