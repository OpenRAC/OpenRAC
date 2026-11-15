// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// OpenGL state warm-up (gl_warmup.h): the wrapped calls, the state file and the warm-up draws.

#include "renderer/gl/gl_warmup.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/cache_folder.h"
#include "common/joining_thread.h"
#include "common/log.h"
#include "renderer/settings/render_opt.h"

namespace openrac::renderer::gl_warmup {

using namespace openrac::gl;

namespace {

// The GL values this file asks for that renderer/gl/gl.h does not list.
constexpr GLenum kBlendSrcRgb = 0x80C9, kBlendDstRgb = 0x80C8, kBlendSrcAlpha = 0x80CB,
                 kBlendDstAlpha = 0x80CA, kBlendEquationRgb = 0x8009, kColourWritemask = 0x0C23,
                 kCurrentProgram = 0x8B8D, kDrawFramebufferBinding = 0x8CA6,
                 kReadFramebufferBinding = 0x8CAA, kVertexArrayBinding = 0x85B5,
                 kArrayBufferBinding = 0x8894, kViewport = 0x0BA2, kSamples = 0x80A9;
constexpr GLenum kAttachmentObjectType = 0x8CD0, kAttachmentRedSize = 0x8212,
                 kAttachmentGreenSize = 0x8213, kAttachmentBlueSize = 0x8214,
                 kAttachmentAlphaSize = 0x8215, kAttachmentDepthSize = 0x8216,
                 kAttachmentStencilSize = 0x8217, kAttachmentComponentType = 0x8211,
                 kAttachmentEncoding = 0x8210;
constexpr GLenum kBackLeft = 0x0402, kBack = 0x0405, kDepth = 0x1801, kStencil = 0x1802;
constexpr GLenum kNone = 0, kFloat = 0x1406, kUnsignedInt = 0x1405, kUnsignedNormalized = 0x8C17,
                 kSrgb = 0x8C40;
constexpr GLenum kAttribEnabled = 0x8622, kAttribSize = 0x8623, kAttribType = 0x8625,
                 kAttribNormalized = 0x886A, kAttribInteger = 0x88FD, kAttribDivisor = 0x88FE;
constexpr GLenum kRgba8 = 0x8058, kSrgb8Alpha8 = 0x8C43, kRgba16f = 0x881A, kRgba32f = 0x8814,
                 kRgba32ui = 0x8D70, kR32ui = 0x8236, kR32f = 0x822E, kR16f = 0x822D,
                 kRg16f = 0x822F, kR8 = 0x8229, kRg8 = 0x822B, kRgb10A2 = 0x8059,
                 kR11g11b10f = 0x8C3A, kRgba16 = 0x805B;
constexpr GLenum kDepth16 = 0x81A5, kDepth24 = 0x81A6, kDepth32f = 0x8CAC,
                 kDepth24Stencil8 = 0x88F0, kDepth32fStencil8 = 0x8CAD;
constexpr GLenum kPoints = 0x0000, kLines = 0x0001;
constexpr GLenum kFramebufferComplete = 0x8CD5;

// The vertex attributes looked at: the 16 every GL context has.
constexpr int kAttributes = 16;
// Noted states kept at most, and the runs a state may go unseen before it is dropped.
constexpr std::size_t kMostStates = 4096;
constexpr std::uint32_t kMostAge = 8;
// The file's first eight bytes, which change with its layout.
constexpr char kMagic[8] = {'O', 'R', 'G', 'L', 'W', 'U', '0', '1'};

// One vertex attribute of a noted state, as glVertexAttrib*Pointer and the divisor gave it.
struct Attribute {
    std::uint32_t index, size, type, flags, divisor;  // flags: 1 normalised, 2 integer
};

// One noted state: the program's sources, what the draw drew and into what. Its bytes are its
// key; `age` is not part of it. Plain data, written to the file as it is.
struct State {
    std::uint64_t program_name = 0, program_check = 0;  // program_cache::Key
    std::uint32_t mode = 0;  // GL_POINTS, GL_LINES or GL_TRIANGLES: the primitive's kind
    std::uint32_t blend = 0, src_rgb = 0, dst_rgb = 0, src_a = 0, dst_a = 0, equation = 0;
    std::uint32_t mask = 0;        // The colour mask, a bit per channel.
    std::uint32_t colour = 0;      // The colour attachment's internal format; 0 for none.
    std::uint32_t depth = 0;       // The depth (and stencil) attachment's; 0 for none.
    std::uint32_t samples = 0;     // The target's samples.
    std::uint32_t attributes = 0;  // How many of `attribute` hold.
    Attribute attribute[kAttributes] = {};
    std::uint32_t age = 0;  // Runs since a run drew it; not part of the key.
    std::uint32_t padding = 0;
};

constexpr std::size_t kKeyBytes = offsetof(State, age);

// The state the draws are drawn in as far as the noting needs it, followed through the calls
// that change it, so that a draw in a state already seen costs a compare.
struct Current {
    GLuint program = 0, framebuffer = 0, vertex_array = 0;
    std::uint32_t blend = 0, src_rgb = 1, dst_rgb = 0, src_a = 1, dst_a = 0, equation = 0x8006;
    std::uint32_t mask = 0xF;
    std::uint32_t mode = 0;
    bool changed = true;  // Something above changed since the last draw noted it.
};

// The functions this file stands in front of, and the queries it needs that gl.h does not list.
struct Real {
    PFN_glDrawArrays glDrawArrays = nullptr;
    PFN_glDrawArraysInstanced glDrawArraysInstanced = nullptr;
    PFN_glDrawElements glDrawElements = nullptr;
    PFN_glDrawElementsInstanced glDrawElementsInstanced = nullptr;
    PFN_glUseProgram glUseProgram = nullptr;
    PFN_glDeleteProgram glDeleteProgram = nullptr;
    PFN_glBindFramebuffer glBindFramebuffer = nullptr;
    PFN_glBindVertexArray glBindVertexArray = nullptr;
    PFN_glEnable glEnable = nullptr;
    PFN_glDisable glDisable = nullptr;
    PFN_glBlendFuncSeparate glBlendFuncSeparate = nullptr;
    PFN_glBlendEquation glBlendEquation = nullptr;
    PFN_glColorMask glColorMask = nullptr;
    void(OPENRAC_GLAPI* glGetFramebufferAttachmentParameteriv)(GLenum, GLenum, GLenum, GLint*) =
        nullptr;
    void(OPENRAC_GLAPI* glGetVertexAttribiv)(GLuint, GLenum, GLint*) = nullptr;
    void(OPENRAC_GLAPI* glDisableVertexAttribArray)(GLuint) = nullptr;
    void(OPENRAC_GLAPI* glFlush)() = nullptr;
};

// A private target warm() draws into: a framebuffer of renderbuffers of one state's formats.
struct Target {
    GLuint framebuffer = 0, colour = 0, depth = 0;
    bool complete = false;
};

struct Warmup {
    bool installed = false;
    bool replaying = false;  // warm() draws: not noted.
    Real real;
    Current current;
    std::unordered_map<GLuint, program_cache::Key> programs;  // Live programs by GL name.
    std::unordered_set<std::uint64_t> seen;  // Hashes of the Current states already noted.
    std::vector<State> states;               // Every state, in the order noted or read.
    std::unordered_map<std::string, std::size_t> index;  // A state's key bytes to its place.
    std::unordered_map<std::string, Target> targets;     // By formats and samples.
    std::filesystem::path file;                          // The driver's file; empty: none is kept.
    int added = 0;  // States noted this run and not saved since.
    Counts counts;
    JoiningThread saver;  // A save of the file while the game draws, if any.
    std::atomic<bool> saving{false};
    std::chrono::steady_clock::time_point saved_at{};
};

Warmup& w() {
    static Warmup warmup;
    return warmup;
}

std::uint64_t fnv1a(
    const void* data, std::size_t size, std::uint64_t hash = 0xCBF29CE484222325ull
) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t n = 0; n < size; ++n) {
        hash = (hash ^ bytes[n]) * 0x100000001B3ull;
    }
    return hash;
}

