// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/touch_overlay.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "common/log.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/ui/logo.h"
#include "renderer/ui/touch_icons.h"

namespace openrac::renderer::ui::touch_overlay {
namespace {

using namespace openrac::gl;
using platform::Button;
using platform::TouchControl;

// One quad per control, its shape and symbol drawn by the fragment shader from signed distances
// in the quad's own coordinates (antialiased by the pixel's size). The options button's symbol is
// the one picture: the logo's mark (logo.h), made at the button's size. The d-pad's arms sit on
// one plate, drawn first.
constexpr const char* kVertex = R"glsl(#version 410 core
uniform vec4 box;     // the control's box in device coordinates: centre x, y, half width, half height
uniform vec2 extent;  // its half width and height in radii
out vec2 v_at;        // across the box in radii from the centre, down positive
void main() {
    vec2 corner = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;
    v_at = vec2(corner.x, -corner.y) * extent;
    gl_Position = vec4(box.xy + corner * box.zw, 0.0, 1.0);
}
)glsl";

constexpr const char* kFragment = R"glsl(#version 410 core
in vec2 v_at;
uniform vec3 tint;     // the symbol's colour
uniform float held;    // 1 while a finger holds it
uniform float stretch; // half the width over half the height
uniform int symbol;    // see Symbol in touch_overlay.cpp
uniform vec2 knob;     // a stick's knob, -1..1 of the circle
uniform float angle;   // an arrow's direction, in radians clockwise from up
uniform int label0;    // two letters (letter()), -1 for none
uniform int label1;
uniform sampler2D tex_T0;  // the logo's mark, its coverage in alpha; or the icons (touch_icons.h)
uniform vec2 logo_half;    // the mark's half width and height in radii
uniform vec4 icon_rect;    // an icon's cell in the icons' texture: u0, v0, u1, v1
uniform float icon_half;   // its half size in radii
uniform vec3 accent;       // the game's accent (the launcher's, ui_colors.h): a held control's light
out vec4 colour;

// smoothstep() is undefined with equal edges: where d is flat across a pixel (fwidth 0, far from
// any shape, d = 1e3) some drivers return garbage, so the edge keeps a width.
float fill(float d) { float w = max(fwidth(d) * 0.75, 1e-5); return 1.0 - smoothstep(-w, w, d); }
float pill(vec2 p, float half_width, float r) {
    return length(vec2(max(abs(p.x) - half_width, 0.0), p.y)) - r;
}
float segment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));
}

vec4 over(vec4 under, vec4 top) {
    float a = top.a + under.a * (1.0 - top.a);
    return vec4(a > 0.0 ? (top.rgb * top.a + under.rgb * under.a * (1.0 - top.a)) / a : vec3(0.0), a);
}
// Every button, round or a pill as wide as `stretch` says: a see-through slate face in the middle
// of a darker base, a clear rim, a light along the top edge. A press shrinks the face a little and
// warms it.
const vec3 kSlateBase = vec3(0.075, 0.075, 0.085);
const vec3 kSlateTop = vec3(0.205, 0.205, 0.235);
const vec3 kSlateBottom = vec3(0.150, 0.150, 0.172);
vec4 slate(vec2 p, float d) {
    vec4 c = vec4(kSlateBase, 0.38 * fill(d));
    c = over(c, vec4(mix(vec3(1.0), accent, 0.6 * held), (0.34 + 0.4 * held) * fill(abs(d + 0.025) - 0.022)));  // the rim
    float f = d + 0.12 + 0.04 * held;
    vec3 face = mix(kSlateTop, kSlateBottom, clamp(p.y * 0.5 + 0.5, 0.0, 1.0));
    face = mix(face, accent * 0.5, 0.30 * held);
    c = over(c, vec4(face, (0.42 + 0.25 * held) * fill(f)));
    float edge = fill(abs(f + 0.025) - 0.016) * (1.0 - smoothstep(-0.2, 0.3, p.y));
    c = over(c, vec4(vec3(1.0), 0.09 * edge));
    return c;
}

