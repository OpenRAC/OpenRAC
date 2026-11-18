// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay (debug_overlay.h): what the environment asks of it, its initialisation, the
// scripted keys, the panel, the window's screenshots and the entry points, which draw the rest from
// debug/ (debug/overlay.h). Its frame-time window, with its plot, its pause and its single step,
// follows OpenGOAL's (game/graphics/opengl_renderer/debug_gui.cpp, FrameTimeRecorder; ISC License,
// Copyright (c) 2020-2026 OpenGOAL Team), and the per-pass table its profiler's (Profiler.cpp):
// name, time, draw calls, triangles; the panel's sections are in debug/frame.cpp and
// debug/renderer.cpp.

#include "debug_overlay.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/env.h"
#include "common/log.h"
#include "common/stats.h"
#include "debug/menu_ui.h"
#include "debug/overlay.h"
#include "debug/ui_scale.h"
#include "frontend.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "renderer/gl/gl.h"
#include "renderer/gl/gl_stats.h"
#include "renderer/ui/backdrop_blur.h"
#include "renderer/ui/imgui_gl.h"
#include "renderer/ui/logo.h"
#include "scene/screenshot.h"
#include "settings.h"

#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::frontend::debug_overlay {

namespace {

// ---- The environment ----

// Returns the value `var` was set to as a string of its own, or nothing when it is unset. For the
// reads below that walk the value as the C library does, up to its NUL, which env::value's view
// need not have.
std::optional<std::string> env_string(env::Var var) {
    const std::optional<std::string_view> found = env::value(var);

    // An unset variable stays nothing, which differs from the empty string.
    if (!found) {
        return std::nullopt;
    }

    return std::string(*found);
}

void configure() {
    if (s.configured) {
        return;
    }
    s.configured = true;
    s.developer = env::flag(env::Var::Developer);
    // OPENRAC_MENU=TAB[@TICK]: the menu's tab when it opens, and the tick of the game it opens at
    // (screenshots, checks of it); without a tick, its key opens it.
    if (const std::optional<std::string_view> v = env::value(env::Var::Menu); v && !v->empty()) {
        const std::string_view tab = v->substr(0, v->find('@'));
        for (int t = 0; t < static_cast<int>(menu::Tab::Program); ++t) {
            std::string name = menu::tab_name(static_cast<menu::Tab>(t));
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (name == tab) {
                s.menu_tab = t;
            }
        }
        if (const std::size_t at = v->find('@'); at != std::string_view::npos) {
            s.menu_open_at = std::atoll(std::string(v->substr(at + 1)).c_str());
        }
    } else {
        notify_menu_hint();  // what opens the menu, at the start
    }
    // OPENRAC_OVERLAY: shown from the start; a list of names opens those sections ("all": every
    // one), "inspect" selects the hero and opens the inspector, "labels" names the mobys in view,
    // "memview" opens the memory viewer.
    if (const std::optional<std::string_view> v = env::value(env::Var::Overlay); v && *v != "0") {
        const std::string list(*v);
        s.wanted = list != "meat-proxy-review";
        const auto has = [&](const char* word) {
            std::size_t at = 0;
            const std::size_t n = std::strlen(word);
            while ((at = list.find(word, at)) != std::string::npos) {
                const bool starts = at == 0 || list[at - 1] == ',';
                const bool ends = at + n == list.size() || list[at + n] == ',';
                if (starts && ends) {
                    return true;
                }
                at += n;
            }
            return false;
        };
        bool any = false;
        std::array<bool, kSectionCount> open{};
        for (std::size_t i = 0; i < kSectionCount; ++i) {
            open[i] = has(kSectionNames[i]) || has("all");
            any = any || open[i];
        }
        if (any) {
            s.open_at_start = open;
        }
        s.labels = has("labels");
        s.object_viewer.open = s.developer && has("object-viewer");
        s.meat_proxy_review = s.developer && has("meat-proxy-review");
        set_picker_enabled(has("picker") || s.object_viewer.open);
        s.wireframe = has("wireframe");
        s.memory_viewer = has("memview");
        if (has("inspect")) {
            s.inspector = true;
            s.follow_hero = true;
        }
    }
    // OPENRAC_HIDE_PASSES=NAME,...: the renderer's passes of those names are not drawn (the names of
    // the Renderer section's rows: sky, terrain, ties, shrubs, mobys, effects, hud).
    if (const std::optional<std::string> v = env_string(env::Var::HidePasses)) {
        std::string name;
        for (const char* at = v->c_str();; ++at) {
            if (*at == ',' || *at == '\0') {
                if (!name.empty()) {
                    s.hidden.insert(name);
                }
                name.clear();
                if (*at == '\0') {
                    break;
                }
            } else {
                name += *at;
            }
        }
    }
    // OPENRAC_KEYS=FRAME[+HELD]:KEY,...: keys pressed by script at those window frames, as
    // OPENRAC_PRESS presses pad buttons: KEY is SDL's name of a key (F3, F5, Return, ...), or
    // click@X,Y for a left click at that place of the window (in its own units, not pixels).
    // +HELD: that many pictures later while the game is paused at FRAME, where frames do not count.
    if (const auto value = env::value(env::Var::Keys)) {
        configure_scripted_input(*value);
    }
    // OPENRAC_OVERLAY_SHOT=N:PREFIX: every N ticks the window, overlay included, to PREFIX<tick>.png
    // (from OPENRAC_SHOT_FROM on, as OPENRAC_SHOT's).
    if (const std::optional<std::string> v = env_string(env::Var::OverlayShot)) {
        const char* colon = std::strchr(v->c_str(), ':');
        s.shot_every = std::strtoull(v->c_str(), nullptr, 10);
        s.shot_prefix = colon != nullptr ? colon + 1 : "overlay_";

        // Read with strtoull, as OPENRAC_SHOT's is (frontend.cpp), not env::integer's strtoll.
        if (const std::optional<std::string> from = env_string(env::Var::ShotFrom)) {
            s.shot_from = std::strtoull(from->c_str(), nullptr, 10);
        }
    }
    // OPENRAC_PAUSE_AT=TICK[:FRAMES]: the game pauses at that tick, and goes on by itself after
    // FRAMES pictures of it when given (for scripted checks of the pause).
    if (const std::optional<std::string> v = env_string(env::Var::PauseAt)) {
        s.pause_at = std::atoll(v->c_str());
        const char* colon = std::strchr(v->c_str(), ':');
        s.held_frames = colon != nullptr ? std::atoi(colon + 1) : 0;
    }
    // OPENRAC_SHADER_RELOAD=TICK: every shader program is read again from its files at that tick
    // (what the Shaders section's button does), with the result in the log.
    s.reload_at = env::integer(env::Var::ShaderReload, s.reload_at);
    // OPENRAC_TIME_SCALE=X: the game's speed (0.25: quarter speed), from 1/64 to 1.
    {
        // Unset reads as 0, which, as any value not above 0, leaves the speed as it is.
        const double scale = env::number(env::Var::TimeScale, 0.0);
        if (scale > 0.0) {
            s.stretch = std::clamp(1.0 / scale, 1.0, 64.0);
        }
    }
}

// The overlay's scale for the window now (debug/ui_scale.h): 1 on a 1080p desktop, 2 on 4K.
float window_ui_scale(SDL_Window* window) {
    int height = 0;
#if !defined(__ANDROID__)
    if (const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window))) {
        height = mode->h;
    }
    constexpr bool desktop = true;