std::string key_bytes(const State& state) {
    return std::string(reinterpret_cast<const char*>(&state), kKeyBytes);
}

// Returns whether the warm-up is on: installed, and the registry's entry on.
bool on() {
    return w().installed && render_opt::enabled(render_opt::Opt::GlStateWarmup);
}

// The primitive kind of draw mode `mode`, which is what a driver's variant can follow.
std::uint32_t mode_kind(GLenum mode) {
    return mode == kPoints ? kPoints : mode <= 0x0003 ? kLines : GL_TRIANGLES;
}

// The internal format of a colour attachment of these channel sizes, component type and
// encoding; 0 for one this file does not make.
std::uint32_t colour_format(GLint r, GLint g, GLint b, GLint a, GLint type, GLint encoding) {
    const auto is = [&](GLint rr, GLint gg, GLint bb, GLint aa) {
        return r == rr && g == gg && b == bb && a == aa;
    };
    if (type == static_cast<GLint>(kUnsignedNormalized)) {
        return is(8, 8, 8, 8)      ? (encoding == static_cast<GLint>(kSrgb) ? kSrgb8Alpha8 : kRgba8)
               : is(10, 10, 10, 2) ? kRgb10A2
               : is(16, 16, 16, 16) ? kRgba16
               : is(8, 8, 0, 0)     ? kRg8
               : is(8, 0, 0, 0)     ? kR8
               : is(8, 8, 8, 0)     ? kRgba8
                                    : 0;
    }
    if (type == static_cast<GLint>(kFloat)) {
        return is(16, 16, 16, 16)   ? kRgba16f
               : is(32, 32, 32, 32) ? kRgba32f
               : is(11, 11, 10, 0)  ? kR11g11b10f
               : is(16, 16, 0, 0)   ? kRg16f
               : is(16, 0, 0, 0)    ? kR16f
               : is(32, 0, 0, 0)    ? kR32f
                                    : 0;
    }
    if (type == static_cast<GLint>(kUnsignedInt)) {
        return is(32, 32, 32, 32) ? kRgba32ui : is(32, 0, 0, 0) ? kR32ui : 0;
    }
    return 0;
}