// A letter of the shoulders' names in strokes, in a box 1 wide and 2 high centred on p's origin:
// 0 L, 1 R, 2 one, 3 two.
float letter(int code, vec2 p) {
    float d = 1e3;
    if (code == 0) {
        d = min(segment(p, vec2(-0.35, -1.0), vec2(-0.35, 1.0)), segment(p, vec2(-0.35, 1.0), vec2(0.4, 1.0)));
    } else if (code == 1) {
        d = min(segment(p, vec2(-0.4, -1.0), vec2(-0.4, 1.0)), segment(p, vec2(-0.4, -1.0), vec2(0.0, -1.0)));
        d = min(d, segment(p, vec2(-0.4, 0.0), vec2(0.0, 0.0)));
        d = min(d, p.x > 0.0 ? abs(length(p - vec2(0.0, -0.5)) - 0.5) : 1e3);
        d = min(d, segment(p, vec2(0.0, 0.0), vec2(0.45, 1.0)));
    } else if (code == 2) {
        d = min(segment(p, vec2(0.05, -1.0), vec2(0.05, 1.0)), segment(p, vec2(-0.3, -0.65), vec2(0.05, -1.0)));
        d = min(d, segment(p, vec2(-0.3, 1.0), vec2(0.4, 1.0)));
    } else if (code == 3) {
        vec2 c = vec2(0.0, -0.45);
        d = (p.y < c.y || p.x > 0.0) && p.y < 0.0 ? abs(length(p - c) - 0.45) : 1e3;
        d = min(d, segment(p, vec2(0.35, -0.2), vec2(-0.42, 1.0)));
        d = min(d, segment(p, vec2(-0.42, 1.0), vec2(0.45, 1.0)));
    }
    return d;
}

