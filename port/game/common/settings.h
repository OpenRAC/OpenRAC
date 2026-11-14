// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The player's settings for a native game: what the port adds to the game's own options (the
// window and the display it is on, the picture's shape and resolution, anti-aliasing and texture
// filtering, how many frames are drawn and whether they wait for the display, how loud). They are
// kept in a small text file, `key = value` a line, beside the memory cards (`--settings FILE`
// names another), read once at start and written whenever one changes.
// docs/port/SETTINGS.md lists every key, and what OpenGOAL and ReRAC offer beside them.
#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "common/hdr.h"
#include "renderer/settings/bloom_settings.h"
#include "renderer/settings/gtao_settings.h"
#include "renderer/settings/motion_blur_settings.h"
#include "renderer/settings/render_opt.h"
#include "renderer/settings/shadow_settings.h"
#include "renderer/settings/temporal_settings.h"
#include "renderer/settings/volumetric_settings.h"
#include "renderer/textures/texture_filter.h"
#include "renderer/ui/fps_counter.h"

namespace openrac::settings {

// The F2 cycle: hidden, the existing frame counter, or separate CPU/GPU measurements.
using FrameCounter = renderer::ui::fps_counter::Mode;

// The window. Borderless covers the desktop at the desktop's mode (no exclusive full screen; an
// old file's `fullscreen` reads as borderless).
enum class DisplayMode {
    Windowed,
    Borderless,
    Count,
};

// The picture's shape. The game draws for a 4:3 television; a wider picture shows more of the
// world to the sides (the vertical field of view is the game's), with the 2D layer where it was.
enum class Aspect {
    Standard,   // 4:3, the game's own
    Wide16x10,  // 16:10
    Wide16x9,   // 16:9
    Wide21x9,   // 21:9
    Wide32x9,   // 32:9
    Window,     // whatever the window's shape is (never narrower than 4:3)
    Custom,     // Settings::aspect_custom_x : aspect_custom_y (the file's `aspect_custom`)
    Count,
};

// Where the game's HUD goes in a picture wider than the game's.
enum class Hud {
    Centre,  // where the game put it: inside the 4:3 picture in the middle
    Edges,   // what the game anchors to the screen's left or right goes out to the picture's sides
    Count,
};

// Whether a frame waits for the display's refresh.
enum class VSync {
    Off,
    On,
    Adaptive,  // waits, but a late frame is shown at once (it tears) rather than a refresh later
    Count,
};

// How many frames are drawn. The game's tick stays its own (50 a second for PAL): the frames
// between two ticks move the camera, the mobys and their poses from one tick to the next.
enum class FrameRate {
    Original,   // one frame a tick
    Display,    // the display's refresh rate
    Capped,     // Settings::frame_cap frames a second
    Unlimited,  // as many as the machine draws
    Count,
};

// Gameplay field of view bounds and native value, in percent.
inline constexpr int kMinGameplayFov = 80;
inline constexpr int kDefaultGameplayFov = 100;
inline constexpr int kMaxGameplayFov = 120;
inline constexpr int kGameplayFovStep = 5;

// The presets of Enhanced graphics: the options that cost the most, set together. Custom is any
// other mix of them.
enum class EnhancedPreset {
    Low,     // weak phones and integrated graphics: every effect at its cheapest, motion blur off
    Medium,  // handhelds and Apple Silicon: fewer rendered pixels and smaller sun maps
    High,    // desktop graphics cards
    Custom
};
inline constexpr const char* kEnhancedPresetNames[] = {"Low", "Medium", "High", "Custom"};

// Where Enhanced graphics start at the Medium preset: phones, handhelds and Apple Silicon. The
// same effects at a lower cost; players can still pick High.
#if defined(__ANDROID__) || (defined(__APPLE__) && defined(__aarch64__))
inline constexpr bool kHandheldDefaults = true;
#else
inline constexpr bool kHandheldDefaults = false;
#endif

struct Settings {
    DisplayMode display_mode = DisplayMode::Windowed;
    int monitor = 0;  // 0: the display the window is on; n: the nth display, counted from 1
    int window_width = 0;  // the window's size when last windowed; 0 x 0: the port's default
    int window_height = 0;
    Aspect aspect = Aspect::Standard;
    int aspect_custom_x = 16;
    int aspect_custom_y = 9;
    Hud hud = Hud::Centre;
    int gameplay_fov =
        kDefaultGameplayFov;  // Vertical field of view as a percentage of the gameplay camera.
    int render_scale = 100;  // the frame's resolution, as a percentage of the window's picture
    int msaa = 1;            // samples a pixel: 1 (off), 2, 4, 8, 16
    renderer::GtaoSettings gtao_settings;
    // Enhanced rendering at all (port/enhanced; the launcher's Mods entry): read at start, Vulkan
    // only. Off, nothing of it is made and the game draws as a build without it.
    bool enhanced_rendering = false;
    bool enhanced_graphics = true;  // F4 within it; retains individual feature preferences
    bool enhanced_shadows = true;   // cascades and Ratchet map; off restores original shadows
    renderer::ShadowQuality shadow_quality =
        kHandheldDefaults ? renderer::ShadowQuality::Medium : renderer::ShadowQuality::High;
    renderer::ShadowFilter shadow_filter = renderer::ShadowFilter::Soft;
    renderer::VolumetricQuality volumetric_quality = renderer::VolumetricQuality::Off;