#else
    constexpr bool desktop = false;
#endif
    return ui_scale(
        SDL_GetWindowDisplayScale(window),
        SDL_GetWindowPixelDensity(window),
        height,
        desktop,
        settings::get().menu_size
    );
}

// The overlay's windows (the panel, Enhanced Settings, the inspectors) in the
// launcher's colours, as the menu wears them (debug/menu_ui.h): its panels, hairlines and accent.
void launcher_style(ImGuiStyle& style) {
    const auto c = [](ui_colors::Color colour, float alpha = 1.0f) {
        ImVec4 v = ImGui::ColorConvertU32ToFloat4(colour);
        v.w *= alpha;
        return v;
    };
    const ui_colors::Colors& u = ui();
    // Keep developer windows darker and their controls distinct from the surrounding panels.
    const auto panel = ui_colors::mix(u.panel, u.ink_deep, 0.3f);
    const auto panel_hi = ui_colors::mix(u.panel_hi, u.ink_deep, 0.35f);
    const auto control = ui_colors::mix(u.control_card, u.text, 0.07f);
    const auto control_hi = ui_colors::mix(u.control_card_hi, u.text, 0.12f);
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = c(u.text);
    colors[ImGuiCol_TextDisabled] = c(u.dim);
    colors[ImGuiCol_WindowBg] = c(panel, 0.98f);
    colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_PopupBg] = c(panel, 0.99f);
    colors[ImGuiCol_Border] = c(u.line);
    colors[ImGuiCol_FrameBg] = c(control);
    colors[ImGuiCol_FrameBgHovered] = c(control_hi);
    colors[ImGuiCol_FrameBgActive] = c(ui_colors::mix(u.panel, u.amber, 0.18f));
    colors[ImGuiCol_TitleBg] = c(u.ink_deep);
    colors[ImGuiCol_TitleBgActive] = c(panel_hi);
    colors[ImGuiCol_TitleBgCollapsed] = c(u.ink_deep, 0.8f);
    colors[ImGuiCol_MenuBarBg] = c(panel_hi);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ScrollbarGrab] = c(u.line_strong);
    colors[ImGuiCol_ScrollbarGrabHovered] = c(u.dim);
    colors[ImGuiCol_ScrollbarGrabActive] = c(u.soft);
    colors[ImGuiCol_CheckMark] = c(u.amber);
    colors[ImGuiCol_SliderGrab] = c(u.amber);
    colors[ImGuiCol_SliderGrabActive] = c(u.gold);
    colors[ImGuiCol_Button] = c(ui_colors::mix(u.control_card, u.text, 0.1f));
    colors[ImGuiCol_ButtonHovered] = c(control_hi);
    colors[ImGuiCol_ButtonActive] = c(ui_colors::mix(control, u.amber, 0.3f));
    colors[ImGuiCol_Header] = c(ui_colors::mix(u.panel_hi, u.amber, 0.14f));
    colors[ImGuiCol_HeaderHovered] = c(ui_colors::mix(u.panel_hi, u.amber, 0.26f));
    colors[ImGuiCol_HeaderActive] = c(ui_colors::mix(u.panel_hi, u.amber, 0.38f));
    colors[ImGuiCol_Separator] = c(u.line);
    colors[ImGuiCol_SeparatorHovered] = c(u.amber, 0.6f);
    colors[ImGuiCol_SeparatorActive] = c(u.amber);
    colors[ImGuiCol_ResizeGrip] = c(u.line);
    colors[ImGuiCol_ResizeGripHovered] = c(u.amber, 0.6f);
    colors[ImGuiCol_ResizeGripActive] = c(u.amber);
    colors[ImGuiCol_Tab] = c(control);
    colors[ImGuiCol_TabHovered] = c(ui_colors::mix(u.panel_hi, u.amber, 0.26f));
    colors[ImGuiCol_TabSelected] = c(ui_colors::mix(u.panel, u.amber, 0.15f));
    colors[ImGuiCol_TabSelectedOverline] = c(u.amber, 0.0f);
    colors[ImGuiCol_TabDimmed] = c(control);
    colors[ImGuiCol_TabDimmedSelected] = c(ui_colors::mix(u.panel, u.amber, 0.10f));
    colors[ImGuiCol_PlotLines] = c(u.soft);
    colors[ImGuiCol_PlotLinesHovered] = c(u.amber);
    colors[ImGuiCol_PlotHistogram] = c(u.brand);
    colors[ImGuiCol_PlotHistogramHovered] = c(u.amber);
    colors[ImGuiCol_TableHeaderBg] = c(panel_hi);
    colors[ImGuiCol_TableBorderStrong] = c(u.line_strong);
    colors[ImGuiCol_TableBorderLight] = c(u.line);
    colors[ImGuiCol_TableRowBgAlt] = c(u.text, 0.03f);
    colors[ImGuiCol_TextSelectedBg] = c(u.amber, 0.3f);
    colors[ImGuiCol_DragDropTarget] = c(u.amber);
    colors[ImGuiCol_NavCursor] = c(u.amber);
    colors[ImGuiCol_ModalWindowDimBg] = c(u.ink_deep, 0.6f);
}