// The internal format of a depth attachment of these sizes and type; 0 for none or unknown.
std::uint32_t depth_format(GLint depth, GLint stencil, GLint type) {
    if (depth == 0) {
        return 0;
    }
    if (type == static_cast<GLint>(kFloat)) {
        return stencil != 0 ? kDepth32fStencil8 : kDepth32f;
    }
    return depth == 24 ? (stencil != 0 ? kDepth24Stencil8 : kDepth24) : depth == 16 ? kDepth16 : 0;
}

// Fills `state`'s target formats from the framebuffer bound for drawing; false when its colour
// cannot be made again here.
bool read_target(State& state) {
    Real& r = w().real;
    const bool window = w().current.framebuffer == 0;
    const GLenum colour = window ? (is_es() ? kBack : kBackLeft) : GL_COLOR_ATTACHMENT0;
    const GLenum depth = window ? kDepth : GL_DEPTH_ATTACHMENT;
    const GLenum stencil = window ? kStencil : GL_STENCIL_ATTACHMENT;
    const auto ask = [&r](GLenum attachment, GLenum what) {
        GLint value = 0;
        r.glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, attachment, what, &value);
        return value;
    };

    // The colour attachment, if any.
    if (ask(colour, kAttachmentObjectType) != static_cast<GLint>(kNone)) {
        state.colour = colour_format(
            ask(colour, kAttachmentRedSize),
            ask(colour, kAttachmentGreenSize),
            ask(colour, kAttachmentBlueSize),
            ask(colour, kAttachmentAlphaSize),
            ask(colour, kAttachmentComponentType),
            ask(colour, kAttachmentEncoding)
        );

        // A format not made here.
        if (state.colour == 0) {
            return false;
        }
    }

    // The depth and stencil attachment, if any.
    if (ask(depth, kAttachmentObjectType) != static_cast<GLint>(kNone)) {
        const GLint stencil_bits = ask(stencil, kAttachmentObjectType) != static_cast<GLint>(kNone)
                                       ? ask(stencil, kAttachmentStencilSize)
                                       : 0;
        state.depth = depth_format(
            ask(depth, kAttachmentDepthSize), stencil_bits, ask(depth, kAttachmentComponentType)
        );
    }

    GLint samples = 0;
    glGetIntegerv(kSamples, &samples);
    state.samples = static_cast<std::uint32_t>(std::max(samples, 0));
    return true;
}