void main() {
    vec2 p = v_at;
    float press = 1.0 - 0.06 * held;  // a held control sinks a little
    vec4 c = vec4(0.0);
    vec4 ink = vec4(tint, 0.92);
    if (symbol == 9) {
        // The d-pad's round base, as the stick's.
        float d = length(p) - 1.0;
        c = vec4(kSlateBase, 0.36 * fill(d));
        c = over(c, vec4(vec3(1.0), 0.30 * fill(abs(d + 0.02) - 0.018)));
    } else if (symbol == 6) {
        // An arm of the d-pad: a key pointed toward the middle, an arrow on it.
        float ca = cos(angle), sa = sin(angle);
        vec2 q = mat2(ca, -sa, sa, ca) * p / press;
        // The pad's middle is 1.5 radii inward (kArm): the keys' outer ends follow the base's circle.
        float key = max(max(abs(q.x) - 0.80, length(q - vec2(0.0, 1.5)) - 2.45),
                        (q.y + abs(q.x) - 1.02) * 0.7071);
        vec3 face = mix(kSlateTop, kSlateBottom, clamp(q.y * 0.5 + 0.5, 0.0, 1.0));
        face = mix(face, accent * 0.5, 0.30 * held);
        c = vec4(face, (0.55 + 0.2 * held) * fill(key));
        c = over(c, vec4(mix(vec3(1.0), accent, 0.6 * held), (0.30 + 0.4 * held) * fill(abs(key + 0.03) - 0.02)));
        vec2 t = q + vec2(0.0, 0.45);
        float tri = max(abs(t.x) * 0.9 - t.y * 0.45, t.y - 0.14) - 0.17;
        c = over(c, vec4(mix(vec3(0.93, 0.91, 0.88), accent, held), 0.9 * fill(tri)));
    } else if (symbol == 5) {
        // The stick: a round base, an arrow each way (the one it leans to lit), a round
        // knob with four dots.
        float d = length(p) - 1.0;
        c = vec4(kSlateBase, 0.36 * fill(d));
        c = over(c, vec4(vec3(1.0), 0.30 * fill(abs(d + 0.02) - 0.018)));
        c = over(c, vec4(mix(vec3(0.165, 0.165, 0.19), vec3(0.13, 0.13, 0.15), clamp(p.y * 0.5 + 0.5, 0.0, 1.0)), 0.38 * fill(d + 0.08)));
        c = over(c, vec4(vec3(1.0), 0.06 * fill(abs(d + 0.10) - 0.012) * (1.0 - smoothstep(-0.3, 0.3, p.y))));
        float lean = clamp(length(knob), 0.0, 1.0);
        for (int i = 0; i < 4; ++i) {
            float a = float(i) * 1.5708;
            vec2 dir = vec2(sin(a), -cos(a));
            vec2 t = vec2(dot(p, vec2(-dir.y, dir.x)), dot(p, dir) - 0.74);
            float tri = max(abs(t.x) * 0.95 + t.y * 0.55, -t.y - 0.05) - 0.055;
            float toward = lean * max(dot(normalize(knob + vec2(1e-5)), dir), 0.0);
            vec3 ink = mix(vec3(0.30, 0.30, 0.34), accent, smoothstep(0.3, 0.9, toward));
            c = over(c, vec4(ink, (0.9) * fill(tri)));
        }
        vec2 k = p - knob * 0.50;
        float kr = 0.46;
        float kd = length(k) - kr;
        c = over(c, vec4(kSlateBase, 0.55 * fill(kd)));
        vec2 kf = k;
        vec3 face = mix(vec3(0.235, 0.235, 0.27), vec3(0.18, 0.18, 0.205), clamp(kf.y / kr * 0.5 + 0.5, 0.0, 1.0));
        face = mix(face, accent * 0.6, 0.12 * held);
        c = over(c, vec4(face, 0.65 * fill(kd + kr * 0.14)));
        c = over(c, vec4(vec3(1.0), 0.08 * fill(abs(kd + kr * 0.16) - 0.012) * (1.0 - smoothstep(-0.2, 0.15, kf.y / kr))));
        for (int i = 0; i < 4; ++i) {
            float a = float(i) * 1.5708;
            c = over(c, vec4(vec3(0.36, 0.36, 0.40), 0.9 * fill(length(kf - vec2(sin(a), -cos(a)) * 0.17) - 0.035)));
        }
    } else {
        // A button: slate, its symbol or icon on it.
        float d = pill(p, stretch - 1.0, 1.0);
        vec2 q = p;
        c = slate(p, d);
        float s = 1e3;
        if (symbol == 1) {         // triangle
            vec2 t = q + vec2(0.0, -0.08);
            float e = max(abs(t.x) * 0.866 - t.y * 0.5, t.y) - 0.27;
            s = abs(e) - 0.045;
        } else if (symbol == 2) {  // circle
            s = abs(length(q) - 0.40) - 0.06;
        } else if (symbol == 3) {  // cross
            vec2 r = abs(q);
            s = min(segment(r, vec2(0.0), vec2(0.34, 0.34)), 1e3) - 0.065;
        } else if (symbol == 4) {  // square
            vec2 r = abs(q);
            s = abs(max(r.x, r.y) - 0.31) - 0.055;
        } else if (symbol == 7) {  // Select: a bar
            s = pill(q, 0.30, 0.13);
            ink.rgb = vec3(0.92);
        } else if (symbol == 8) {  // Start: a play arrow
            vec2 t = vec2(q.y, -q.x) + vec2(0.0, -0.04);
            s = max(abs(t.x) * 0.9 - t.y * 0.45, t.y - 0.16) - 0.20;
            ink.rgb = vec3(0.92);
        }
        // The port's options: the logo's mark, its coverage read from the picture.
        float mark = -1.0;
        if (symbol == 10) {
            vec2 uv = q / logo_half * 0.5 + 0.5;
            bool inside = all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)));
            float read = texture(tex_T0, clamp(uv, 0.0, 1.0)).a;  // outside any branch: see below
            mark = inside ? read : 0.0;
            ink.rgb = vec3(0.92);
        }
        // An action's icon: white ink and the accent's, their coverage read from the icons' texture.
        vec4 icon = vec4(0.0);
        if (symbol == 11) {
            // Read for every pixel, then masked: read only inside the square, the mip level at its
            // edge came from broken derivatives and drew a thin square around the icon (Adreno).
            vec2 uv = q / icon_half * 0.5 + 0.5;
            vec2 cover = texture(tex_T0, mix(icon_rect.xy, icon_rect.zw, clamp(uv, 0.0, 1.0))).rg;
            if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)))) {
                vec3 tint = mix(vec3(0.96), accent, cover.g / max(cover.r + cover.g, 1e-4));
                icon = vec4(tint, max(cover.r, cover.g));
            }
        }
        if (label0 >= 0) {        // the shoulders: their name
            const float size = 0.38;  // half a letter's height
            vec2 at = q / size;
            s = min(letter(label0, at + vec2(0.8, 0.0)), letter(label1, at - vec2(0.8, 0.0))) * size - 0.07;
            ink.rgb = vec3(mix(mp_vec3(0.95), mp_vec3(accent), mp_float(0.5) * mp_float(held)));
        }
        // The symbol: a faint shadow under it on a pill (the shoulders), then the ink.
        if (stretch > 1.0) {
            c = over(c, vec4(vec3(0.0), 0.45 * ink.a * fill(s - 0.02) * (1.0 - fill(s))));
        }
        ink.a *= 0.75 + 0.25 * held;
        if (symbol == 11) {
            c = over(c, vec4(icon.rgb, icon.a * (0.88 + 0.12 * held)));
        } else {
            c = over(c, vec4(ink.rgb, ink.a * (mark >= 0.0 ? mark : fill(s))));
        }
        // The glow of a press, outside the outline.
        c = over(vec4(accent, 0.25 * held * (1.0 - smoothstep(0.0, 0.18, d)) * step(0.0, d)), c);
    }
    if (c.a <= 0.002) { discard; }
    colour = c;
}
)glsl";