// The style and the font at s.ui_scale, from ImGui's defaults each time (a scale change rebuilds
// them). The overlay's windows wear the menu's look: Inter, rounded frames, roomy rows; on a touch
// screen larger still, for a finger.
void style_and_font() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark();
    launcher_style(style);
    // The launcher's corners: 12 px for windows and panels, 6 px for frames, tabs and grabs. No
    // borders: a fill sets each apart (the site's STYLE.md).
    style.WindowRounding = 12.0f;
    style.ChildRounding = 12.0f;
    style.PopupRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.WindowPadding = ImVec2(12.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 5.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(6.0f, 3.0f);
    style.IndentSpacing = 16.0f;
    style.SeparatorTextBorderSize = 1.0f;
    const bool touch = drawer_possible();
    if (touch) {
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        // Buttons, boxes and headers roomy for a finger, and reached a little beyond.
        style.WindowPadding = ImVec2(16.0f, 14.0f);
        style.FramePadding = ImVec2(12.0f, 9.0f);
        style.ItemSpacing = ImVec2(10.0f, 8.0f);
        style.CellPadding = ImVec2(8.0f, 5.0f);
        style.TouchExtraPadding = ImVec2(6.0f, 6.0f);
        style.ScrollbarSize = 20.0f;
        style.GrabMinSize = 22.0f;
    }
    style.ScaleAllSizes(s.ui_scale);
    io.Fonts->Clear();
    // ImGui's own, kept only for a build without the menu's typefaces.
    ImFontConfig config;
    config.SizePixels = std::round(13.0f * s.ui_scale);
    io.Fonts->AddFontDefault(&config);
    add_menu_fonts(touch);
    io.FontDefault = menu_fonts.panel;
}

// The logo for the drawer's header, drawn once into ImGui's font atlas before the atlas goes to
// the GPU, so that ImGui draws it as it draws text, with no texture of its own.
void add_logo() {
    ImGuiIO& io = ImGui::GetIO();
    const renderer::ui::logo::Mask mask =
        renderer::ui::logo::draw(static_cast<int>(std::lround(dp(30.0f))), true);
    const int rect = io.Fonts->AddCustomRectRegular(mask.width, mask.height);
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    const ImFontAtlasCustomRect* at = io.Fonts->GetCustomRectByIndex(rect);
    if (pixels == nullptr || at == nullptr || !at->IsPacked()) {
        return;
    }
    for (int y = 0; y < mask.height; ++y) {
        for (int x = 0; x < mask.width; ++x) {
            unsigned char* p =
                pixels + (static_cast<std::size_t>(at->Y + y) * width + at->X + x) * 4;
            p[0] = p[1] = p[2] = 255;
            p[3] = mask.alpha[static_cast<std::size_t>(y) * mask.width + x];
        }
    }
    s.logo_rect = rect;
    s.logo_name_x = mask.name_x;
}