// Fills `state`'s vertex attributes from the vertex array bound.
void read_attributes(State& state) {
    Real& r = w().real;

    // Each enabled attribute, as its pointer was given.
    for (GLuint index = 0; index < kAttributes; ++index) {
        GLint enabled = 0;
        r.glGetVertexAttribiv(index, kAttribEnabled, &enabled);

        // Not read by the draw.
        if (enabled == 0) {
            continue;
        }

        GLint size = 0, type = 0, normalised = 0, integer = 0, divisor = 0;
        r.glGetVertexAttribiv(index, kAttribSize, &size);
        r.glGetVertexAttribiv(index, kAttribType, &type);
        r.glGetVertexAttribiv(index, kAttribNormalized, &normalised);
        r.glGetVertexAttribiv(index, kAttribInteger, &integer);
        r.glGetVertexAttribiv(index, kAttribDivisor, &divisor);
        state.attribute[state.attributes++] =
            {index,
             static_cast<std::uint32_t>(size),
             static_cast<std::uint32_t>(type),
             (normalised != 0 ? 1U : 0U) | (integer != 0 ? 2U : 0U),
             static_cast<std::uint32_t>(divisor)};
    }
}

// Writes `states` to `file`, whole: a file of its own renamed into place. Any thread.
void write_file(const std::filesystem::path& file, const std::vector<State>& states) {
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    // A name of this save's own: two programs (tests run in parallel) may save the file at once.
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto thread = std::hash<std::thread::id>{}(std::this_thread::get_id());
    const std::filesystem::path partial =
        file.string() + std::format(".{:x}{:x}.part", stamp, thread);
    {
        std::ofstream out(partial, std::ios::binary | std::ios::trunc);
        const auto count = static_cast<std::uint64_t>(states.size());
        const std::uint64_t hash = fnv1a(states.data(), states.size() * sizeof(State));
        out.write(kMagic, sizeof kMagic);
        out.write(reinterpret_cast<const char*>(&count), sizeof count);
        out.write(
            reinterpret_cast<const char*>(states.data()),
            static_cast<std::streamsize>(states.size() * sizeof(State))
        );
        out.write(reinterpret_cast<const char*>(&hash), sizeof hash);
        if (!out) {
            log::warn("OpenGL state warm-up: could not write {}", partial.string());
            std::filesystem::remove(partial, ec);
            return;
        }
    }
    std::filesystem::rename(partial, file, ec);
    if (ec) {
        log::warn("OpenGL state warm-up: could not save {}: {}", file.string(), ec.message());
        std::filesystem::remove(partial, ec);
    }
}

// Reads the driver's file into the states, each a run older; a file that is not one whole save
// is dropped.
void read_file() {
    Warmup& s = w();
    std::ifstream in(s.file, std::ios::binary);

    // The first run on this driver.
    if (!in) {
        return;
    }

    char magic[sizeof kMagic] = {};
    std::uint64_t count = 0;
    in.read(magic, sizeof magic);
    in.read(reinterpret_cast<char*>(&count), sizeof count);

    // Another layout, or a file cut short.
    if (!in || std::memcmp(magic, kMagic, sizeof magic) != 0 || count > kMostStates) {
        log::info(
            "OpenGL state warm-up: {} is of another build or damaged, dropped", s.file.string()
        );
        return;
    }

    std::vector<State> states(count);
    std::uint64_t hash = 0;
    in.read(
        reinterpret_cast<char*>(states.data()), static_cast<std::streamsize>(count * sizeof(State))
    );
    in.read(reinterpret_cast<char*>(&hash), sizeof hash);

    // Not one whole save.
    if (!in || hash != fnv1a(states.data(), states.size() * sizeof(State))) {
        log::info("OpenGL state warm-up: {} is damaged, dropped", s.file.string());
        return;
    }

    // Each state a run older; one too old, or out of range, stays out.
    for (State& state : states) {
        if (state.age >= kMostAge || state.attributes > kAttributes) {
            continue;
        }
        ++state.age;
        if (s.index.emplace(key_bytes(state), s.states.size()).second) {
            s.states.push_back(state);
        }
    }

    log::info("OpenGL state warm-up: {} states read from {}", s.states.size(), s.file.string());
}