// What the fragment shader draws for a control.
enum Symbol : int {
    kNone = 0,
    kTriangle = 1,
    kCircle = 2,
    kCross = 3,
    kSquare = 4,
    kStick = 5,
    kArrow = 6,
    kSelect = 7,
    kStart = 8,
    kDpad = 9,
    kMenu = 10,
    kIcon = 11,
};

// The shoulders' names, letter by letter (letter() in the shader).
constexpr int kGlyphL = 0;
constexpr int kGlyphR = 1;
constexpr int kGlyph1 = 2;
constexpr int kGlyph2 = 3;

struct State {
    renderer::Shader shader;
    GLuint vao = 0;
    bool failed = false;
    // How far each control is pressed in: all the way at once when a finger lands, so that a
    // press shows in the frame it is made, then easing back out when it lifts (kPressEase); and
    // when the pad was last drawn.
    std::vector<float> press;
    std::chrono::steady_clock::time_point last{};
    // The logo's mark for the options button, and the height in pixels it was made at.
    GLuint mark = 0;
    int mark_height = 0;
    float mark_aspect = 1.0f;
};

// The mark's half height on the options button, in radii: as large as the face buttons' symbols.
constexpr float kMarkHalf = 0.54f;
// An action's icon's half size in radii: most of the button, a rim of slate around it.
constexpr float kIconHalf = 0.66f;

// The seconds a let-go control takes to ease most of the way out.
constexpr float kPressEase = 0.06f;

State s;
// The accent (ui_colors.h's amber): the launcher's own until the game says.
float g_accent[3] = {1.0f, 0.706f, 0.263f};

struct Look {
    int symbol = kNone;
    float r = 1.0f, g = 1.0f, b = 1.0f;
    float angle = 0.0f;
    int label0 = -1, label1 = -1;
};