// The controllers as ImGui's gamepad: buttons and d-pad only. The sticks move Ratchet and the
// camera and are too easy to nudge, so they do not move through the menus. While a button is
// being bound, and until every button is let go after it, ImGui gets nothing: the press that was
// bound must not also press a row or close the menu.
//
// The d-pad reaches ImGui as the arrow keys: a gamepad's move starts again from the window's
// visible area whenever the row in focus is outside it (ImGui's NavUpdateCreateMoveRequest), and
// then a held Down past the last row, while the rows still scroll, jumped back to the top. The
// keyboard's moves do not do that. Arrows are sent only as the d-pad changes, so the keyboard's
// own arrows still work.
void feed_gamepads() {
    struct Button {
        ImGuiKey key;
        SDL_GamepadButton button;
        bool arrow = false;                          // sent as a key of the keyboard
        SDL_Scancode key_of = SDL_SCANCODE_UNKNOWN;  // that key's, on a real keyboard
    };

    static constexpr Button kButtons[] = {
        {ImGuiKey_GamepadStart, SDL_GAMEPAD_BUTTON_START},
        {ImGuiKey_GamepadBack, SDL_GAMEPAD_BUTTON_BACK},
        {ImGuiKey_GamepadFaceDown, SDL_GAMEPAD_BUTTON_SOUTH},
        {ImGuiKey_GamepadFaceRight, SDL_GAMEPAD_BUTTON_EAST},
        {ImGuiKey_GamepadFaceLeft, SDL_GAMEPAD_BUTTON_WEST},
        {ImGuiKey_GamepadFaceUp, SDL_GAMEPAD_BUTTON_NORTH},
        {ImGuiKey_UpArrow, SDL_GAMEPAD_BUTTON_DPAD_UP, true, SDL_SCANCODE_UP},
        {ImGuiKey_DownArrow, SDL_GAMEPAD_BUTTON_DPAD_DOWN, true, SDL_SCANCODE_DOWN},
        {ImGuiKey_LeftArrow, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true, SDL_SCANCODE_LEFT},
        {ImGuiKey_RightArrow, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true, SDL_SCANCODE_RIGHT},
        {ImGuiKey_GamepadL1, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
        {ImGuiKey_GamepadR1, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    };
    constexpr Sint16 kTriggerDown = 8000;
    std::array<bool, std::size(kButtons)> down{};
    bool l2 = false;
    bool r2 = false;
    bool any = false;
    int count = 0;
    if (SDL_JoystickID* ids = SDL_GetGamepads(&count)) {
        for (int i = 0; i < count; ++i) {
            SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
            if (pad == nullptr) {
                continue;
            }
            any = true;
            for (std::size_t b = 0; b < std::size(kButtons); ++b) {
                down[b] = down[b] || SDL_GetGamepadButton(pad, kButtons[b].button);
            }
            l2 = l2 || SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > kTriggerDown;
            r2 = r2 || SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > kTriggerDown;
        }
        SDL_free(ids);
    }
    const bool held = l2 || r2 || std::find(down.begin(), down.end(), true) != down.end();
    // A press awaited for a binding or for the menu's button belongs to the capture: the menu
    // ignores the controller until every button is let go after it.
    if (controls_capturing() || s.menu_button_capture) {
        s.pad_locked = true;
    } else if (!held) {
        s.pad_locked = false;
    }
    ImGuiIO& io = ImGui::GetIO();
    const bool* keyboard = SDL_GetKeyboardState(nullptr);
    for (std::size_t b = 0; b < std::size(kButtons); ++b) {
        const bool on = !s.pad_locked && down[b];
        if (!kButtons[b].arrow) {
            io.AddKeyEvent(kButtons[b].key, on);
            continue;
        }
        // The d-pad as the arrows, which the keyboard also presses: held while either holds it,
        // and let go once neither does, whatever came between (a frame not drawn, the focus lost,
        // a release ImGui never saw). ImGui keeps a key held until told otherwise, so an arrow
        // told down and not up again went on moving the focus by itself.
        const bool held_key = keyboard != nullptr && keyboard[kButtons[b].key_of]
                              && SDL_GetKeyboardFocus() != nullptr;
        const bool want = on || held_key;
        if (ImGui::IsKeyDown(kButtons[b].key) != want) {
            io.AddKeyEvent(kButtons[b].key, want);
        }
    }
    io.AddKeyEvent(ImGuiKey_GamepadL2, !s.pad_locked && l2);
    io.AddKeyEvent(ImGuiKey_GamepadR2, !s.pad_locked && r2);
    io.BackendFlags = any ? io.BackendFlags | ImGuiBackendFlags_HasGamepad
                          : io.BackendFlags & ~ImGuiBackendFlags_HasGamepad;
}

bool initialise(SDL_Window* window) {
    if (s.initialised) {
        return true;
    }
    // ImGui's OpenGL backend loads the system's GL; a window drawn with Vulkan has none, and has the
    // renderer's GL instead (imgui_gl.h).
    s.gl_subset = SDL_GL_GetCurrentContext() == nullptr;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // nothing of the overlay is written beside the game
    io.LogFilename = nullptr;
    s.ui_scale = window_ui_scale(window);
    style_and_font();
    add_logo();
    std::string error;
    const bool started =
        s.gl_subset ? ImGui_ImplSDL3_InitForOther(window) && renderer::ui::imgui_gl::init(error)
                    : ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext())
                          && ImGui_ImplOpenGL3_Init(
                              openrac::gl::is_es() ? "#version 300 es" : "#version 410 core"
                          );
    // The controllers reach ImGui through feed_gamepads, not the backend.
    ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_Manual, nullptr, 0);
    if (!started) {
        log::warn("debug overlay: Dear ImGui could not start{}", error.empty() ? "" : ": " + error);
        ImGui::DestroyContext();
        return false;
    }
    s.initialised = true;
    return true;
}

// The window moved to another display, or the display changed its mode or scale: the style, the
// font and the logo again at the new scale, and the font's texture with them.
void follow_scale(SDL_Window* window) {
    const float scale = window_ui_scale(window);
    if (std::abs(scale - s.ui_scale) < 0.01f) {
        return;
    }
    log::info("debug overlay: drawn at {:.2f}x (was {:.2f}x)", scale, s.ui_scale);
    s.ui_scale = scale;
    style_and_font();
    add_logo();
    if (s.gl_subset) {
        renderer::ui::imgui_gl::reload_font();
    } else {
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
    }
}

// ---- The panel ----

