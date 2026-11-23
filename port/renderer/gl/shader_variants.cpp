// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL feature variants (shader_variants.h): stable logical locations, CPU uniform values and
// exact native programs. Hooks sit ahead of statistics and state warm-up, which see native IDs.

#include "renderer/gl/shader_variants.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <format>
#include <optional>
#include <regex>
#include <span>
#include <unordered_map>
#include <utility>

#include "common/log.h"
#include "renderer/gl/program_cache.h"
#include "renderer/gl/shader_modes.h"

namespace openrac::renderer::shader_variants {
namespace {

// GL reports an absent uniform block with all bits set.
constexpr gl::GLuint kInvalidIndex = 0xFFFFFFFFu;
// GLSL vec4 and mat4 payload widths, in floats.
constexpr std::size_t kVectorComponents = 4;
constexpr std::size_t kMatrixComponents = 16;

// The GL entry points that carry logical programs, locations or execution into a native variant.
// clang-format off: one wrapped GL entry point per row.
#define OPENRAC_VARIANT_CALLS(X) \
    X(glUseProgram) \
    X(glDeleteProgram) \
    X(glGetUniformLocation) \
    X(glGetUniformBlockIndex) \
    X(glUniformBlockBinding) \
    X(glUniform1i) \
    X(glUniform1ui) \
    X(glUniform1f) \
    X(glUniform2f) \
    X(glUniform3f) \
    X(glUniform4f) \
    X(glUniform4fv) \
    X(glUniformMatrix4fv) \
    X(glDrawArrays) \
    X(glDrawArraysInstanced) \
    X(glDrawElements) \
    X(glDrawElementsInstanced)
// clang-format on

// Saved functions belong to the current context and include any previously installed observers.
struct Real {
#define OPENRAC_VARIANT_MEMBER(name) gl::PFN_##name name = nullptr;
    OPENRAC_VARIANT_CALLS(OPENRAC_VARIANT_MEMBER)
#undef OPENRAC_VARIANT_MEMBER
    gl::PFN_glDispatchCompute glDispatchCompute = nullptr;
};

// The supported uniform setters, preserving their scalar type and matrix transpose flag.
enum class Kind {
    Unwritten,
    Integer,
    Unsigned,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Matrix
};

// A typed upload retained in CPU storage, with its original order and transpose flag.
struct UniformWrite {
    Kind kind = Kind::Unwritten;
    unsigned integer = 0;       // Integer bits, including negative mode values.
    std::vector<float> values;  // Owned copies of vector and matrix arrays.
    gl::GLboolean transpose = gl::GL_FALSE;
    std::uint64_t serial = 0;
};

// One stable location, including any wider writes whose tails the latest upload did not replace.
struct Uniform {
    std::string name;                 // Authored name, including an optional array element.
    int mode = -1;                    // Feature slot, or -1 for ordinary runtime data.
    UniformWrite latest;              // The most recent upload at this location.
    std::vector<UniformWrite> wider;  // Earlier uploads that still contribute array elements.
};

// One logical uniform block; a variant may optimise it away until a feature needs it.
struct Block {
    std::string name;
    std::optional<gl::GLuint> binding;  // Explicit override. A query alone keeps the GLSL binding.
};

struct Program;

// Native handles and their location table belong to their logical program, deleted together.
struct Variant {
    gl::GLuint program = 0;
    std::vector<int> locations;  // Per logical location; populated after the first query.
    Program* owner = nullptr;    // Logical program, which outlives its native variants.
};

// One Shader's feature values and runtime data, retained across exact program switches.
struct Program {
    std::string source;  // Uncompiled template used to recognise authored uniform declarations.
    std::vector<std::string> modes;
    std::vector<int> values;
    Compiler compile;
    program_cache::Key identity;
    std::unordered_map<std::string, Variant> variants;
    std::vector<Uniform> uniforms;
    std::vector<Block> blocks;
    Variant* selected = nullptr;  // Bound variant. Map entries stay at stable addresses.
    std::uint64_t serial = 0;     // Last CPU uniform write.
    bool changed = false;         // An actual feature value changed since the preceding draw.
};

// Serialises an in-process key only; disk manifests store checked integer text instead.
std::string key_bytes(const std::vector<int>& values) {
    return {reinterpret_cast<const char*>(values.data()), values.size() * sizeof(int)};
}

}  // namespace

// Owned by Context; callbacks run on the rendering thread and borrow this registry.
struct Context::State {
    Real real;
    bool installed = false;
    bool replaying = false;
    Program* current = nullptr;  // Null while a raw program is bound.
    std::unordered_map<gl::GLuint, Program> programs;
    std::unordered_map<gl::GLuint, Variant*> native;  // Borrows variants from programs.
    Counts counts;
};

namespace {

// Borrows the registry owned by the program cache, never a separately allocated global state.
Context::State& state() {
    return program_cache::variant_context().state();
}

// Returns the logical program for intercepted GL calls outside native warm-up.
Program* current_program() {
    auto& s = state();
    return s.replaying ? nullptr : s.current;
}

// ---- Uniform state ----

// Recognises authored declarations even when a zero-valued variant has no active driver uniform.
bool declared(const Program& p, std::string_view name, bool block = false) {
    const std::string base(name.substr(0, name.find('[')));

    // GL names are identifiers; rejecting punctuation also keeps the regular expression literal.
    if (base.empty() || !std::ranges::all_of(base, shader_modes::glsl_name_char)) {
        return false;
    }

    // Blocks have a brace; loose uniforms can share a comma-separated declaration.
    const std::string pattern = block ? "\\buniform\\s+" + base + "\\s*\\{"
                                      : "\\buniform\\s+\\w+\\s+[^;{}]*\\b" + base + "\\b[^;{}]*;";

    return std::regex_search(p.source, std::regex(pattern));
}

// Resolves one stable logical location against the selected native program.
int native_location(Variant& variant, std::size_t index, const Uniform& uniform) {
    constexpr int kUnqueried = -2;  // GL's ordinary absent location is -1.
    variant.locations.resize(std::max(variant.locations.size(), index + 1), kUnqueried);
    int& location = variant.locations[index];

    // A native location is immutable for the life of this linked variant.
    if (location == kUnqueried) {
        location = state().real.glGetUniformLocation(variant.program, uniform.name.c_str());
    }

    return location;
}

// Replays a typed write using the selected variant's own driver location.
void upload(
    Variant& variant, std::size_t index, const Uniform& uniform, const UniformWrite& write
) {
    auto& r = state().real;

    // Feature constants have no native uniform, and unwritten runtime values start at GL's zero.
    if (uniform.mode >= 0 || write.kind == Kind::Unwritten) {
        return;
    }

    const int location = native_location(variant, index, uniform);

    // Inactive runtime uniforms have no destination in this exact executable.
    if (location < 0) {
        return;
    }

    const auto* data = write.values.data();

    // The setter type is preserved so integer samplers and matrices keep GL's semantics.
    switch (write.kind) {
        case Kind::Integer:
            r.glUniform1i(location, std::bit_cast<int>(write.integer));
            break;
        case Kind::Unsigned:
            r.glUniform1ui(location, write.integer);
            break;
        case Kind::Float:
            r.glUniform1f(location, data[0]);
            break;
        case Kind::Vec2:
            r.glUniform2f(location, data[0], data[1]);
            break;
        case Kind::Vec3:
            r.glUniform3f(location, data[0], data[1], data[2]);
            break;
        case Kind::Vec4:
            r.glUniform4fv(
                location, static_cast<int>(write.values.size() / kVectorComponents), data
            );
            break;
        case Kind::Matrix:
            r.glUniformMatrix4fv(
                location,
                static_cast<int>(write.values.size() / kMatrixComponents),
                write.transpose,
                data
            );
            break;
        case Kind::Unwritten:
            break;
    }
}

// Restores all CPU runtime data in write order when selecting a different native variant.
void replay(const Program& p, Variant& variant) {
    // A wider earlier upload can still supply elements beyond a later shorter write.
    struct PendingWrite {
        std::size_t index;          // Location in the logical program.
        const UniformWrite* write;  // Borrowed from that location for the duration of replay.
    };

    std::vector<PendingWrite> order;
    order.reserve(p.uniforms.size());

    // Keep writes through different array offsets in the order the renderer issued them.
    for (std::size_t n = 0; n < p.uniforms.size(); ++n) {
        const auto& uniform = p.uniforms[n];

        if (uniform.mode >= 0 || uniform.latest.kind == Kind::Unwritten) {
            continue;
        }

        for (const auto& write : uniform.wider) {
            order.push_back({n, &write});
        }

        order.push_back({n, &uniform.latest});
    }

    std::ranges::sort(order, {}, [](const PendingWrite& entry) { return entry.write->serial; });

    for (const auto& entry : order) {
        upload(variant, entry.index, p.uniforms[entry.index], *entry.write);
    }

    // Block indices can differ between variants; only explicit bindings override GLSL defaults.
    for (const Block& block : p.blocks) {
        if (!block.binding) {
            continue;
        }

        const auto index = state().real.glGetUniformBlockIndex(variant.program, block.name.c_str());

        if (index != kInvalidIndex) {
            state().real.glUniformBlockBinding(variant.program, index, *block.binding);
        }
    }
}

// ---- Exact executable selection ----

// Adds a variant to its stable owning map and indexes successful native handles.
Variant& add_variant(Context::State& s, Program& p, std::string key, gl::GLuint program) {
    auto& variant = p.variants.emplace(std::move(key), Variant{program, {}, &p}).first->second;

    // A failed compile is remembered by key but has no native GL handle.
    if (program != 0) {
        s.native.emplace(program, &variant);
    }

    return variant;
}

// Finds or compiles one exact executable, recording a failed combination only once.
Variant& exact_variant(Context::State& s, Program& p, const std::string& key) {
    const auto found = p.variants.find(key);

    // Existing combinations include failed compiles, which must not repeat every frame.
    if (found != p.variants.end()) {
        ++s.counts.reused;
        return found->second;
    }

    std::string error;
    const gl::GLuint program = p.compile(p.values, error);
    auto& variant = add_variant(s, p, key, program);
    ++s.counts.selected;

    // A refused variant never draws with a different feature program.
    if (program == 0) {
        ++s.counts.failed;
        log::error("OpenGL shader variant: {}", error);
        return variant;
    }

    program_cache::note_feature_set(p.identity, p.values);
    return variant;
}

// Selects the exact feature program before a draw; failure skips execution without a fallback.
bool prepare() {
    Program* p = current_program();

    // Programs without feature modes already have a concrete native executable.
    if (!p) {
        return true;
    }

    auto& s = state();

    // Repeated draws keep their exact native program and allocate no key storage.
    if (!p->changed) {
        ++s.counts.reused;
        return true;
    }

    const std::string key = key_bytes(p->values);
    auto& variant = exact_variant(s, *p, key);

    // A failed compile skips execution, without a generic or previously selected replacement.
    if (variant.program == 0) {
        return false;
    }

    s.real.glUseProgram(variant.program);

    // Switching executables restores uniforms that their compilers laid out independently.
    if (p->selected != &variant) {
        replay(*p, variant);
        p->selected = &variant;
    }

    p->changed = false;
    return true;
}

// Binds a logical shader without losing feature or runtime values between uses.
void OPENRAC_GLAPI watch_glUseProgram(gl::GLuint program) {
    auto& s = state();

    // Warm-up owns its native program selection and leaves logical CPU values untouched.
    if (s.replaying) {
        s.real.glUseProgram(program);
        return;
    }

    const auto found = s.native.find(program);
    s.current = found == s.native.end() ? nullptr : found->second->owner;

    // Logical uses bind the last exact executable until the next draw selects a variant.
    s.real.glUseProgram(s.current ? s.current->selected->program : program);
}

// Deletes all exact programs owned by the logical Shader and removes every native alias.
void OPENRAC_GLAPI watch_glDeleteProgram(gl::GLuint program) {
    auto& s = state();
    const auto found = s.programs.find(program);

    // Unregistered programs, including failed compiles, keep native GL ownership.
    if (found == s.programs.end()) {
        s.real.glDeleteProgram(program);
        return;
    }

    // Every successful variant belongs to this Shader until its explicit release.
    for (const auto& [key, variant] : found->second.variants) {
        if (variant.program != 0) {
            s.native.erase(variant.program);
            s.real.glDeleteProgram(variant.program);
        }
    }

    // A deleted logical shader is no longer a destination for uniform writes.
    if (s.current == &found->second) {
        s.current = nullptr;
    }

    s.programs.erase(found);
}

// ---- GL uniform hooks ----

// Returns a stable CPU location, including uniforms inactive in the initial exact variant.
gl::GLint OPENRAC_GLAPI watch_glGetUniformLocation(gl::GLuint program, const gl::GLchar* name) {
    auto& s = state();
    const auto found = s.programs.find(program);

    // Raw programs use the driver's ordinary locations.
    if (s.replaying || found == s.programs.end()) {
        return s.real.glGetUniformLocation(program, name);
    }

    auto& p = found->second;
    const auto uniform = std::ranges::find(p.uniforms, name, &Uniform::name);

    // A name's logical index remains stable across every variant selection.
    if (uniform != p.uniforms.end()) {
        return static_cast<int>(uniform - p.uniforms.begin());
    }

    // Unknown declarations retain GL's no-op location.
    if (!declared(p, name)) {
        return -1;
    }

    Uniform added;
    added.name = name;
    const auto mode = std::ranges::find(p.modes, added.name);
    // Resolve feature membership once, rather than comparing every name at each uniform write.
    added.mode = mode == p.modes.end() ? -1 : static_cast<int>(mode - p.modes.begin());
    p.uniforms.push_back(std::move(added));
    return static_cast<int>(p.uniforms.size() - 1);
}

// Returns the stable block slot rather than a variant-specific driver index.
gl::GLuint OPENRAC_GLAPI watch_glGetUniformBlockIndex(gl::GLuint program, const gl::GLchar* name) {
    auto& s = state();
    const auto found = s.programs.find(program);

    // Programs without feature modes retain their native block indices.
    if (s.replaying || found == s.programs.end()) {
        return s.real.glGetUniformBlockIndex(program, name);
    }

    auto& p = found->second;
    const auto block = std::ranges::find(p.blocks, name, &Block::name);

    if (block != p.blocks.end()) {
        return static_cast<gl::GLuint>(block - p.blocks.begin());
    }

    if (!declared(p, name, true)) {
        return kInvalidIndex;
    }

    p.blocks.push_back(Block{std::string(name), std::nullopt});
    return static_cast<gl::GLuint>(p.blocks.size() - 1);
}

// Retains logical block bindings and updates any active native block immediately.
void OPENRAC_GLAPI
watch_glUniformBlockBinding(gl::GLuint program, gl::GLuint block, gl::GLuint binding) {
    auto& s = state();
    const auto found = s.programs.find(program);

    if (s.replaying || found == s.programs.end()) {
        s.real.glUniformBlockBinding(program, block, binding);
        return;
    }

    auto& p = found->second;

    // GL_INVALID_INDEX and invalid logical indices cannot name a block.
    if (block >= p.blocks.size()) {
        return;
    }

    p.blocks[block].binding = binding;
    const gl::GLuint program_id = p.selected->program;
    const auto native = s.real.glGetUniformBlockIndex(program_id, p.blocks[block].name.c_str());

    if (native != kInvalidIndex) {
        s.real.glUniformBlockBinding(program_id, native, binding);
    }
}

// Retains a uniform's typed CPU value and updates the selected exact executable.
void remember(
    gl::GLint location,
    Kind kind,
    unsigned integer = 0,
    std::span<const float> values = {},
    gl::GLboolean transpose = gl::GL_FALSE
) {
    Program* p = current_program();

    // Negative and invalid logical locations cannot change a shader's state.
    if (!p || location < 0 || static_cast<std::size_t>(location) >= p->uniforms.size()) {
        return;
    }

    const auto index = static_cast<std::size_t>(location);
    auto& uniform = p->uniforms[index];
    // Feature writes select an exact executable and never enter the upload history.
    if (uniform.mode >= 0 && kind == Kind::Integer) {
        auto& value = p->values[static_cast<std::size_t>(uniform.mode)];
        const int next = std::bit_cast<int>(integer);
        p->changed = p->changed || value != next;
        value = next;
        return;
    }

    auto& latest = uniform.latest;

    // At the same location, a new upload supersedes only writes it fully covers.
    // The surviving writes have distinct, decreasing lengths, bounding the retained history.
    if (kind == Kind::Vec4 || kind == Kind::Matrix) {
        std::erase_if(uniform.wider, [values](const UniformWrite& write) {
            return write.values.size() <= values.size();
        });

        if (latest.values.size() > values.size()) {
            uniform.wider.push_back(std::move(latest));
        }
    }

    latest.kind = kind;
    latest.integer = integer;
    latest.serial = ++p->serial;
    latest.transpose = transpose;
    latest.values.assign(values.begin(), values.end());

    upload(*p->selected, index, uniform, latest);
}

// Captures integer mode and sampler writes while raw programs retain the native setter.
void OPENRAC_GLAPI watch_glUniform1i(gl::GLint location, gl::GLint value) {
    if (!current_program()) {
        state().real.glUniform1i(location, value);
        return;
    }

    remember(location, Kind::Integer, static_cast<unsigned>(value));
}

// Captures unsigned runtime data without interpreting it as a signed feature mode.
void OPENRAC_GLAPI watch_glUniform1ui(gl::GLint location, gl::GLuint value) {
    if (!current_program()) {
        state().real.glUniform1ui(location, value);
        return;
    }

    remember(location, Kind::Unsigned, value);
}

// Captures owned float data so caller buffers can be reused before a variant switch.
void remember_floats(
    gl::GLint location,
    Kind kind,
    std::span<const float> values,
    gl::GLboolean transpose = gl::GL_FALSE
) {
    remember(location, kind, 0, values, transpose);
}

// Preserves a scalar float across exact program switches.
void OPENRAC_GLAPI watch_glUniform1f(gl::GLint location, gl::GLfloat value) {
    if (!current_program()) {
        state().real.glUniform1f(location, value);
        return;
    }

    remember_floats(location, Kind::Float, std::span(&value, 1));
}

// Preserves a float pair across exact program switches.
void OPENRAC_GLAPI watch_glUniform2f(gl::GLint location, gl::GLfloat x, gl::GLfloat y) {
    if (!current_program()) {
        state().real.glUniform2f(location, x, y);
        return;
    }

    remember_floats(location, Kind::Vec2, std::array{x, y});
}

// Preserves a float triple across exact program switches.
void OPENRAC_GLAPI
watch_glUniform3f(gl::GLint location, gl::GLfloat x, gl::GLfloat y, gl::GLfloat z) {
    if (!current_program()) {
        state().real.glUniform3f(location, x, y, z);
        return;
    }

    remember_floats(location, Kind::Vec3, std::array{x, y, z});
}

// Preserves a float quadruple across exact program switches.
void OPENRAC_GLAPI
watch_glUniform4f(gl::GLint location, gl::GLfloat x, gl::GLfloat y, gl::GLfloat z, gl::GLfloat w) {
    if (!current_program()) {
        state().real.glUniform4f(location, x, y, z, w);
        return;
    }

    remember_floats(location, Kind::Vec4, std::array{x, y, z, w});
}

// Copies vector arrays before caller-owned memory can change.
void OPENRAC_GLAPI
watch_glUniform4fv(gl::GLint location, gl::GLsizei count, const gl::GLfloat* values) {
    if (!current_program()) {
        state().real.glUniform4fv(location, count, values);
        return;
    }

    if (count > 0 && values) {
        remember_floats(
            location, Kind::Vec4, {values, static_cast<std::size_t>(count) * kVectorComponents}
        );
    }
}

// Copies matrix arrays together with their transpose flag.
void OPENRAC_GLAPI watch_glUniformMatrix4fv(
    gl::GLint location, gl::GLsizei count, gl::GLboolean transpose, const gl::GLfloat* values
) {
    if (!current_program()) {
        state().real.glUniformMatrix4fv(location, count, transpose, values);
        return;
    }

    if (count > 0 && values) {
        remember_floats(
            location,
            Kind::Matrix,
            {values, static_cast<std::size_t>(count) * kMatrixComponents},
            transpose
        );
    }
}

// ---- GL draw hooks ----

// Selects an exact feature program before an ordinary triangle or line draw.
void OPENRAC_GLAPI watch_glDrawArrays(gl::GLenum mode, gl::GLint first, gl::GLsizei count) {
    if (prepare()) {
        state().real.glDrawArrays(mode, first, count);
    }
}

// Selects an exact feature program before an instanced array draw.
void OPENRAC_GLAPI watch_glDrawArraysInstanced(
    gl::GLenum mode, gl::GLint first, gl::GLsizei count, gl::GLsizei instances
) {
    if (prepare()) {
        state().real.glDrawArraysInstanced(mode, first, count, instances);
    }
}

// Selects an exact feature program before an indexed draw.
void OPENRAC_GLAPI
watch_glDrawElements(gl::GLenum mode, gl::GLsizei count, gl::GLenum type, const void* indices) {
    if (prepare()) {
        state().real.glDrawElements(mode, count, type, indices);
    }
}

// Selects an exact feature program before an instanced indexed draw.
void OPENRAC_GLAPI watch_glDrawElementsInstanced(
    gl::GLenum mode, gl::GLsizei count, gl::GLenum type, const void* indices, gl::GLsizei instances
) {
    if (prepare()) {
        state().real.glDrawElementsInstanced(mode, count, type, indices, instances);
    }
}

// Selects an exact feature program before compute execution.
void OPENRAC_GLAPI watch_glDispatchCompute(gl::GLuint x, gl::GLuint y, gl::GLuint z) {
    if (prepare()) {
        state().real.glDispatchCompute(x, y, z);
    }
}

// Installs once, after any existing GL observers; a new context resets the saved pointer table.
void install() {
    auto& s = state();

    if (s.installed) {
        return;
    }

#define OPENRAC_VARIANT_WRAP(name)                                                                 \
    s.real.name = gl::name;                                                                        \
    gl::name = watch_##name;
    OPENRAC_VARIANT_CALLS(OPENRAC_VARIANT_WRAP)
#undef OPENRAC_VARIANT_WRAP
    s.real.glDispatchCompute = gl::glDispatchCompute;

    // OpenGL 4.1 contexts have graphics variants but no compute entry point.
    if (gl::glDispatchCompute) {
        gl::glDispatchCompute = watch_glDispatchCompute;
    }

    s.installed = true;
}

}  // namespace

Context::Context() : m_state(std::make_unique<State>()) {}

Context::~Context() = default;

void Context::reset() {
    m_state = std::make_unique<State>();
}

Context::State& Context::state() {
    return *m_state;
}

void register_program(
    gl::GLuint program, std::string source, std::vector<std::string> modes, Compiler compiler
) {
    install();
    auto& s = state();
    auto& p = s.programs.try_emplace(program).first->second;
    p.identity = program_cache::key(source, "OpenRAC exact GL variants v1");
    p.source = std::move(source);
    p.values.assign(modes.size(), 0);
    p.modes = std::move(modes);
    p.compile = std::move(compiler);
    p.selected = &add_variant(s, p, key_bytes(p.values), program);

    // Only exact combinations observed on earlier loads are precompiled.
    for (const auto& values : program_cache::feature_sets(p.identity, p.modes.size())) {
        const std::string key = key_bytes(values);

        if (p.variants.contains(key)) {
            continue;
        }

        std::string error;
        const auto native = p.compile(values, error);

        // A cached combination may fail on this run. Leave it eligible for an exact retry on use.
        if (native == 0) {
            ++s.counts.failed;
            log::warn("OpenGL shader variant prewarm: {}", error);
            continue;
        }

        add_variant(s, p, key, native);
        ++s.counts.selected;
    }
}

int capture_warmup_bindings(
    gl::GLuint program, UniformQuery read_uniform, std::span<WarmupBinding> bindings
) {
    const auto& native = state().native;
    const auto found = native.find(program);

    // Ordinary programs have no logical bindings to capture.
    if (found == native.end()) {
        return 0;
    }

    auto& variant = *found->second;
    const auto& owner = *variant.owner;

    // Without the native query, logical aliases cannot report the effective value reliably.
    if (!read_uniform) {
        return -1;
    }

    std::size_t count = 0;
    const auto append = [&](const std::string& name, gl::GLint value, unsigned block) {
        if (count == bindings.size() || name.size() >= bindings[count].name.size()) {
            return false;
        }

        auto& binding = bindings[count++];
        std::ranges::copy(name, binding.name.begin());
        binding.value = value;
        binding.block = block;
        return true;
    };

    std::vector<gl::GLint> captured;

    // Sampler array names and their [0] spellings can share one native location.
    for (std::size_t n = 0; n < owner.uniforms.size(); ++n) {
        const auto& uniform = owner.uniforms[n];

        if (uniform.mode >= 0 || uniform.latest.kind != Kind::Integer) {
            continue;
        }

        const auto location = native_location(variant, n, uniform);

        // An alias has the same effective value and needs only one recorded assignment.
        if (location < 0 || std::ranges::find(captured, location) != captured.end()) {
            continue;
        }

        // Allow space for a mat4 if the caller passed an integer setter to the wrong uniform type.
        std::array<gl::GLint, kMatrixComponents> actual{};
        read_uniform(program, location, actual.data());

        if (!append(uniform.name, actual[0], 0)) {
            return -1;
        }

        captured.push_back(location);
    }

    for (const auto& block : owner.blocks) {
        if (block.binding
            && state().real.glGetUniformBlockIndex(program, block.name.c_str()) != kInvalidIndex
            && !append(block.name, static_cast<gl::GLint>(*block.binding), 1)) {
            return -1;
        }
    }

    return static_cast<int>(count);
}

std::string specialise(
    std::string source, const std::vector<std::string>& modes, const std::vector<int>& values
) {
    // Shared stage descriptions can name features declared in only one of the stages.
    for (std::size_t n = 0; n < modes.size(); ++n) {
        const auto [from, to] = shader_modes::declaration(source, modes[n]);

        if (from != std::string::npos) {
            const auto value = static_cast<std::uint32_t>(values[n]);
            source.replace(
                from, to - from, std::format("const int {} = int(0x{:08x}u);", modes[n], value)
            );
        }
    }

    return source;
}

ReplayScope::ReplayScope() : m_previous(state().replaying) {
    state().replaying = true;
}

ReplayScope::~ReplayScope() {
    state().replaying = m_previous;
}

Counts counts() {
    return state().counts;
}

bool owns(gl::GLuint program) {
    return state().native.contains(program);
}

}  // namespace openrac::renderer::shader_variants