Look look(const TouchControl& c) {
    if (!c.icon.empty() && touch_icons::has(c.icon)) {
        return {kIcon, 1.0f, 1.0f, 1.0f};
    }
    if (c.kind == TouchControl::Kind::Menu) {
        return {kMenu, 0.95f, 0.92f, 0.88f};
    }
    if (c.kind != TouchControl::Kind::Button) {
        return {kStick, 1.0f, 0.56f, 0.14f};
    }
    // The DualShock's own colours, a little brighter over a game.
    switch (c.button) {
        case Button::Triangle: return {kTriangle, 0.35f, 0.92f, 0.78f};
        case Button::Circle: return {kCircle, 1.0f, 0.42f, 0.45f};
        case Button::Cross: return {kCross, 0.55f, 0.72f, 1.0f};
        case Button::Square: return {kSquare, 1.0f, 0.55f, 0.88f};
        case Button::Up: return {kArrow, 0.93f, 0.91f, 0.88f, 0.0f};
        case Button::Right: return {kArrow, 0.93f, 0.91f, 0.88f, 1.5708f};
        case Button::Down: return {kArrow, 0.93f, 0.91f, 0.88f, 3.1416f};
        case Button::Left: return {kArrow, 0.93f, 0.91f, 0.88f, -1.5708f};
        case Button::L1: return {kNone, 0.95f, 0.92f, 0.88f, 0.0f, kGlyphL, kGlyph1};
        case Button::L2: return {kNone, 0.95f, 0.92f, 0.88f, 0.0f, kGlyphL, kGlyph2};
        case Button::R1: return {kNone, 0.95f, 0.92f, 0.88f, 0.0f, kGlyphR, kGlyph1};
        case Button::R2: return {kNone, 0.95f, 0.92f, 0.88f, 0.0f, kGlyphR, kGlyph2};
        case Button::Select: return {kSelect, 0.95f, 0.92f, 0.88f};
        case Button::Start: return {kStart, 0.95f, 0.92f, 0.88f};
        default: return {kNone, 0.9f, 0.9f, 0.9f};
    }
}

// The options button's mark made `height` pixels high, white with the logo's coverage as alpha,
// when it is not that size yet.
void make_mark(int height) {
    height = std::max(height, 8);
    if (s.mark != 0 && s.mark_height == height) {
        return;
    }
    const logo::Mask mask = logo::draw(height, false);
    std::vector<std::uint8_t> rgba(mask.alpha.size() * 4, 255);
    for (std::size_t i = 0; i < mask.alpha.size(); ++i) {
        rgba[i * 4 + 3] = mask.alpha[i];
    }
    if (s.mark == 0) {
        glGenTextures(1, &s.mark);
    }
    glBindTexture(GL_TEXTURE_2D, s.mark);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(GL_RGBA8),
        mask.width,
        mask.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgba.data()
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glBindTexture(GL_TEXTURE_2D, 0);
    s.mark_height = height;
    s.mark_aspect = static_cast<float>(mask.width) / static_cast<float>(mask.height);
}

// An arm of the DualShock's d-pad; an action button that presses the d-pad is a button of its own.
bool is_arm(const TouchControl& c) {
    return c.kind == TouchControl::Kind::Button && c.action.empty()
           && (c.button == Button::Up || c.button == Button::Down || c.button == Button::Left
               || c.button == Button::Right);
}

bool ready() {
    if (s.failed) {
        return false;
    }
    if (s.vao == 0) {
        std::string error;
        if (!s.shader.build("touch overlay", kVertex, kFragment, error)) {
            log::warn("the on-screen pad cannot be drawn: {}", error);
            s.failed = true;
            return false;
        }
        glGenVertexArrays(1, &s.vao);
    }
    return true;
}

}  // namespace