void panel(const Shown& shown) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(viewport->WorkSize.x - 20.0f, viewport->WorkSize.y - 20.0f));
    ImGui::SetNextWindowBgAlpha(0.82f);
    // On a touch screen it has no keys to name, and a gamepad moves through it while the game is
    // paused (draw); a finger scrolls it.
    const bool keys = !drawer_possible();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize
                                   | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
                                   | (keys ? ImGuiWindowFlags_NoNav : ImGuiWindowFlags_None);
    if (ImGui::Begin("OpenRAC debug", nullptr, flags)) {
        if (!keys) {
            drag_scroll(s.panel_scroll);
        }
        const Averages a = averages();
        const std::string where = openrac_game_level_name(s.level);
        text_coloured(kYellow, "OpenRAC {}  {}  tick {}  {:.0f} tps  {:.0f} fps", s.game, where, s.tick, a.tps, a.fps);
        if (ImGui::Button(keys ? (s.paused ? "resume (F5)" : "pause (F5)") : (s.paused ? "resume" : "pause"))) {
            s.paused = !s.paused;
            s.held_frames = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button(keys ? "step (F6)" : "step")) {
            s.paused = true;
            s.step = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(dp(70.0f));
        static const char* const kSpeeds[] = {"1x", "1/2", "1/4", "1/8", "1/16"};
        int speed = std::clamp(static_cast<int>(std::lround(std::log2(s.stretch))), 0, 4);
        if (ImGui::Combo("speed", &speed, kSpeeds, 5)) {
            s.stretch = static_cast<double>(1 << speed);
        }
        if (s.paused) {
            ImGui::SameLine();
            text_coloured(kRed, "PAUSED");
        }
        if (ImGui::Button(keys ? "screenshot (F12)" : "screenshot")) {
            s.shot_tick = s.tick;
        }
        ImGui::SameLine();
        bool dumping = stats::dumping();
        if (ImGui::Checkbox("dump frames", &dumping)) {
            if (dumping) {
                std::string error;
                s.message = stats::open_dump(s.dump_path, error) ? "frames are written to " + s.dump_path : error;
            } else {
                stats::close_dump();
                s.message = "frame dump closed";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(keys ? "record diagnostics (Ctrl+F3)" : "record diagnostics")) {
            toggle_capture(false);
        }
        if (!s.message.empty()) {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + dp(420.0f));
            text_coloured(kDim, "{}", s.message);
            ImGui::PopTextWrapPos();
        }
        const auto header = [](Section section, const char* label) {
            ImGui::SetNextItemOpen(s.open_at_start[section], ImGuiCond_Once);
            return ImGui::CollapsingHeader(label);
        };
        if (header(kFrame, "Frame")) {
            frame_section(shown, a);
        }
        if (header(kRenderer, "Renderer")) {
            renderer_section(shown);
        }
        if (header(kMemory, "Memory")) {
            memory_section();
        }
        if (header(kMobys, "Mobys")) {
            mobys_section();
        }
        if (header(kGame, "Game")) {
            game_section(shown);
        }
        if (header(kShaders, "Shaders")) {
            shaders_section();
        }
        if (keys) {
            text_coloured(kDim, "F3 hide  F5 pause  F6 step  F7/F8 slower/faster  F12 screenshot");
        }
    }
    ImGui::End();
}

// A screenshot's file: in the working folder; on Android, whose apps run in "/" (not writable),
// in the app's own files (HOME, which the launcher sets).
std::string shot_name(std::uint64_t tick, bool enhanced) {
    const std::string name =
        std::format("openrac_window_{}{}.png", tick, enhanced ? "_enhanced" : "");
#if defined(__ANDROID__)
    if (const std::optional<std::string> home = env::system_variable("HOME");
        home && !home->empty()) {
        std::error_code ignored;
        const std::filesystem::path dir = std::filesystem::path(*home) / "screenshots";
        std::filesystem::create_directories(dir, ignored);
        return (dir / name).string();
    }
#endif
    return name;
}

// The window as it is now, overlay included (the back buffer, before the swap), top row first.
std::vector<std::uint8_t> read_window(int width, int height) {
    using namespace openrac::gl;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    // OpenGL's first row is the bottom one.
    std::vector<std::uint8_t> flipped(pixels.size());
    const std::size_t stride = static_cast<std::size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        std::memcpy(flipped.data() + static_cast<std::size_t>(y) * stride,
                    pixels.data() + static_cast<std::size_t>(height - 1 - y) * stride, stride);
    }
    for (std::size_t i = 3; i < flipped.size(); i += 4) {
        flipped[i] = 0xFF;
    }
    return flipped;
}

// The window to a PNG. False when it could not be written.
bool write_window(const std::string& path, int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (scene::write_png(path, width, height, read_window(width, height))) {
        s.message = "window written to " + path;
        log::info("{}", s.message);
        return true;
    }
    s.message = "cannot write " + path;
    log::warn("{}", s.message);
    return false;
}

}  // namespace

void set_visible(bool on) {
    s.visible = on;
    stats::set_enabled(stats::Client::Overlay, on);
}

void request() {
    configure();
    s.wanted = true;
}

bool drawer_available() {
    return drawer_possible();
}

bool touch_editing() {
    return s.touch_editing;
}

bool take_touch_reset() {
    const bool reset = s.touch_reset;
    s.touch_reset = false;
    return reset;
}

void set_touch_guides(float x, float y) {
    s.touch_guides = ImVec2(x, y);
}

void set_touch_selected(float size) {
    s.touch_selected = size;
}

int take_touch_size_steps() {
    const int steps = s.touch_size_steps;
    s.touch_size_steps = 0;
    return steps;
}

void toggle_drawer() {
    if (drawer_possible()) {
        set_menu(!s.menu_open);
    }
}

bool visible() {
    return s.visible || s.enhanced_settings || s.meat_proxy_review || s.material_settings;
}

bool wants_cursor() {
    return s.menu_open || s.menu_at > 0.0f || visible() || s.object_viewer.open || picker_enabled()
           || s.touch_editing;
}