// Saves the states on a thread of their own, unless a save is still writing.
void save_soon() {
    Warmup& s = w();

    // Nothing to keep, nothing new, or a save still under way.
    if (s.file.empty() || s.added == 0 || s.saving.load()) {
        return;
    }

    s.added = 0;
    s.saved_at = std::chrono::steady_clock::now();
    s.saver.join();
    s.saving = true;
    s.saver = JoiningThread([file = s.file, states = s.states] {
        write_file(file, states);
        w().saving = false;
    });
}

// Notes the state of a draw of `mode` if it is one not seen this run. The hot path is one test.
void note(GLenum mode) {
    Warmup& s = w();
    Current& c = s.current;
    const std::uint32_t kind = mode_kind(mode);

    // The same state as the last draw, or a warm-up draw.
    if ((!c.changed && c.mode == kind) || s.replaying) {
        return;
    }

    c.mode = kind;
    c.changed = false;

    // Off, or a program the warm-up does not know (not built by Shader::build).
    if (!on()) {
        return;
    }

    const auto program = s.programs.find(c.program);
    if (program == s.programs.end()) {
        return;
    }

    // Seen this run.
    const std::array<std::uint32_t, 11> fields{
        c.program,
        c.framebuffer,
        c.vertex_array,
        c.blend,
        c.src_rgb,
        c.dst_rgb,
        c.src_a,
        c.dst_a,
        c.equation,
        c.mask,
        c.mode,
    };
    if (!s.seen.insert(fnv1a(fields.data(), sizeof fields)).second) {
        return;
    }

    State state{};
    state.program_name = program->second.name;
    state.program_check = program->second.check;
    state.mode = kind;
    state.blend = c.blend;
    state.src_rgb = c.blend != 0 ? c.src_rgb : 0;
    state.dst_rgb = c.blend != 0 ? c.dst_rgb : 0;
    state.src_a = c.blend != 0 ? c.src_a : 0;
    state.dst_a = c.blend != 0 ? c.dst_a : 0;
    state.equation = c.blend != 0 ? c.equation : 0;
    state.mask = c.mask;

    // A target of a format not made here: nothing to warm it with.
    if (!read_target(state)) {
        return;
    }

    read_attributes(state);
    std::string bytes = key_bytes(state);

    // Known from an earlier run: drawn again in this one.
    if (const auto found = s.index.find(bytes); found != s.index.end()) {
        s.states[found->second].age = 0;
        return;
    }

    // Full: older states are not pushed out by a run's new ones.
    if (s.states.size() >= kMostStates) {
        return;
    }

    s.index.emplace(std::move(bytes), s.states.size());
    s.states.push_back(state);
    ++s.added;
    ++s.counts.noted;

    // Kept while the game goes on too, so a run that does not end cleanly keeps what it drew.
    if (std::chrono::steady_clock::now() - s.saved_at >= std::chrono::seconds(30)) {
        save_soon();
    }
}

// ---- The watching functions ----

void OPENRAC_GLAPI watch_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    note(mode);
    w().real.glDrawArrays(mode, first, count);
}

void OPENRAC_GLAPI watch_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei n) {
    note(mode);
    w().real.glDrawArraysInstanced(mode, first, count, n);
}

void OPENRAC_GLAPI watch_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* at) {
    note(mode);
    w().real.glDrawElements(mode, count, type, at);
}

void OPENRAC_GLAPI
watch_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* at, GLsizei n) {
    note(mode);
    w().real.glDrawElementsInstanced(mode, count, type, at, n);
}

void OPENRAC_GLAPI watch_glUseProgram(GLuint program) {
    Current& c = w().current;
    c.changed = c.changed || c.program != program;
    c.program = program;
    w().real.glUseProgram(program);
}

void OPENRAC_GLAPI watch_glDeleteProgram(GLuint program) {
    w().programs.erase(program);
    w().real.glDeleteProgram(program);
}

void OPENRAC_GLAPI watch_glBindFramebuffer(GLenum target, GLuint framebuffer) {
    Current& c = w().current;
    // The read framebuffer alone draws nothing.
    if (target != GL_READ_FRAMEBUFFER) {
        c.changed = c.changed || c.framebuffer != framebuffer;
        c.framebuffer = framebuffer;
    }
    w().real.glBindFramebuffer(target, framebuffer);
}