    struct NanotechLight {
        bool enabled = true;
        std::array<float, 3> colour{0.15f, 0.55f, 1.0f};
        float brightness = 1.20f;
        float range = 2.22f;
    } nanotech_light;

    // The menu puts Off before the renderer's quality levels.
    int shadow_quality_choice() const {
        return enhanced_shadows ? static_cast<int>(shadow_quality) + 1 : 0;
    }

    void set_shadow_quality_choice(int choice) {
        enhanced_shadows = choice > 0;
        if (enhanced_shadows) {
            shadow_quality = static_cast<renderer::ShadowQuality>(choice - 1);
        }
    }
    // A texture pack from <data>/texture_packs/<game id>/ (docs/port/TEXTURE_PACKS.md); takes
    // effect at the next start.
    bool texture_pack = true;
    bool probe_spheres = false;        // diagnostic placement/fade overlay
    bool medial_axis_debug = false;    // Baked centre paths and junctions.
    bool ceiling_area_lights = false;  // Baked source rectangles and emitting-direction arrows.
    bool environment_probe_debug =
        false;  // Baked reflection capture positions and influence boxes.
    float probe_gi_multiplier = 1.0f;
    // The settings sweep's experiments alone (debug/capture.cpp); never saved.
    bool sweep_rim_light_off = false;
    bool sweep_probe_lighting_off = false;
    bool sweep_translucent_off = false;
    bool sweep_translucent_unlit = false;
    bool sweep_translucent_full = false;
    bool sweep_moby_blended_off = false;
    bool sweep_one_triangle = false;
    float probe_normal_bias = 0.5f;  // receiver offsets in world units
    float probe_view_bias = 0.5f;
    bool sun_direction_override = false;
    float sun_azimuth = 0, sun_elevation = 45;  // Light-source angles, degrees; Z is up.
    std::optional<std::array<float, 3>> sunlight_direction() const;
    // Restore this window's preferences without changing general graphics/audio or startup enablement.
    void reset_enhanced();
    // Sets the options a preset covers (upscaling, shadow quality and filtering, occlusion
    // quality, motion blur, texture upscaling, effects detail) and leaves the rest alone; Custom
    // changes nothing.
    void apply_enhanced_preset(EnhancedPreset preset);
    // The preset those options match now, or Custom.
    EnhancedPreset enhanced_preset() const;
    bool gtao = true;  // ground-truth ambient occlusion when enhanced rendering is enabled

    int occlusion_quality_choice() const {
        return gtao ? static_cast<int>(gtao_settings.quality) + 1 : 0;
    }

    void set_occlusion_quality_choice(int choice) {
        gtao = choice > 0;
        if (gtao) {
            gtao_settings.quality = static_cast<renderer::GtaoQuality>(choice - 1);
        }
    }
    renderer::TemporalQuality temporal_quality =  // Enhanced Vulkan only.
        kHandheldDefaults ? renderer::TemporalQuality::Balanced
                          : renderer::TemporalQuality::Quality;
    renderer::BloomSettings bloom;
    bool agx_preview = true;                 // SDR-only AgX colour treatment experiment.
    float agx_exposure = 0.25f;              // Base exposure in stops, before AgX.
    float agx_brightness = 0.5f;             // 0..1 adds -0.8..+0.8 EV; midpoint is neutral.

    float adjusted_agx_exposure() const { return agx_exposure + (agx_brightness - .5f) * 1.6f; }

    bool motion_blur = true;  // Motion relative to the tracking focus.
    renderer::MotionBlurSettings motion_blur_settings;
    float lily_momentum = .25f;  // Tilt response time in seconds; zero follows waves immediately.
    float motion_blur_strength = 0.5f;       // Exposure strength in [0, 1].
    float temporal_sharpness = 0.2f;         // AMD RCAS strength in [0, 1].
    renderer::TextureFilter texture_filter = renderer::TextureFilter::EdgeAware;
    // Effects detail: High draws every Enhanced effect in full; Medium and Low allow the lossy
    // shortcuts that cost visibly little (renderer/render_opt.h).
    renderer::render_opt::EffectsDetail effects_detail =
        kHandheldDefaults ? renderer::render_opt::EffectsDetail::Medium
                          : renderer::render_opt::EffectsDetail::High;
    int anisotropy = 0;  // 0: mode default; otherwise 1 (off), 2, 4, 8, 16
    // Draw distance: how far the game's own culls reach, in percent of the game's (100, Original:
    // each moby's, tie's and shrub's own distance, at most the level's caps; 400 the default); 0
    // Unlimited, no distance limit. What the game hides by its visibility mask or a bit of its
    // records stays hidden at any setting. Applied to the mobys, ties and shrubs of every game and
    // to Going Commando's terrain (rac1's ties and shrubs: scene::LevelScene::choose_scenery).
    int draw_distance = 400;