bool event(const SDL_Event& e) {
    configure();
    release_scripted_pointer(e);
    // A press awaited for a binding (the Controls tab): the controllers' presses are the
    // capture's, not the menu's. The keyboard does nothing either (no menu key, no Enter on
    // another row, no Android Back, which a controller's B may be), but Esc gives the capture up. Key releases still reach the menu, so a key
    // held when the capture began is let go there; the mouse stays free for the Cancel row.
    if (controls_capturing()) {
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || e.type == SDL_EVENT_GAMEPAD_BUTTON_UP
            || e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION || e.type == SDL_EVENT_TEXT_INPUT) {
            return true;
        }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_ESCAPE && !e.key.repeat) {
                controls_cancel_capture();
            }
            s.alt_alone = false;
            return true;
        }
    }
    // A click on a notification is the card's (a finger's tap: drawer_event).
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT
        && e.button.which != SDL_TOUCH_MOUSEID && s.initialised && !s.menu_open
        && press_notification(ImVec2(e.button.x, e.button.y))) {
        return true;
    }
    if (drawer_possible() && drawer_event(e)) {
        return true;
    }
    if (menu_window_possible()) {
        // The controls window waits for the menu's new controller button or key: the next one
        // pressed (Escape keeps the old one).
        if (s.menu_button_capture) {
            if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                s.menu_button_capture = false;
                settings::get().menu_bar_button =
                    SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(e.gbutton.button));
                settings::save();
                return true;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                s.menu_button_capture = false;
                return true;
            }
        }
        if (s.menu_key_capture && e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            s.menu_key_capture = false;
            if (e.key.key != SDLK_ESCAPE) {
                const bool alt = e.key.key == SDLK_LALT || e.key.key == SDLK_RALT;
                settings::get().menu_bar_key = alt ? "Alt" : SDL_GetScancodeName(e.key.scancode);
                settings::save();
            }
            return true;
        }
        // Alt+Tab: the system takes the Tab, and SDL reports the Alt still held when the window
        // has the focus again. An Alt that comes with the focus is not one by itself.
        if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED || e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            s.alt_alone = false;
            s.focus_ns = e.window.timestamp;
        }
        const SDL_Scancode key = menu_bar_scancode();
        // Freecam owns Alt for mouse look instead of opening the menu.
        if (s.camera_tools.enabled) {
            s.alt_alone = false;
        }
        if (key == SDL_SCANCODE_UNKNOWN && !s.camera_tools.enabled) {
            // Alt by itself opens and closes the menu (Alt+Return is fullscreen: another key in
            // between makes it not by itself).
            const bool alt = e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP
                                 ? e.key.key == SDLK_LALT || e.key.key == SDLK_RALT
                                 : false;
            constexpr Uint64 kFocusSettleNs = 300'000'000;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                s.alt_alone = false;  // Alt-drag navigation is not an Alt-only menu toggle.
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                s.alt_alone = alt && e.key.timestamp - s.focus_ns > kFocusSettleNs;
            } else if (e.type == SDL_EVENT_KEY_UP && alt && s.alt_alone) {
                s.alt_alone = false;
                toggle_menu();
            }
        } else if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat && e.key.scancode == key
                   && (e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) == 0) {
            toggle_menu();
            return true;
        }
    }
    // The menu's controller button (menu_bar_button), on a desktop and a touch screen alike, while
    // the game's window has the focus. Not "...": a running Steam client opens its own panel on it.
    // Start+Select, the default, opens the menu from the game's pad (menu_pad_chord) and closes
    // it here: the second of the pair pressed while the first is held, on one controller.
    MenuMotion& m = s.menu_motion;
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && s.menu_open && menu_bar_chord()
        && (e.gbutton.button == SDL_GAMEPAD_BUTTON_START
            || e.gbutton.button == SDL_GAMEPAD_BUTTON_BACK)
        && SDL_GetKeyboardFocus() != nullptr && !s.pad_locked && !controls_capturing()) {
        SDL_Gamepad* pad = SDL_GetGamepadFromID(e.gbutton.which);
        const SDL_GamepadButton other = e.gbutton.button == SDL_GAMEPAD_BUTTON_START
                                            ? SDL_GAMEPAD_BUTTON_BACK
                                            : SDL_GAMEPAD_BUTTON_START;
        if (pad != nullptr && SDL_GetGamepadButton(pad, other)) {
            m.pad = true;
            toggle_menu();
            return true;
        }
    }
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && SDL_GetKeyboardFocus() != nullptr
        && menu_bar_button() != SDL_GAMEPAD_BUTTON_INVALID && e.gbutton.button == menu_bar_button()
        && !s.pad_locked && !controls_capturing()) {
        m.pad = true;
        toggle_menu();
        return true;
    }
    // Whose buttons the menu names at its foot: the controller's, or the keyboard's. Watched
    // while it is shut too, so a menu opened from a controller names the controller's.
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN
        || (e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && std::abs(e.gaxis.value) > 16000)) {
        m.pad = true;
    } else if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        m.pad = false;
    }
    // Escape closes the desktop's menu, as B does (menu_window).
    if (menu_window_possible() && s.menu_open && e.type == SDL_EVENT_KEY_DOWN
        && e.key.key == SDLK_ESCAPE) {
        set_menu(false);
        return true;
    }
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        if (e.key.key == SDLK_F3 && (e.key.mod & SDL_KMOD_CTRL) != 0) {
            toggle_capture(false);
            return true;
        }
        if (e.key.key == SDLK_F3) {
            set_visible(!s.visible);
            s.wanted = false;
            return true;
        }
        // Pause, step, speed and the screenshot are their own: they work with the panel hidden,
        // and say so in the picture (status_badge).
        switch (e.key.key) {
            case SDLK_F5:
                s.paused = !s.paused;
                s.held_frames = 0;
                return true;
            case SDLK_F6:
                s.paused = true;
                s.step = true;
                return true;
            case SDLK_F7:
                s.stretch = std::min(s.stretch * 2.0, 16.0);
                return true;
            case SDLK_F8:
                s.stretch = std::max(s.stretch / 2.0, 1.0);
                return true;
            case SDLK_F12:
                s.shot_tick = s.tick;
                return true;
            // The menu, open at its Controls tab (debug/menu_controls.cpp).
            case SDLK_F13:
                if (menu_window_possible()) {
                    s.menu_want_tab = static_cast<int>(menu::Tab::Controls);
                    set_menu(true);
                }
                return true;
            default:
                break;
        }
    }
    if (!(visible() || s.object_viewer.open || picker_enabled() || s.menu_open) || !s.initialised) {
        return false;
    }
    ImGui_ImplSDL3_ProcessEvent(&e);
    const ImGuiIO& io = ImGui::GetIO();
    // While the desktop's menu is open the keyboard and the controller are its own (the game waits
    // under it); the port's function keys still reach the window, as the menu presses some.
    if (menu_window_possible() && s.menu_open) {
        if (e.type == SDL_EVENT_KEY_DOWN && !(e.key.key >= SDLK_F1 && e.key.key <= SDLK_F24)) {
            return true;
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            return true;
        }
    }
    switch (e.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
        case SDL_EVENT_MOUSE_MOTION:
            return io.WantCaptureMouse
                   || (picker_enabled()
                       && (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                           || e.type == SDL_EVENT_MOUSE_BUTTON_UP)
                       && e.button.button == SDL_BUTTON_LEFT);
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT:
            return io.WantTextInput
                   || ((s.object_viewer.open || s.meat_proxy_review) && io.WantCaptureKeyboard);
        default:
            return false;
    }
}