void OPENRAC_GLAPI watch_glBindVertexArray(GLuint vertex_array) {
    Current& c = w().current;
    c.changed = c.changed || c.vertex_array != vertex_array;
    c.vertex_array = vertex_array;
    w().real.glBindVertexArray(vertex_array);
}

void OPENRAC_GLAPI watch_glEnable(GLenum what) {
    if (what == GL_BLEND) {
        Current& c = w().current;
        c.changed = c.changed || c.blend != 1;
        c.blend = 1;
    }
    w().real.glEnable(what);
}

void OPENRAC_GLAPI watch_glDisable(GLenum what) {
    if (what == GL_BLEND) {
        Current& c = w().current;
        c.changed = c.changed || c.blend != 0;
        c.blend = 0;
    }
    w().real.glDisable(what);
}

void OPENRAC_GLAPI
watch_glBlendFuncSeparate(GLenum src_rgb, GLenum dst_rgb, GLenum src_a, GLenum dst_a) {
    Current& c = w().current;
    c.changed = c.changed || c.src_rgb != src_rgb || c.dst_rgb != dst_rgb || c.src_a != src_a
                || c.dst_a != dst_a;
    c.src_rgb = src_rgb;
    c.dst_rgb = dst_rgb;
    c.src_a = src_a;
    c.dst_a = dst_a;
    w().real.glBlendFuncSeparate(src_rgb, dst_rgb, src_a, dst_a);
}

void OPENRAC_GLAPI watch_glBlendEquation(GLenum equation) {
    Current& c = w().current;
    c.changed = c.changed || c.equation != equation;
    c.equation = equation;
    w().real.glBlendEquation(equation);
}

void OPENRAC_GLAPI watch_glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    Current& c = w().current;
    const std::uint32_t mask =
        (r != 0 ? 1U : 0U) | (g != 0 ? 2U : 0U) | (b != 0 ? 4U : 0U) | (a != 0 ? 8U : 0U);
    c.changed = c.changed || c.mask != mask;
    c.mask = mask;
    w().real.glColorMask(r, g, b, a);
}

// ---- The warm-up ----

// Returns the private target of `state`'s formats and samples, made the first time.
Target& target_of(const State& state) {
    Warmup& s = w();
    const std::string key = std::format("{}/{}/{}", state.colour, state.depth, state.samples);
    auto [at, added] = s.targets.try_emplace(key);
    Target& t = at->second;

    // Made before, complete or not.
    if (!added) {
        return t;
    }

    constexpr GLsizei kSide = 4;
    const auto samples = static_cast<GLsizei>(state.samples);
    const auto storage = [samples](GLenum format) {
        if (samples > 1) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, kSide, kSide);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, format, kSide, kSide);
        }
    };
    glGenFramebuffers(1, &t.framebuffer);
    s.real.glBindFramebuffer(GL_FRAMEBUFFER, t.framebuffer);
    if (state.colour != 0) {
        glGenRenderbuffers(1, &t.colour);
        glBindRenderbuffer(GL_RENDERBUFFER, t.colour);
        storage(state.colour);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, t.colour);
    }
    if (state.depth != 0) {
        const bool stencil = state.depth == kDepth24Stencil8 || state.depth == kDepth32fStencil8;
        glGenRenderbuffers(1, &t.depth);
        glBindRenderbuffer(GL_RENDERBUFFER, t.depth);
        storage(state.depth);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER,
            t.depth
        );
    }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    t.complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == kFramebufferComplete;
    return t;
}

// What warm() changes, read before and given back after.
struct Saved {
    GLint program = 0, draw = 0, read = 0, vertex_array = 0, array_buffer = 0;
    GLint viewport[4] = {};
    GLint blend = 0, src_rgb = 0, dst_rgb = 0, src_a = 0, dst_a = 0, equation = 0;
    GLint mask[4] = {};
};

