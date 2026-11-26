// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/touch_overlay.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "common/log.h"
#include "renderer/frame/window_quads.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/shader.h"
#include "renderer/ui/logo.h"
#include "renderer/ui/touch_icons.h"
#include "renderer/ui/touch_overlay_sources.h"

namespace openrac::renderer::ui::touch_overlay {
namespace {

using namespace openrac::gl;
using platform::Button;
using platform::TouchControl;

// One quad per control, its shape and symbol drawn by the fragment shader from signed distances
// in the quad's own coordinates (antialiased by the pixel's size). The options button's symbol is
// the one picture: the logo's mark (logo.h), made at the button's size. The d-pad's arms sit on
// one plate, drawn first.
using shader_sources::renderer_ui_touch_overlay::kVertex;

using shader_sources::renderer_ui_touch_overlay::kFragment;

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

struct Look {
    int symbol = kNone;
    float r = 1.0f, g = 1.0f, b = 1.0f;
    float angle = 0.0f;
    int label0 = -1, label1 = -1;
};

// Uniforms prepared for one control or the plate behind the d-pad.
struct Quad {
    float x;                   // Horizontal centre in window coordinates.
    float y;                   // Vertical centre in window coordinates.
    float radius;              // Radius in units of the window height.
    float stretch;             // Horizontal extension of a pill-shaped control.
    float reach;               // Half height of the quad, including glow, in radii.
    Look look;                 // Symbol, tint, orientation and glyphs.
    float held = 0.0f;         // Smoothed pressure used for the glow.
    float knob_x = 0.0f;       // Normalized horizontal stick displacement.
    float knob_y = 0.0f;       // Normalized vertical stick displacement.
    touch_icons::Cell icon{};  // Atlas rectangle for an action icon.
};

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
    // Reused scratch for one pass. Each quad has one matching texture name, or zero.
    std::vector<Quad> quads;
    std::vector<RasterTexture> textures;
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

// Adds the plate before the overlapping controls, in the box spanned by the d-pad's arms.
void prepare_plate(std::span<const TouchControl> controls) {
    float left = 1.0f;
    float right = 0.0f;
    float top = 1.0f;
    float bottom = 0.0f;
    float arm = 0.0f;

    // Button layouts can contain individual actions that also press a direction.
    for (const TouchControl& control : controls) {
        // Only the arms of the shared d-pad contribute to the plate.
        if (is_arm(control)) {
            left = std::min(left, control.x);
            right = std::max(right, control.x);
            top = std::min(top, control.y);
            bottom = std::max(bottom, control.y);
            arm = control.radius;
        }
    }

    // Layouts without a d-pad have no plate to draw.
    if (arm <= 0.0f) {
        return;
    }

    // The radius leaves a 20 percent margin around the arms.
    s.quads.push_back(
        {(left + right) * 0.5f,
         (top + bottom) * 0.5f,
         (bottom - top) * 0.5f + arm * 1.2f,
         1.0f,
         1.0f,
         {kDpad, 0.93f, 0.91f, 0.88f}}
    );
    s.textures.emplace_back();
}

// Uploads stale textures and prepares the complete draw list before the graph imports them.
void prepare_controls(const platform::TouchPad& touch, int height) {
    const auto controls = touch.controls();
    const auto now = std::chrono::steady_clock::now();
    // A first draw or a long pause presents the resting state immediately.
    const float dt = s.last == std::chrono::steady_clock::time_point{}
                         ? 1.0f
                         : std::min(std::chrono::duration<float>(now - s.last).count(), 1.0f);
    s.last = now;
    const float ease = 1.0f - std::exp(-dt / kPressEase);
    s.press.resize(controls.size(), 0.0f);
    s.quads.clear();
    s.textures.clear();
    glActiveTexture(GL_TEXTURE0);
    prepare_plate(controls);

    // The layout order controls overlap between neighbouring buttons.
    for (std::size_t i = 0; i < controls.size(); ++i) {
        const TouchControl& control = controls[i];
        // A hidden menu entry has no texture dependency.
        if (control.kind == TouchControl::Kind::Menu && !touch.menu_button()) {
            continue;
        }

        // Extra reach gives a pressed control's glow room outside its outline.
        Quad quad{control.x, control.y, control.radius, control.stretch, 1.2f, look(control)};
        RasterTexture texture;

        // The atlas and logo must exist when the graph validates their reads.
        if (quad.look.symbol == kIcon) {
            quad.icon = touch_icons::cell(control.icon).value_or(touch_icons::Cell{});
            texture = {touch_icons::texture(), touch_icons::mip_levels()};
        } else if (quad.look.symbol == kMenu) {
            // The rasterized mark spans twice its half height.
            const float mark_height =
                2.0f * kMarkHalf * control.radius * static_cast<float>(height);
            make_mark(static_cast<int>(std::lround(mark_height)));
            texture.name = s.mark;
        }

        float& press = s.press[i];
        const bool picked = touch.editing() && control.group == touch.selected();
        // Presses and selections show at once and releases ease out.
        press = touch.held(i) || touch.toggled(i) || picked ? 1.0f : press - press * ease;
        quad.held = press;
        touch.knob(i, quad.knob_x, quad.knob_y);
        quad.knob_x /= control.radius;
        quad.knob_y /= control.radius;
        s.quads.push_back(quad);
        s.textures.push_back(texture);
    }
}

// Binds the program once when the graph starts recording the quads.
void bind_overlay() {
    s.shader.use();
    glBindVertexArray(s.vao);
    glUniform1i(s.shader.uniform("tex_T0"), 0);
    glUniform3f(s.shader.uniform("accent"), g_accent[0], g_accent[1], g_accent[2]);
}

// Writes the prepared uniforms for one draw. The graph executor binds its texture.
void bind_quad(const Quad& quad, float aspect) {
    // Convert the window's 0..1 coordinates to clip space, with y increasing upwards.
    const float across = quad.stretch - 1.0f + quad.reach;
    glUniform4f(
        s.shader.uniform("box"),
        quad.x * 2.0f - 1.0f,
        1.0f - quad.y * 2.0f,
        quad.radius * across * 2.0f / aspect,
        quad.radius * quad.reach * 2.0f
    );
    glUniform2f(s.shader.uniform("extent"), across, quad.reach);
    glUniform1f(s.shader.uniform("stretch"), quad.stretch);

    // Icon and logo coverage use different coordinates in the same sampler.
    if (quad.look.symbol == kIcon) {
        const auto& icon = quad.icon;
        glUniform4f(s.shader.uniform("icon_rect"), icon.u0, icon.v0, icon.u1, icon.v1);
        glUniform1f(s.shader.uniform("icon_half"), kIconHalf);
    } else if (quad.look.symbol == kMenu) {
        glUniform2f(s.shader.uniform("logo_half"), kMarkHalf * s.mark_aspect, kMarkHalf);
    }

    const Look& look = quad.look;
    glUniform3f(s.shader.uniform("tint"), look.r, look.g, look.b);
    glUniform1f(s.shader.uniform("held"), quad.held);
    glUniform1i(s.shader.uniform("symbol"), look.symbol);
    glUniform1f(s.shader.uniform("angle"), look.angle);
    glUniform1i(s.shader.uniform("label0"), look.label0);
    glUniform1i(s.shader.uniform("label1"), look.label1);
    glUniform2f(s.shader.uniform("knob"), quad.knob_x, quad.knob_y);
}

}  // namespace

void draw(platform::TouchPad& touch, int width, int height) {
    // Minimized windows have no drawable to import.
    if (width <= 0 || height <= 0) {
        return;
    }

    touch.set_aspect(static_cast<float>(width) / static_cast<float>(height));

    // An invisible pad leaves the window colour untouched.
    if (!touch.visible() || !ready()) {
        return;
    }

    prepare_controls(touch, height);
    const float aspect = touch.aspect();
    draw_window_quads(
        "touch overlay", {width, height}, s.textures, bind_overlay, [aspect](std::size_t index) {
            bind_quad(s.quads[index], aspect);
        }
    );
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