    // The draw distance's factor: 1 at Original, 0 Unlimited.
    float draw_distance_scale() const { return static_cast<float>(draw_distance) / 100.0f; }

    // Original: the game's own distances, with its fades and billboards.
    bool original_draw_distance() const { return draw_distance == 100; }

    // Active distance: how far the game updates its mobys (animation, AI) when its moby loop goes
    // by what MobyProc drew (the +0x31 flag). Match (the default) takes Draw distance's reach, so
    // whatever is drawn far away is live; Original keeps the game's own distances, so a moby past
    // them is drawn standing in its pose, as before. The AI's own ranges (sight, aggro, attack)
    // are the game's at either setting.
    bool original_active_distance = false;

    // The active distance's factor: Draw distance's at Match, 1 at Original.
    float active_distance_scale() const {
        return original_active_distance ? 1.0f : draw_distance_scale();
    }

    int anisotropy_samples(bool enhanced) const {
        return anisotropy ? anisotropy : enhanced ? 4 : 1;
    }

    // HDR output (common/hdr.h, docs/port/HDR.md), off by default: the picture goes to the screen
    // as it always did. On (with the Vulkan renderer), the same picture is shown on an HDR
    // screen with the game's white at paper white. From off, it takes effect at the next start.
    hdr::Mode hdr = hdr::Mode::Off;
    int hdr_paper_white = 0;  // nits of the game's white; 0: the system's SDR white, else 203
    int hdr_peak = 0;         // nits the screen is told it may show; 0: the display's, else 1000
    // How light above white looks, in percent (hdr::Look). The defaults are the plain HDR look.
    int hdr_highlights = 100;  // light above white: 0 as in SDR
    int hdr_sdr_hue = 0;       // take the SDR hue
    int hdr_sdr_chroma = 0;    // take the SDR saturation
    int hdr_blowout = 0;       // very bright light fades to white

    // The menu's size in percent of the one fitted to the screen (debug/ui_scale.h): 100 is Auto.
    int menu_size = 100;