void draw(platform::TouchPad& touch, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    touch.set_aspect(static_cast<float>(width) / static_cast<float>(height));
    if (!touch.visible() || !ready()) {
        return;
    }
    // All of the state the pad's quads depend on, whatever the frame or picture under it left.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    s.shader.use();
    glBindVertexArray(s.vao);
    glUniform3f(s.shader.uniform("accent"), g_accent[0], g_accent[1], g_accent[2]);
    const float aspect = touch.aspect();
    const auto controls = touch.controls();
    const auto now = std::chrono::steady_clock::now();
    const float dt = s.last == std::chrono::steady_clock::time_point{}
                         ? 1.0f
                         : std::min(std::chrono::duration<float>(now - s.last).count(), 1.0f);
    s.last = now;
    const float ease = 1.0f - std::exp(-dt / kPressEase);
    s.press.resize(controls.size(), 0.0f);
    // A control's box, in window units (0..1 down, the radius in heights), to device coordinates,
    // with `reach` radii around its centre (more than 1 leaves room for a press's glow).
    const auto box = [&](float x, float y, float radius, float stretch, float reach) {
        const float across = stretch - 1.0f + reach;
        glUniform4f(s.shader.uniform("box"), x * 2.0f - 1.0f, 1.0f - y * 2.0f, radius * across * 2.0f / aspect,
                    radius * reach * 2.0f);
        glUniform2f(s.shader.uniform("extent"), across, reach);
        glUniform1f(s.shader.uniform("stretch"), stretch);
    };
    // The d-pad's plate, under its arms: the box the four of them span.
    float left = 1.0f, right = 0.0f, top = 1.0f, bottom = 0.0f, arm = 0.0f;
    for (const TouchControl& c : controls) {
        if (is_arm(c)) {
            left = std::min(left, c.x);
            right = std::max(right, c.x);
            top = std::min(top, c.y);
            bottom = std::max(bottom, c.y);
            arm = c.radius;
        }
    }
    if (arm > 0.0f) {
        box((left + right) * 0.5f, (top + bottom) * 0.5f, (bottom - top) * 0.5f + arm * 1.2f, 1.0f, 1.0f);
        glUniform3f(s.shader.uniform("tint"), 0.93f, 0.91f, 0.88f);
        glUniform1f(s.shader.uniform("held"), 0.0f);
        glUniform1i(s.shader.uniform("symbol"), kDpad);
        glUniform1f(s.shader.uniform("angle"), 0.0f);
        glUniform1i(s.shader.uniform("label0"), -1);
        glUniform1i(s.shader.uniform("label1"), -1);
        glUniform2f(s.shader.uniform("knob"), 0.0f, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    for (std::size_t i = 0; i < controls.size(); ++i) {
        const TouchControl& c = controls[i];
        if (c.kind == TouchControl::Kind::Menu && !touch.menu_button()) {
            continue;
        }
        const Look l = look(c);
        if (l.symbol == kIcon) {
            const auto cell = touch_icons::cell(c.icon);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, touch_icons::texture());
            glUniform4f(
                s.shader.uniform("icon_rect"),
                cell ? cell->u0 : 0.0f,
                cell ? cell->v0 : 0.0f,
                cell ? cell->u1 : 0.0f,
                cell ? cell->v1 : 0.0f
            );
            glUniform1f(s.shader.uniform("icon_half"), kIconHalf);
        }
        if (l.symbol == kMenu) {
            const float mark_height = 2.0f * kMarkHalf * c.radius * static_cast<float>(height);
            make_mark(static_cast<int>(std::lround(mark_height)));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, s.mark);
            glUniform2f(s.shader.uniform("logo_half"), kMarkHalf * s.mark_aspect, kMarkHalf);
        }
        box(c.x, c.y, c.radius, c.stretch, 1.2f);
        glUniform3f(s.shader.uniform("tint"), l.r, l.g, l.b);
        float& press = s.press[i];
        // While the layout is edited, the control picked to be resized lit as if held.
        const bool picked = touch.editing() && c.group == touch.selected();
        press = touch.held(i) || touch.toggled(i) || picked ? 1.0f : press - press * ease;
        glUniform1f(s.shader.uniform("held"), press);
        glUniform1i(s.shader.uniform("symbol"), l.symbol);
        glUniform1f(s.shader.uniform("angle"), l.angle);
        glUniform1i(s.shader.uniform("label0"), l.label0);
        glUniform1i(s.shader.uniform("label1"), l.label1);
        float dx = 0.0f;
        float dy = 0.0f;
        touch.knob(i, dx, dy);
        glUniform2f(s.shader.uniform("knob"), dx / c.radius, dy / c.radius);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void set_accent(float r, float g, float b) {
    g_accent[0] = r;
    g_accent[1] = g;
    g_accent[2] = b;
}

void release() {
    touch_icons::release();
    s.shader.release();
    if (s.vao != 0) {
        glDeleteVertexArrays(1, &s.vao);
    }
    if (s.mark != 0) {
        glDeleteTextures(1, &s.mark);
    }
    s = State{};
}

}  // namespace openrac::renderer::ui::touch_overlay