void tick(const Tick& t) {
    configure();
    if (s.wanted && !s.visible) {
        s.wanted = false;
        set_visible(true);
    }
    if (s.menu_open_at >= 0 && t.tick >= static_cast<std::uint64_t>(s.menu_open_at)) {
        s.menu_open_at = -1;
        set_menu(true);
    }
    if (s.pause_at >= 0 && t.tick == static_cast<std::uint64_t>(s.pause_at)) {
        s.paused = true;
        s.pause_at = -1;
        log::info("debug overlay: paused at tick {}", t.tick);
    }
    if (s.reload_at >= 0 && t.tick == static_cast<std::uint64_t>(s.reload_at)) {
        s.reload_at = -1;
        std::string message;
        const int done = renderer::gl_stats::reload_all(shader_root(), message);
        s.message = std::format("{} programs reloaded{}", done, message.empty() ? "" : "\n" + message);
        log::info("debug overlay: {}", s.message);
    }
    stats::gauge("mobys", t.state != nullptr ? static_cast<double>(t.state->mobys.size()) : 0.0);
    stats::gauge("mobys_drawn", static_cast<double>(t.mobys_drawn));
    stats::gauge("ui_quads", static_cast<double>(t.ui_quads));
    stats::gauge("effect_quads", static_cast<double>(t.effect_quads));
    stats::gauge("packet_bytes", static_cast<double>(t.packet_bytes));
    stats::gauge("level", static_cast<double>(t.level));
    s.tick = t.tick;  // a screenshot is named after it, with the panel shown or not
    s.game = t.game;  // and a performance capture's report says where it was
    s.level = t.level;
    if (!s.visible) {
        return;
    }
    s.game = t.game;
    s.ram = t.ram;
    s.level = t.level;
    if (t.state != nullptr) {
        s.state = *t.state;
    }
    s.draws = t.draws;
    s.tick = t.tick;
    s.counts = t;
    if (s.follow_hero) {
        if (const host::LiveMoby* h = hero()) {
            s.selected = h->address;
        }
    }
}

void begin(bool first, bool held, float alpha, std::uint64_t tick_number) {
    if ((!s.script.empty() || !s.script_file.empty()) && (first || held)) {
        s.held_pictures = held ? s.held_pictures + 1 : 0;
        run_script(tick_number, s.held_pictures);
    }
    if (first) {
        // The tick's frame was opened where the tick ended (main.cpp); what came since read the
        // scene from the game's memory.
        stats::set_frame(tick_number, alpha);
        stats::lap("scene");
    } else {
        stats::begin_frame(held ? stats::FrameKind::Held : stats::FrameKind::Between, tick_number, alpha);
    }
}

void before_render(renderer::Renderer* renderer, std::uint64_t frame) {
    using namespace openrac::gl;
    // OPENRAC_HIDE_PASSES_EVERY=N: the passes are hidden in every other N frames only, so a place
    // the hero is held at is shot with them and without them in one run (port/tools/regress/
    // survey.py). `frame` is the tick's window frame, the trace's and OPENRAC_SHOT's number.
    static const auto every = static_cast<std::uint64_t>(
        std::max<std::int64_t>(0, env::integer(env::Var::HidePassesEvery, 0))
    );
    if (renderer != nullptr && !s.hidden.empty()) {
        const bool hide = every == 0 || (frame / every) % 2 == 1;
        for (const auto& r : renderer->renderers()) {
            if (s.hidden.contains(r->name())) {
                r->enabled = !hide;
            }
        }
    }
    if (s.wireframe && s.visible) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        s.wireframe_set = true;
    }
    stats::lap("prepare");
}