    VSync vsync = VSync::On;
    FrameRate frame_rate = FrameRate::Display;
    int frame_cap = 60;                         // FrameRate::Capped's frames a second
    FrameCounter show_fps = FrameCounter::Off;  // Counter mode in the picture's corner.
    bool gpu_breadcrumbs = true;
    // The key that opens and closes the menu, by its place on the keyboard: an SDL scancode name
    // ("`", the key under Escape; "F4", "M"), or "Alt" (either Alt pressed and let go by itself).
    std::string menu_bar_key = "`";
    // The controller button that does the same (settings.ini's menu_button): "start+back", Start and
    // Select pressed together, which the game never sees (platform/menu_chord.h); an SDL gamepad
    // button name ("paddle1", the Steam Controller's R4; "misc1"); or "none".
    std::string menu_bar_button = "start+back";
    bool mute_unfocused = false;  // silence while another window has the keyboard
    bool skip_cutscenes = true;   // Start skips an in-game cutscene (without the game's L1 L2 R1 R2 hold)
    // Lower input latency, off by default (the timing players know). Reads the pad again just
    // before each tick (frontend.h refresh_pads), and turns the camera in the frames between ticks
    // with the right stick as it is now (frontend/camera_latch.h, the picture only). The game's
    // ticks and logic are unchanged.
    bool responsive_controls = false;
    // Low-latency frames, off by default (only when frames are shown, never the game's ticks):
    // frames between ticks are not held half a tick back, and fewer finished frames queue for the
    // display. On Vulkan a frame between ticks starts (reads the time and input) as late as it can
    // while still being queued the moment the GPU is free, so the frame rate stays the same.
    bool low_latency_frames = false;
    int volume = 100;  // of everything the port plays, 0 (nothing at all) to 100 (as mixed)
    // The on-screen pad (platform/touch_pad.h): its controls' size and the camera's speed, in
    // percent of the pad's own, and where the player moved its groups (TouchPad::layout()).
    int touch_scale = 75;
    // Its buttons: 0 the game's actions as icons, changing with what the game is doing (the Arc
    // style, game/common/frontend/touch_controls.h), 1 the DualShock's (Classic).
    int touch_style = 0;
    int touch_camera = 100;
    std::string touch_layout;
    bool verbose_log =
        false;  // the detailed log (as OPENRAC_LOG=verbose): level loads, pad changes, callers
    // Options the game keeps in its own memory, which a console forgets at power off (keys
    // game_*; game_host.h, openrac_game_option): the port remembers them between runs.
    std::map<std::string, int, std::less<>> game_options;
    // What the player bound each pad button to (keys bind_*, values as platform/bindings.h names
    // them: "Space,PadSouth"). Kept as the file wrote them, so this file needs nothing of SDL;
    // platform::set_bindings reads the names. A button with no binding keeps the port's default.
    std::map<std::string, std::string, std::less<>> bindings;
};

// The settings in use.
Settings& get();

// Reads `file` (missing or unreadable: the defaults) and remembers it for save().
void load(const std::filesystem::path& file);

// A first start, the file load() read having no display_mode (nor the older fullscreen key): the
// game covers the screen (borderless) instead of opening a window. A file with either key keeps
// what it says, so a player's own setup never changes. `scripted` (a hidden window, an offscreen
// video driver: checks and tools) keeps the window. Returns whether the mode changed.
bool first_start_display(bool scripted);

// The settings in one line, for the log and its header.
std::string summary();

// Writes the settings to the file load() was given; nothing without one.
void save();

// Sets the volume (held to 0..100). output_gain() is it as a factor, for the sound's own thread;
// set_muted(true) holds the factor at 0 whatever the volume is (the window lost the keyboard).
void set_volume(int volume);

// HDR output's mode by its file name ("off", "on"), as OPENRAC_HDR asks for it over the file's:
// false, with nothing changed, for another word.
bool set_hdr_mode(std::string_view name);
void set_muted(bool muted);
float output_gain();

// The start of every key of Settings::game_options in the file.
inline constexpr std::string_view kGameOptionPrefix = "game_";

// The prefix of a key that binds a pad button (platform/bindings.h names the values).
inline constexpr std::string_view kBindPrefix = "bind_";

// Returns the game's option `key` (`game_` and a name, one of Settings::game_options), or
// `fallback` while none is remembered. openrac_game_option's body (program/game_host.cpp).
int game_option(std::string_view key, int fallback);

// Remembers `value` for the game's option `key` (one starting with kGameOptionPrefix) and writes
// the file when the value changed. openrac_game_set_option's body (program/game_host.cpp).
void set_game_option(std::string_view key, int value);

// Width over height of the picture for a window of that size.
float aspect_ratio(const Settings& settings, int window_width, int window_height);

// The frame caps the options page offers, in frames a second, and the ones a file may hold.
inline constexpr int kFrameCaps[] = {30, 60, 75, 90, 120, 144, 165, 240, 360};
inline constexpr int kFrameCapMin = 20;
inline constexpr int kFrameCapMax = 1000;

// The menu's sizes the menu offers, in percent of Auto (100), and the range a file may hold.
inline constexpr int kMenuSizes[] = {60, 70, 80, 90, 100, 110, 125, 150, 175, 200};
inline constexpr int kMenuSizeMin = 50;
inline constexpr int kMenuSizeMax = 250;

// The render scales, sample counts and anisotropies the options page offers.
inline constexpr int kRenderScales[] = {50, 75, 100, 125, 150, 200};
inline constexpr int kSampleCounts[] = {1, 2, 4, 8, 16};
inline constexpr int kAnisotropies[] = {1, 2, 4, 8, 16};
// Draw distance's steps, in percent of the game's; 0 is Unlimited, the last.
inline constexpr int kDrawDistances[] = {100, 150, 200, 400, 0};  // 400 the default

// What the drawer offers for the on-screen pad's size and camera speed, in percent; a file may
// hold any whole number between the first and the last.
inline constexpr int kTouchScales[] = {50, 60, 70, 75, 80, 90, 100, 110, 125, 150};
inline constexpr int kTouchCameraSpeeds[] = {25, 50, 75, 100, 125, 150, 200, 250, 300};

// The value `by` (-1 or 1) steps to from `now` in a list held in order: a value between two of the
// list's (a file's) steps to the nearer one that way, and the list's ends go no further.
int step_listed(std::span<const int> list, int now, int by);

// Advances the frame counter by `by` modes, wrapping in either direction.
void step_frame_counter(int by);

// What the options page shows for a value.
const char* name(FrameCounter mode);
const char* name(DisplayMode mode);
const char* name(Aspect aspect);
const char* name(Hud hud);
const char* name(VSync vsync);
const char* name(renderer::TextureFilter filter);
std::string frame_rate_name(const Settings& settings);

}  // namespace openrac::settings