Saved save_state() {
    Saved v;
    glGetIntegerv(kCurrentProgram, &v.program);
    glGetIntegerv(kDrawFramebufferBinding, &v.draw);
    glGetIntegerv(kReadFramebufferBinding, &v.read);
    glGetIntegerv(kVertexArrayBinding, &v.vertex_array);
    glGetIntegerv(kArrayBufferBinding, &v.array_buffer);
    glGetIntegerv(kViewport, v.viewport);
    glGetIntegerv(GL_BLEND, &v.blend);
    glGetIntegerv(kBlendSrcRgb, &v.src_rgb);
    glGetIntegerv(kBlendDstRgb, &v.dst_rgb);
    glGetIntegerv(kBlendSrcAlpha, &v.src_a);
    glGetIntegerv(kBlendDstAlpha, &v.dst_a);
    glGetIntegerv(kBlendEquationRgb, &v.equation);
    glGetIntegerv(kColourWritemask, v.mask);
    return v;
}

// Gives back `v`, through the watching functions, so that what they follow is right again.
void restore_state(const Saved& v) {
    glUseProgram(static_cast<GLuint>(v.program));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(v.draw));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(v.read));
    glBindVertexArray(static_cast<GLuint>(v.vertex_array));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(v.array_buffer));
    glViewport(v.viewport[0], v.viewport[1], v.viewport[2], v.viewport[3]);
    if (v.blend != 0) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glBlendFuncSeparate(
        static_cast<GLenum>(v.src_rgb),
        static_cast<GLenum>(v.dst_rgb),
        static_cast<GLenum>(v.src_a),
        static_cast<GLenum>(v.dst_a)
    );
    glBlendEquation(static_cast<GLenum>(v.equation));
    glColorMask(
        static_cast<GLboolean>(v.mask[0] != 0),
        static_cast<GLboolean>(v.mask[1] != 0),
        static_cast<GLboolean>(v.mask[2] != 0),
        static_cast<GLboolean>(v.mask[3] != 0)
    );
}

// Draws `state` once with program `program`, into its private target: one triangle (or line, or
// point) of vertices that read zeros, which covers no pixel unless the program places it alone.
void warm_one(const State& state, GLuint program, GLuint buffer) {
    Target& t = target_of(state);

    // Formats this driver cannot draw into: nothing to warm.
    if (!t.complete) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, t.framebuffer);
    glViewport(0, 0, 4, 4);
    glUseProgram(program);
    if (state.blend != 0) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(state.src_rgb, state.dst_rgb, state.src_a, state.dst_a);
        glBlendEquation(state.equation);
    } else {
        glDisable(GL_BLEND);
    }
    glColorMask(
        static_cast<GLboolean>((state.mask & 1U) != 0),
        static_cast<GLboolean>((state.mask & 2U) != 0),
        static_cast<GLboolean>((state.mask & 4U) != 0),
        static_cast<GLboolean>((state.mask & 8U) != 0)
    );

    // The attributes in the noted formats, all reading the zeros of `buffer`.
    GLuint vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    for (std::uint32_t n = 0; n < state.attributes; ++n) {
        const Attribute& a = state.attribute[n];
        const auto size = static_cast<GLint>(a.size);
        if ((a.flags & 2U) != 0) {
            glVertexAttribIPointer(a.index, size, a.type, 0, nullptr);
        } else {
            glVertexAttribPointer(a.index, size, a.type, (a.flags & 1U) != 0, 0, nullptr);
        }
        glVertexAttribDivisor(a.index, a.divisor);
        glEnableVertexAttribArray(a.index);
    }
    const GLsizei count = state.mode == kPoints ? 1 : state.mode == kLines ? 2 : 3;
    glDrawArrays(state.mode, 0, count);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vertex_array);
    ++w().counts.warmed;
}

}  // namespace