void draw(const Shown& shown) {
    using namespace openrac::gl;
    if (s.wireframe_set) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        s.wireframe_set = false;
    }
    stats::lap("present");
    s.window = shown.window;
    const bool badge = s.paused || s.stretch != 1.0 || notifications_pending() || capture_busy()
                       || !mods::play_notice().empty();
    const bool menu_out = s.menu_open || s.menu_at > 0.0f || s.shot_when_shut || s.touch_editing;
    if (!(visible() || picker_enabled() || badge || menu_out || s.shot_tick)
        || shown.window == nullptr || !initialise(shown.window)) {
        s.drew_last_frame = false;
        return;
    }
    const stats::Scope scope("overlay");
    capture_frame(shown);
    // The processor's load, each half second.
    if (const stats::Frame* f = stats::history(0); s.visible && f != nullptr && f->start_s - s.cpu_wall >= 0.5) {
        const double seconds = stats::process_cpu_seconds();
        s.cpu_load = s.cpu_wall > 0.0 ? (seconds - s.cpu_seconds) / (f->start_s - s.cpu_wall) : 0.0;
        s.cpu_seconds = seconds;
        s.cpu_wall = f->start_s;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, shown.window_width, shown.window_height);
    {
        // A gamepad and the keyboard move through the menu's rows while it is open (and on a
        // touch screen through the panel while the game stands still under it); they are the
        // game's otherwise. The d-pad comes as the arrow keys (feed_gamepads), so both flags.
        ImGuiIO& io = ImGui::GetIO();
        const bool moves = s.menu_open || (drawer_possible() && s.visible && s.paused);
        constexpr ImGuiConfigFlags kNav =
            ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags = moves ? io.ConfigFlags | kNav : io.ConfigFlags & ~kNav;
        // The pointer hidden over the game (frontend's apply_mouse_capture): ImGui's backend
        // must not show it again while it draws only notifications.
        io.ConfigFlags = wants_cursor() ? io.ConfigFlags & ~ImGuiConfigFlags_NoMouseCursorChange
                                        : io.ConfigFlags | ImGuiConfigFlags_NoMouseCursorChange;
    }
    follow_scale(shown.window);
    if (!s.gl_subset) {
        ImGui_ImplOpenGL3_NewFrame();
    }
    ImGui_ImplSDL3_NewFrame();
    // After frames it skipped, ImGui's clock holds the whole gap: a menu opening then would end
    // in one frame. It starts again from one frame's time.
    if (!s.drew_last_frame) {
        ImGui::GetIO().DeltaTime = std::min(ImGui::GetIO().DeltaTime, 1.0f / 60.0f);
    }
    s.drew_last_frame = true;
    feed_gamepads();
    refresh_scripted_pointer();
    ImGui::NewFrame();
    enhanced_settings_window();
    meat_proxy_review_window(shown);
    material_settings_window(shown);
    controls_frame();
    if (s.visible) {
        panel(shown);
        inspector_window();
        memory_viewer_window();
        picture_marks(shown);
    }
    if (drawer_possible()) {
        drawer(shown);
    } else {
        menu_window(shown);
    }
    menu_sounds_frame();
    if (s.touch_editing) {
        touch_edit_bar();
    }
    picker_overlay(shown);
    object_viewer_window(shown);
    status_notifications();

    std::string scripted_shot;
    const stats::Frame* last = stats::history(0);
    const bool tick_frame = last == nullptr || last->kind != stats::FrameKind::Between;
    if (s.shot_every > 0 && tick_frame && s.tick % s.shot_every == 0 && s.tick >= s.shot_from && s.shots < 10000) {
        // Once a tick; and once more while paused, a few pictures in ("_held").
        static std::uint64_t written = ~0ull;
        static int held = 0;
        held = s.paused ? held + 1 : 0;
        if (written != s.tick || held == 5) {
            written = s.tick;
            scripted_shot = std::format(
                "{}{}{}{}.png",
                s.shot_prefix,
                s.tick,
                held == 5 ? "_held" : "",
                shown.enhanced_graphics ? "_enhanced" : ""
            );
            s.shots++;
        }
    }
    if (!s.shot_tick && scripted_shot.empty()) {
        draw_notifications();
    }
    ImGui::Render();
#if defined(OPENRAC_VULKAN)
    // The panel needs no depth. Drawn in a pass with the default depth image, Adreno broke it at
    // 2x MSAA.
    if (gl::is_vulkan()) {
        vkgl::default_framebuffer_depth(false);
    }
#endif
    if (s.gl_subset) {
        renderer::ui::imgui_gl::render(ImGui::GetDrawData());
    } else {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
#if defined(OPENRAC_VULKAN)
    if (gl::is_vulkan()) {
        vkgl::default_framebuffer_depth(true);
    }
#endif
    renderer::gl_stats::reset_binds();

    if (!scripted_shot.empty()) {
        s.message = "window written to " + scripted_shot;
        scene::write_png_later(
            scripted_shot,
            shown.window_width,
            shown.window_height,
            read_window(shown.window_width, shown.window_height),
            s.message
        );
    }
    if (s.shot_tick) {
        const std::string path = shot_name(*s.shot_tick, shown.enhanced_graphics);
        const bool saved = write_window(path, shown.window_width, shown.window_height);
        notify({
            .title = saved ? "Screenshot saved" : "The screenshot could not be saved",
            .text = path,
            .folder = saved ? path : std::string(),
            .tone = saved ? Tone::Done : Tone::Error,
            .seconds = saved ? 3.0 : 8.0,
        });
        s.shot_tick.reset();
    }
}

void pad(std::uint16_t buttons, const std::uint8_t analog[4]) {
    s.buttons = buttons;
    std::memcpy(s.analog, analog, 4);
}

bool holding() {
    if (!s.paused) {
        return false;
    }
    if (s.step) {
        s.step = false;
        return false;
    }
    if (s.held_frames > 0 && --s.held_frames == 0) {
        s.paused = false;
        log::info("debug overlay: resumed");
        return false;
    }
    return true;
}

double tick_stretch() {
    configure();
    return s.stretch;
}

bool take_settings_changed() {
    return std::exchange(s.settings_changed, false);
}

void release() {
    release_object_viewer();
    renderer::ui::backdrop_blur::release();
    release_material_preview();
    if (s.initialised) {
        if (s.gl_subset) {
            renderer::ui::imgui_gl::shutdown();
        } else {
            ImGui_ImplOpenGL3_Shutdown();
        }
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        s.initialised = false;
    }
}

}  // namespace openrac::frontend::debug_overlay