void install(GetProcAddress get) {
    Warmup& s = w();

    // Once; and never on the Vulkan backend, whose pipelines are made ahead on their own.
    if (s.installed || !loaded() || is_vulkan()) {
        return;
    }

    s.real.glGetFramebufferAttachmentParameteriv =
        reinterpret_cast<decltype(s.real.glGetFramebufferAttachmentParameteriv)>(
            get("glGetFramebufferAttachmentParameteriv")
        );
    s.real.glGetVertexAttribiv =
        reinterpret_cast<decltype(s.real.glGetVertexAttribiv)>(get("glGetVertexAttribiv"));
    s.real.glFlush = reinterpret_cast<decltype(s.real.glFlush)>(get("glFlush"));

    // A context without the queries: nothing can be noted.
    if (s.real.glGetFramebufferAttachmentParameteriv == nullptr
        || s.real.glGetVertexAttribiv == nullptr || s.real.glFlush == nullptr) {
        log::info("OpenGL state warm-up: the context lacks its queries, off");
        return;
    }

    s.installed = true;
#define OPENRAC_GL_WARMUP_WRAP(name)                                                               \
    s.real.name = gl::name;                                                                        \
    gl::name = watch_##name;
    OPENRAC_GL_WARMUP_WRAP(glDrawArrays)
    OPENRAC_GL_WARMUP_WRAP(glDrawArraysInstanced)
    OPENRAC_GL_WARMUP_WRAP(glDrawElements)
    OPENRAC_GL_WARMUP_WRAP(glDrawElementsInstanced)
    OPENRAC_GL_WARMUP_WRAP(glUseProgram)
    OPENRAC_GL_WARMUP_WRAP(glDeleteProgram)
    OPENRAC_GL_WARMUP_WRAP(glBindFramebuffer)
    OPENRAC_GL_WARMUP_WRAP(glBindVertexArray)
    OPENRAC_GL_WARMUP_WRAP(glEnable)
    OPENRAC_GL_WARMUP_WRAP(glDisable)
    OPENRAC_GL_WARMUP_WRAP(glBlendFuncSeparate)
    OPENRAC_GL_WARMUP_WRAP(glBlendEquation)
    OPENRAC_GL_WARMUP_WRAP(glColorMask)
#undef OPENRAC_GL_WARMUP_WRAP

    // The driver's file: its states hold its formats and what its compiles follow.
    const auto text = [](GLenum what) {
        const auto* got = reinterpret_cast<const char*>(glGetString(what));
        return std::string(got != nullptr ? got : "");
    };
    const std::string driver = text(GL_VENDOR) + '\n' + text(GL_RENDERER) + '\n' + text(GL_VERSION);
    s.file = cache_folder() / "gl_states"
             / std::format("{:016x}.bin", fnv1a(driver.data(), driver.size()));
    s.saved_at = std::chrono::steady_clock::now();
    read_file();
}

void note_program(GLuint program, const program_cache::Key& key) {
    // Not watching: nothing is noted.
    if (!w().installed) {
        return;
    }

    w().programs[program] = key;
}

void warm() {
    Warmup& s = w();

    // Off, or nothing noted on this driver yet.
    if (!on()) {
        return;
    }

    const auto start = std::chrono::steady_clock::now();
    const int before = s.counts.warmed;
    std::unordered_map<std::uint64_t, GLuint> live;  // The programs built now, by their sources.
    for (const auto& [program, key] : s.programs) {
        live[key.name ^ (key.check * 0x9E3779B97F4A7C15ull)] = program;
    }

    // Zeros for every attribute to read: 16 vec4 of 32-bit values at most, three vertices.
    const std::vector<std::uint8_t> zeros(16 * 16 * 4, 0);
    GLuint buffer = 0;
    const Saved saved = save_state();
    s.replaying = true;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(zeros.size()), zeros.data(), GL_STATIC_DRAW
    );

    // Each state of a program built now.
    for (const State& state : s.states) {
        const auto program =
            live.find(state.program_name ^ (state.program_check * 0x9E3779B97F4A7C15ull));
        if (program != live.end()) {
            warm_one(state, program->second, buffer);
        }
    }

    glDeleteBuffers(1, &buffer);
    restore_state(saved);
    s.real.glFlush();
    s.replaying = false;
    // The watching functions saw restore_state's calls: the next draw looks again.
    s.current.changed = true;
    const int warmed = s.counts.warmed - before;

    // Said with the time it took, which the loading screen covered.
    if (warmed != 0) {
        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count();
        log::info(
            "OpenGL state warm-up: {} states drawn at the level's load in {:.0f} ms", warmed, ms
        );
    }

    save_soon();
}

Counts counts() {
    return w().counts;
}

}  // namespace openrac::renderer::gl_warmup
