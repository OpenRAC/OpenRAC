// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What each of the menu's entries (menu_catalog.h) shows and does, and the entries drawn as rows
// (debug/menu_ui.h). Both menus draw them through here, so an entry behaves the same in each.
//
// An entry's `case` is required in both switches below: a missing one does not compile.

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <imgui.h>
#include <iterator>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include "common/log.h"
#include "common/slow_cpu.h"
#include "debug/menu_ui.h"
#include "enhanced/fsr3/fsr3.h"
#include "fixes.h"
#include "frontend.h"
#include "mods.h"
#include "options_menu.h"
#include "renderer/gl.h"
#include "settings.h"

#pragma clang diagnostic error "-Wswitch"

namespace openrac::frontend::debug_overlay {

namespace {

using menu::Id;

// What an entry shows now.
struct Item {
    bool shown = true;
    std::string label;      // empty: the catalog's
    bool on = false;        // a switch
    std::string value;      // a choice's value, a note's line
    bool can_lower = true;  // a choice: whether it can be moved back and on
    bool can_raise = true;
};

constexpr bool kEnhancedBuilt =
#if defined(OPENRAC_ENHANCED_RENDERING) && OPENRAC_ENHANCED_RENDERING
    true;
#else
    false;
#endif

// What a setting changed outside change_setting needs: saved, and marked for the window to apply.
void changed() {
    settings::save();
    s.settings_changed = true;
}

// Whether the enhanced graphics' own rows can be used now: built, started with Vulkan, the level's
// lighting prepared.
bool enhanced_ready() {
    return kEnhancedBuilt && gl::enhanced_rendering_enabled() && gl::enhanced_rendering_available()
           && enhanced_graphics_available();
}

// Its rows past the main switch: with it on.
bool enhanced_on() {
    return enhanced_ready() && settings::get().enhanced_graphics;
}

// A value in steps of `step` within [lo, hi], as a percentage.
std::string percent(float v) {
    return std::format("{:.0f}%", v * 100.0f);
}

float stepped(float v, float step, int by, float lo, float hi) {
    return std::clamp(std::round((v + step * static_cast<float>(by)) / step) * step, lo, hi);
}

// The freecam's fly speeds, in the game's units a second (its slider's range, 1 to 200).
constexpr std::array<float, 13> kFlySpeeds = {1, 2, 5, 10, 15, 20, 30, 40, 50, 75, 100, 150, 200};

int fly_speed_index(float speed) {
    int best = 0;
    for (int i = 1; i < static_cast<int>(kFlySpeeds.size()); ++i) {
        if (std::abs(kFlySpeeds[i] - speed) < std::abs(kFlySpeeds[best] - speed)) {
            best = i;
        }
    }
    return best;
}

constexpr std::array<const char*, 5> kInspectNames =
    {"Main camera", "Shadow cascade 1", "Shadow cascade 2", "Shadow cascade 3", "Shadow cascade 4"};

// HDR's paper whites: 0 is the system's SDR white.
constexpr std::array<int, 13> kWhites =
    {0, 80, 100, 120, 160, 203, 250, 300, 400, 500, 600, 800, hdr::kPaperWhiteMax};
// HDR's peaks: 0 is the display's. Up to PQ's 10000 nits, what the brightest screens are sold at.
constexpr std::array<int, 13> kPeaks =
    {0, 400, 600, 800, 1000, 1400, 2000, 2500, 3000, 4000, 5000, 6000, hdr::kPeakMax};

// The step of `steps` at `value`, or the first above it (a value the file set between steps).
template <std::size_t N>
int step_index(const std::array<int, N>& steps, int value) {
    int at = 0;
    while (at < static_cast<int>(N) - 1 && steps[at] < value) {
        ++at;
    }
    return at;
}

// A slider's setting and its range in percent.
struct Slider {
    int* value;
    int minimum;
    int maximum;
    int step;
};

// The HDR look's slider step. The settings file takes any percent.
constexpr int kHdrLookStep = 5;

Slider slider_of(Id id) {
    settings::Settings& set = settings::get();
    switch (id) {
        case Id::HdrHighlights:
            return {&set.hdr_highlights, 0, 100, kHdrLookStep};
        case Id::HdrSdrHue:
            return {&set.hdr_sdr_hue, 0, 100, kHdrLookStep};
        case Id::HdrSdrChroma:
            return {&set.hdr_sdr_chroma, 0, 100, kHdrLookStep};
        case Id::HdrBlowout:
            return {&set.hdr_blowout, 0, 100, kHdrLookStep};
        case Id::GameplayFov:
        default:
            return {
                &set.gameplay_fov,
                settings::kMinGameplayFov,
                settings::kMaxGameplayFov,
                settings::kGameplayFovStep
            };
    }
}

int white_index() {
    return step_index(kWhites, settings::get().hdr_paper_white);
}

int peak_index() {
    return step_index(kPeaks, settings::get().hdr_peak);
}

constexpr auto kSizes = std::to_array(settings::kMenuSizes);

int size_index() {
    return step_index(kSizes, settings::get().menu_size);
}

// An options row (options_menu.h) as a choice.
Item row_choice(int row) {
    Item item;
    item.value = options_menu::value(row);
    return item;
}

// Why the enhanced graphics cannot be changed now, or nothing when they can.
const char* enhanced_reason() {
    if (!kEnhancedBuilt) {
        return nullptr;
    }
    if (!gl::enhanced_rendering_enabled()) {
        return "Off. Switch it on under Mods in the launcher: bounced light, modern shadows and "
               "ambient occlusion, from the game's next start.";
    }
    if (!gl::enhanced_rendering_available()) {
        return "It needs the Vulkan renderer, and this run draws with OpenGL. Vulkan 1.3 could not "
               "start on this device or driver.";
    }
    if (!enhanced_graphics_available()) {
        return "This level's lighting is not prepared yet. In the launcher, Mods, Enhanced "
               "graphics "
               "prepares it from your levels; the game shows the original lighting until then.";
    }
    return nullptr;
}

bool window_full_screen() {
    return s.window != nullptr && (SDL_GetWindowFlags(s.window) & SDL_WINDOW_FULLSCREEN) != 0;
}

Item item(Id id, std::uint8_t where) {
    using namespace options_menu;
    settings::Settings& set = settings::get();
    Item it;
    switch (id) {
        // ---- Graphics ----
        case Id::FullScreen:
            it.on = window_full_screen();
            return it;
        case Id::DisplayMode:
            return row_choice(kDisplayMode);
        case Id::Monitor:
            return row_choice(kMonitor);
        case Id::MenuSize: {
            const int at = size_index();
            it.value =
                set.menu_size == 100 ? std::string("Auto") : std::format("{}%", set.menu_size);
            it.can_lower = at > 0;
            it.can_raise = at < static_cast<int>(kSizes.size()) - 1;
            return it;
        }
        case Id::Picture:
            it = row_choice(kAspect);
            if (where == menu::kTouch && set.aspect == settings::Aspect::Window) {
                it.value = "Fill screen";  // a window's shape, said as a phone's
            }
            return it;
        case Id::GameplayFov:
            return it;
        case Id::Hud:
            it = row_choice(kHud);
            it.shown = set.aspect != settings::Aspect::Standard;
            return it;
        case Id::RenderScale:
            return row_choice(kRenderScale);
        case Id::Msaa:
            return row_choice(kMsaa);
        case Id::Anisotropy:
            return row_choice(kAnisotropy);
        case Id::DrawDistance:
            return row_choice(kDrawDistance);
        case Id::ActiveDistance:
            return row_choice(kActiveDistance);
        case Id::FrameRate:
            return row_choice(kFrameRate);
        case Id::FrameCap:
            it = row_choice(kFrameCap);
            it.shown = set.frame_rate == settings::FrameRate::Capped;
            it.can_lower = set.frame_cap > settings::kFrameCapMin;
            it.can_raise = set.frame_cap < settings::kFrameCapMax;
            return it;
        case Id::VSync:
            return row_choice(kVSync);
        case Id::FrameCounter:
            it.value = settings::name(set.show_fps);
            return it;
        case Id::Hdr:
            it.value = hdr::kModeNames[static_cast<int>(set.hdr)];
            return it;
        case Id::HdrPaperWhite: {
            it.shown = set.hdr != hdr::Mode::Off;
            const int at = white_index();
            it.value = set.hdr_paper_white == 0 ? std::string("System")
                                                : std::format("{} nits", set.hdr_paper_white);
            it.can_lower = at > 0;
            it.can_raise = at < static_cast<int>(kWhites.size()) - 1;
            return it;
        }
        case Id::HdrPeak: {
            it.shown = set.hdr != hdr::Mode::Off;
            const int at = peak_index();
            it.value =
                set.hdr_peak == 0 ? std::string("Display") : std::format("{} nits", set.hdr_peak);
            it.can_lower = at > 0;
            it.can_raise = at < static_cast<int>(kPeaks.size()) - 1;
            return it;
        }
        case Id::HdrHighlights:
        case Id::HdrSdrHue:
        case Id::HdrSdrChroma:
        case Id::HdrBlowout:
            it.shown = set.hdr != hdr::Mode::Off;
            return it;
        case Id::HdrStatus:
            it.shown = set.hdr != hdr::Mode::Off;
            it.value = frontend::hdr_status();
            return it;
        case Id::LowLatency:
            it.on = set.low_latency_frames;
            return it;
        case Id::ResponsiveControls:
            it.on = set.responsive_controls;
            return it;
        case Id::GpuBreadcrumbs:
            it.shown = gl::is_vulkan();
            it.on = set.gpu_breadcrumbs;
            return it;

        // ---- Enhanced ----
        case Id::EnhancedStatus: {
            const char* reason = enhanced_reason();
            it.shown = reason != nullptr;
            it.value = reason != nullptr ? reason : "";
            return it;
        }
        case Id::EnhancedGraphics:
            it.shown = enhanced_ready();
            it.on = set.enhanced_graphics;
            return it;
        case Id::ModernShadows:
            it.shown = enhanced_on();
            it.on = set.enhanced_shadows;
            return it;
        case Id::ShadowQuality: {
            it.shown = enhanced_on() && set.enhanced_shadows;
            const int quality = static_cast<int>(set.shadow_quality);
            it.value = renderer::kShadowQualityNames[quality];
            it.can_lower = quality > 0;
            it.can_raise = quality < static_cast<int>(renderer::ShadowQuality::Count) - 1;
            return it;
        }
        case Id::RimLight:
            it.shown = enhanced_on();
            it.on = set.rim_light;
            return it;
        case Id::RimShadowLift:
            it.shown = enhanced_on() && set.rim_light;
            it.on = set.rim_shadow_lift;
            return it;
        case Id::BounceStrength:
            it.shown = enhanced_on();
            it.value = std::format("{:g}x", set.probe_gi_multiplier);
            it.can_lower = set.probe_gi_multiplier > 0.05f;
            it.can_raise = set.probe_gi_multiplier < 4.95f;
            return it;
        case Id::AmbientOcclusion:
            it.shown = enhanced_on();
            it.on = set.gtao;
            return it;
        case Id::OcclusionQuality: {
            it.shown = enhanced_on() && set.gtao;
            const bool quality = set.gtao_settings.quality == renderer::GtaoQuality::Quality;
            it.value = quality ? "Quality" : "Performance";
            it.can_lower = quality;
            it.can_raise = !quality;
            return it;
        }
        // The hierarchy is relevant only while enhanced ambient occlusion is enabled.
        case Id::OcclusionHiZ:
            it.shown = enhanced_on() && set.gtao;
            it.on = set.gtao_settings.hi_z;
            return it;
        case Id::TextureUpscaling: {
            it.shown = enhanced_on();
            const int filter = static_cast<int>(set.texture_filter);
            it.value = settings::name(set.texture_filter);
            it.can_lower = filter > 0;
            it.can_raise = filter < static_cast<int>(renderer::TextureFilter::Count) - 1;
            return it;
        }
        case Id::Upscaling: {
            it.shown = enhanced_on() && enhanced::Fsr3::available();
            const std::string_view upscaler = it.shown ? enhanced::Fsr3::active_upscaler() : "none";
            if (upscaler != "none") {
                it.label = std::format("Upscaling ({})", upscaler);
            }
            constexpr int kModes = static_cast<int>(renderer::TemporalQuality::Count);
            const int mode = static_cast<int>(set.temporal_quality);
            it.value = renderer::kTemporalNames[mode];
            it.can_lower = mode > 0;
            it.can_raise = mode < kModes - 1;
            return it;
        }
        case Id::Sharpening:
            it.shown = enhanced_on() && enhanced::Fsr3::available()
                       && set.temporal_quality != renderer::TemporalQuality::Off;
            it.value = percent(set.temporal_sharpness);
            it.can_lower = set.temporal_sharpness > 0.025f;
            it.can_raise = set.temporal_sharpness < 0.975f;
            return it;
        case Id::MotionBlur:
            // In the upscaler's module (enhanced/fsr3), even with upscaling off.
            it.shown = enhanced_on() && enhanced::Fsr3::available();
            it.on = set.motion_blur;
            return it;
        case Id::ColorCorrection:
            it.shown = enhanced_on();
            it.on = set.agx_preview;
            return it;
        case Id::Brightness:
            it.shown = enhanced_on() && set.agx_preview;
            it.value = percent(set.agx_brightness);
            it.can_lower = set.agx_brightness > 0.025f;
            it.can_raise = set.agx_brightness < 0.975f;
            return it;
        case Id::EnhancedReset:
            it.shown = enhanced_ready();
            if (s.menu_confirm_reset) {
                it.label = "Press again to reset";
            }
            return it;
        case Id::EnhancedAdvanced:
            it.shown = gl::enhanced_rendering_available();
            it.on = s.enhanced_settings;
            return it;

        // ---- Game ----
        case Id::Volume:
            it.value = std::format("{}%", set.volume);
            it.can_lower = set.volume > 0;
            it.can_raise = set.volume < 100;
            return it;
        case Id::MuteInBackground:
            it.on = set.mute_unfocused;
            return it;
        case Id::SkipCutscenes:
            it.on = set.skip_cutscenes;
            return it;
        case Id::GameFixes:
            it.shown = fixes::count() > 0;
            return it;

        // ---- Controls ----
        case Id::ControlsFile:
        case Id::ControlsPlayer:
        case Id::ControlsPlayerReset:
        case Id::ControlsStatus:
        case Id::ControllerBindings:
        case Id::LeftStick:
        case Id::RightStick:
        case Id::TriggerPoint:
        case Id::FaceButtons:
        case Id::PadBackground:
        case Id::RumbleStrength:
        case Id::RumbleStyle:
        case Id::RumbleTriggers:
        case Id::RumblePhone:
        case Id::ConnectedControllers:
        case Id::PlayersReset:
        case Id::PlayerOne:
        case Id::PlayerTwo:
        case Id::MenuOpen:
        case Id::ControlsReset: {
            // debug/menu_controls.cpp's.
            const ControlsRow r = controls_row(id);
            it.shown = r.shown;
            it.label = r.label;
            it.on = r.on;
            it.value = r.value;
            it.can_lower = r.can_lower;
            it.can_raise = r.can_raise;
            return it;
        }
        case Id::TouchActionButtons:
            it.on = set.touch_style == 0;
            return it;
        case Id::TouchButtonSize:
            it.value = std::format("{}%", set.touch_scale);
            it.can_lower = set.touch_scale > settings::kTouchScales[0];
            it.can_raise =
                set.touch_scale < settings::kTouchScales[std::size(settings::kTouchScales) - 1];
            return it;
        case Id::TouchCameraSpeed:
            it.value = std::format("{}%", set.touch_camera);
            it.can_lower = set.touch_camera > settings::kTouchCameraSpeeds[0];
            it.can_raise =
                set.touch_camera
                < settings::kTouchCameraSpeeds[std::size(settings::kTouchCameraSpeeds) - 1];
            return it;
        case Id::TouchMoveControls:
            return it;

        // ---- Developer ----
        case Id::DebugOverlay:
            it.on = s.visible;
            return it;
        case Id::MobyPicker:
            it.on = picker_enabled();
            return it;
        case Id::ObjectViewer:
            it.shown = s.developer;
            it.on = s.object_viewer.open;
            return it;
        case Id::MeatProxyReview:
            it.shown = s.developer;
            it.on = s.meat_proxy_review;
            return it;
        case Id::LevelSelect:
            it.shown = s.developer;
            return it;
        case Id::KeepPaused:
            it.on = s.keep_paused;
            return it;
        case Id::StepTick:
            return it;
        case Id::Speed:
            // Back is slower, on is faster: full speed at the right end.
            it.value = s.stretch == 1.0 ? "Full" : std::format("1/{}", static_cast<int>(s.stretch));
            it.can_lower = s.stretch < 16.0;
            it.can_raise = s.stretch > 1.0;
            return it;
        case Id::Screenshot:
            return it;
        case Id::RecordGameplay:
            if (capture_pending()) {
                it.label = "Cancel recording";
            } else if (capture_busy()) {
                it.label = "Stop recording";
            }
            return it;
        case Id::RecordLength:
            it.shown = !capture_pending();
            it.value = capture_length_name();
            return it;
        case Id::RecordSweep:
            it.shown = !capture_pending();
            it.label = std::format("Graphics sweep ({:.0f} s)", settings_sweep_seconds());
            return it;
        case Id::RecordFolder:
            return it;
        case Id::SlowCpu:
            it.shown = slow_cpu::available();
            it.value = slow_cpu_name();
            return it;
        case Id::Freecam:
            it.on = s.camera_tools.enabled;
            return it;
        case Id::FreecamMode:
            it.value = s.camera_tools.mode == CameraDebugMode::FreeLook ? "Free look"
                                                                        : "Culling inspection";
            return it;
        case Id::FreecamInspect: {
            it.shown = s.camera_tools.mode == CameraDebugMode::CullingInspection;
            const int target = std::clamp(s.camera_tools.culling_target, 0, 4);
            it.value = kInspectNames[static_cast<std::size_t>(target)];
            it.can_lower = target > 0;
            it.can_raise = target < 4;
            return it;
        }
        case Id::FreecamSpeed: {
            const int at = fly_speed_index(s.camera_tools.speed);
            it.value = std::format("{:.0f} units/s", s.camera_tools.speed);
            it.can_lower = at > 0;
            it.can_raise = at < static_cast<int>(kFlySpeeds.size()) - 1;
            return it;
        }
        case Id::FreecamReset:
        case Id::FreecamExit:
            return it;
        case Id::FreecamHelp:
            // The keys are drawn as a list (freecam_help); the words are only for culling.
            it.value = "";
            if (s.camera_tools.mode == CameraDebugMode::CullingInspection) {
                it.value = "Yellow: culling frustum. Shadow maps: red, green, blue, purple (near "
                           "to far). Objects not submitted by the game cannot be revealed here.";
            }
            return it;

        // ---- Program ----
        case Id::Quit:
            return it;

        case Id::Count:
            break;
    }
    it.shown = false;
    return it;
}

// Does what entry `id` does: a switch turns over, a choice moves `by`, an action runs. True when
// the menu should close for it (a window opens over the game, or it waits for the menu to go).
bool act(Id id, int by) {
    using namespace options_menu;
    settings::Settings& set = settings::get();
    switch (id) {
        // ---- Graphics ----
        case Id::FullScreen:
            press(SDLK_F11);
            return false;
        case Id::DisplayMode:
            change_setting(kDisplayMode, by);
            return false;
        case Id::MenuSize:
            set.menu_size = kSizes[static_cast<std::size_t>(
                std::clamp(size_index() + by, 0, static_cast<int>(kSizes.size()) - 1)
            )];
            changed();
            return false;
        case Id::Monitor:
            change_setting(kMonitor, by);
            return false;
        case Id::Picture:
            change_setting(kAspect, by);
            return false;
        case Id::GameplayFov:
            return false;
        case Id::Hud:
            change_setting(kHud, by);
            return false;
        case Id::RenderScale:
            change_setting(kRenderScale, by);
            return false;
        case Id::Msaa:
            change_setting(kMsaa, by);
            return false;
        case Id::Anisotropy:
            change_setting(kAnisotropy, by);
            return false;
        case Id::DrawDistance:
            change_setting(kDrawDistance, by);
            return false;
        case Id::ActiveDistance:
            change_setting(kActiveDistance, by);
            return false;
        case Id::FrameRate:
            change_setting(kFrameRate, by);
            return false;
        case Id::FrameCap:
            change_setting(kFrameCap, by);
            return false;
        case Id::VSync:
            change_setting(kVSync, by);
            return false;
        case Id::FrameCounter:
            change_setting(kShowFps, by);
            return false;
        case Id::Hdr: {
            // From Off, it counts from the next start (frontend's apply_hdr says so).
            const int count = static_cast<int>(hdr::Mode::Count);
            set.hdr = static_cast<hdr::Mode>((static_cast<int>(set.hdr) + by + count) % count);
            changed();
            return false;
        }
        case Id::HdrPaperWhite:
            set.hdr_paper_white = kWhites[static_cast<std::size_t>(
                std::clamp(white_index() + by, 0, static_cast<int>(kWhites.size()) - 1)
            )];
            changed();
            return false;
        case Id::HdrPeak:
            set.hdr_peak = kPeaks[static_cast<std::size_t>(
                std::clamp(peak_index() + by, 0, static_cast<int>(kPeaks.size()) - 1)
            )];
            changed();
            return false;
        case Id::HdrHighlights:
        case Id::HdrSdrHue:
        case Id::HdrSdrChroma:
        case Id::HdrBlowout:
            return false;  // sliders (draw_entry)
        case Id::HdrStatus:
            return false;
        case Id::LowLatency:
            change_setting(kLowLatency, 1);
            return false;
        case Id::ResponsiveControls:
            change_setting(kResponsiveControls, 1);
            return false;
        case Id::GpuBreadcrumbs:
            set.gpu_breadcrumbs = !set.gpu_breadcrumbs;
            settings::save();
            return false;

        // ---- Enhanced ----
        case Id::EnhancedStatus:
            return false;
        case Id::EnhancedGraphics:
            set.enhanced_graphics = !set.enhanced_graphics;
            changed();
            return false;
        case Id::ModernShadows:
            set.enhanced_shadows = !set.enhanced_shadows;
            changed();
            return false;
        case Id::ShadowQuality: {
            const int last = static_cast<int>(renderer::ShadowQuality::Count) - 1;
            set.shadow_quality = static_cast<renderer::ShadowQuality>(
                std::clamp(static_cast<int>(set.shadow_quality) + by, 0, last)
            );
            changed();
            return false;
        }
        case Id::RimLight:
            set.rim_light = !set.rim_light;
            changed();
            return false;
        case Id::RimShadowLift:
            set.rim_shadow_lift = !set.rim_shadow_lift;
            changed();
            return false;
        case Id::BounceStrength:
            set.probe_gi_multiplier = stepped(set.probe_gi_multiplier, 0.25f, by, 0.0f, 5.0f);
            changed();
            return false;
        case Id::AmbientOcclusion:
            set.gtao = !set.gtao;
            changed();
            return false;
        case Id::OcclusionQuality:
            set.gtao_settings.quality = set.gtao_settings.quality == renderer::GtaoQuality::Quality
                                            ? renderer::GtaoQuality::Performance
                                            : renderer::GtaoQuality::Quality;
            changed();
            return false;
        // Persist the hierarchy choice with the remaining ambient-occlusion options.
        case Id::OcclusionHiZ:
            set.gtao_settings.hi_z = !set.gtao_settings.hi_z;
            changed();
            return false;
        case Id::TextureUpscaling: {
            const int last = static_cast<int>(renderer::TextureFilter::Count) - 1;
            set.texture_filter = static_cast<renderer::TextureFilter>(
                std::clamp(static_cast<int>(set.texture_filter) + by, 0, last)
            );
            changed();
            return false;
        }
        case Id::Upscaling: {
            constexpr int kModes = static_cast<int>(renderer::TemporalQuality::Count);
            set.temporal_quality = static_cast<renderer::TemporalQuality>(
                std::clamp(static_cast<int>(set.temporal_quality) + by, 0, kModes - 1)
            );
            changed();
            return false;
        }
        case Id::Sharpening:
            set.temporal_sharpness = stepped(set.temporal_sharpness, 0.05f, by, 0.0f, 1.0f);
            changed();
            return false;
        case Id::MotionBlur:
            set.motion_blur = !set.motion_blur;
            changed();
            return false;
        case Id::ColorCorrection:
            set.agx_preview = !set.agx_preview;
            changed();
            return false;
        case Id::Brightness:
            set.agx_brightness = stepped(set.agx_brightness, 0.05f, by, 0.0f, 1.0f);
            changed();
            return false;
        case Id::EnhancedReset:
            // Asked twice, so a stray press keeps the player's choices.
            if (s.menu_confirm_reset) {
                reset_enhanced();
                changed();
            }
            s.menu_confirm_reset = !s.menu_confirm_reset;
            return false;
        case Id::EnhancedAdvanced:
            s.enhanced_settings = !s.enhanced_settings;
            return s.enhanced_settings;

        // ---- Game ----
        case Id::Volume:
            settings::set_volume(set.volume + by * 10);
            settings::save();
            return false;
        case Id::MuteInBackground:
            change_setting(kMuteUnfocused, 1);
            return false;
        case Id::SkipCutscenes:
            change_setting(kSkipCutscenes, 1);
            return false;
        case Id::GameFixes:
            return false;  // its switches, one a fix (draw_fixes)

        // ---- Controls ----
        case Id::ControlsFile:
        case Id::ControlsPlayer:
        case Id::ControlsPlayerReset:
        case Id::ControlsStatus:
        case Id::ControllerBindings:
        case Id::LeftStick:
        case Id::RightStick:
        case Id::TriggerPoint:
        case Id::FaceButtons:
        case Id::PadBackground:
        case Id::RumbleStrength:
        case Id::RumbleStyle:
        case Id::RumbleTriggers:
        case Id::RumblePhone:
        case Id::ConnectedControllers:
        case Id::PlayersReset:
        case Id::PlayerOne:
        case Id::PlayerTwo:
        case Id::MenuOpen:
        case Id::ControlsReset:
            controls_act(id, by);
            return false;
        case Id::TouchActionButtons:
            set.touch_style = set.touch_style == 0 ? 1 : 0;
            settings::save();
            return false;
        case Id::TouchButtonSize:
            set.touch_scale = settings::step_listed(settings::kTouchScales, set.touch_scale, by);
            settings::save();
            return false;
        case Id::TouchCameraSpeed:
            set.touch_camera =
                settings::step_listed(settings::kTouchCameraSpeeds, set.touch_camera, by);
            settings::save();
            return false;
        case Id::TouchMoveControls:
            edit_touch(true);  // shuts the menu itself
            return false;

        // ---- Developer ----
        case Id::DebugOverlay:
            set_visible(!s.visible);
            return false;
        case Id::MobyPicker:
            set_picker_enabled(!picker_enabled());
            return false;
        case Id::ObjectViewer:
            s.object_viewer.open = !s.object_viewer.open;
            if (s.object_viewer.open) {
                set_picker_enabled(true);
            }
            return false;
        case Id::MeatProxyReview:
            s.meat_proxy_review = !s.meat_proxy_review;
            return s.meat_proxy_review;
        case Id::LevelSelect:
            press(SDLK_F1);
            return true;
        case Id::KeepPaused:
            s.keep_paused = !s.keep_paused;
            return false;
        case Id::StepTick:
            s.keep_paused = true;
            s.step = true;
            return false;
        case Id::Speed:
            s.stretch = by < 0 ? std::min(s.stretch * 2.0, 16.0) : std::max(s.stretch / 2.0, 1.0);
            return false;
        case Id::Screenshot:
            // Taken once the menu is out of the picture (menu_closed).
            s.shot_when_shut = true;
            return true;
        case Id::RecordGameplay:
            if (capture_pending()) {
                toggle_capture(true);  // calls it off
                return false;
            }
            if (capture_busy()) {
                toggle_capture(false);
                return false;
            }
            toggle_capture(true);  // starts a moment after the menu has gone
            return true;
        case Id::RecordLength:
            step_capture_length(by);
            return false;
        case Id::RecordSweep:
            toggle_settings_sweep();
            return true;
        case Id::RecordFolder:
            open_capture_folder();
            return false;
        case Id::SlowCpu:
            step_slow_cpu(by);
            return false;
        case Id::Freecam:
            s.camera_tools.enabled = !s.camera_tools.enabled;
            return false;
        case Id::FreecamMode:
            s.camera_tools.mode = s.camera_tools.mode == CameraDebugMode::FreeLook
                                      ? CameraDebugMode::CullingInspection
                                      : CameraDebugMode::FreeLook;
            return false;
        case Id::FreecamInspect:
            s.camera_tools.culling_target = std::clamp(s.camera_tools.culling_target + by, 0, 4);
            return false;
        case Id::FreecamSpeed:
            s.camera_tools.speed = kFlySpeeds[static_cast<std::size_t>(std::clamp(
                fly_speed_index(s.camera_tools.speed) + by,
                0,
                static_cast<int>(kFlySpeeds.size()) - 1
            ))];
            return false;
        case Id::FreecamReset:
            s.camera_reset = true;
            return false;
        case Id::FreecamExit:
            s.camera_tools = {};
            s.camera_alt = 0;
            s.camera_right_mouse = false;
            return false;
        case Id::FreecamHelp:
            return false;

        // ---- Program ----
        case Id::Quit:
            s.quit_asked = true;  // its confirmation first (draw_entry)
            s.quit_focus = true;
            return false;

        case Id::Count:
            break;
    }
    return false;
}

// The game fixes (fixes.h): a switch each. On is the port's fix, off the console's behaviour.
void draw_fixes() {
    for (int i = 0; i < fixes::count(); ++i) {
        if (menu_switch(fixes::fix(i).name, fixes::on(i))) {
            change_setting(options_menu::kRows + 1 + i, 1);
        }
        note_hover(static_cast<int>(Id::GameFixes));
    }
}

}  // namespace

void note_hover(int entry) {
    // The focus a controller or the keyboard moves wins over a pointer left over another row.
    const bool moved = ImGui::GetIO().NavVisible;
    if (moved ? ImGui::IsItemFocused() : ImGui::IsItemHovered() || ImGui::IsItemFocused()) {
        s.menu_hover = entry;
    }
}

void press_entry(Id id) {
    if (act(id, 1)) {
        set_menu(false);
    }
}

std::string entry_label(Id id, std::uint8_t where) {
    const Item it = item(id, where);
    return it.label.empty() ? std::string(menu::entry(id).label) : it.label;
}

void quit_game() {
    // As the window's close button: the frame loop ends the program normally, so the launcher
    // sees an ordinary exit.
    log::info("exit: chosen in the menu, closing the game");
    SDL_Event quit;
    SDL_zero(quit);
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}

void quit_confirm_rows(bool note) {
    if (note) {
        menu_note("Quit the game? Progress since the last save is lost.", ui().amber);
    }
    if (s.quit_focus) {
        s.quit_focus = false;
        s.menu_focus_first = true;
    }
    if (menu_row("Back to the game")) {
        s.quit_asked = false;
        set_menu(false);
    }
    if (menu_row("Quit game", ui().err)) {
        quit_game();
    }
}

namespace {

// The freecam's keys, for the device in hand: the controller's (on Android always, it has no
// keys of its own) or the keyboard and mouse.
void freecam_help(std::uint8_t where) {
    if (where == menu::kTouch || s.menu_motion.pad) {
        const SDL_GamepadType type = pad_type();
        const bool sony = type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4
                          || type == SDL_GAMEPAD_TYPE_PS5;
        const bool nintendo = type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO
                              || type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
        const std::string l2 = sony ? "L2" : nintendo ? "ZL" : "LT";
        const std::string r2 = sony ? "R2" : nintendo ? "ZR" : "RT";
        const std::string l3 = sony ? "L3" : "LS";
        const std::string r3 = sony ? "R3" : "RS";
        menu_key_rows({
            {{l3}, "Move"},
            {{r3}, "Look"},
            {{r2, l2}, "Up, down"},
            {{pad_name(type, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER),
              pad_name(type, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)},
             "Faster, slower"},
            {{l3, r3}, s.developer ? "Both: freecam on and off" : "Both: freecam off"},
        });
    } else {
        menu_key_rows({
            {{"W", "A", "S", "D"}, "Move"},
            {{"Alt"}, "Hold to look (or RMB)"},
            {{"E", "Q"}, "Up, down"},
            {{"Shift", "Ctrl"}, "Faster, slower"},
        });
    }
}

}  // namespace

void draw_entry(Id id, std::uint8_t where) {
    const menu::Entry& e = menu::entry(id);
    if (!menu::offered(e, where)) {
        return;
    }
    if (id == Id::Quit && s.quit_asked) {
        quit_confirm_rows();
        return;
    }
    const Item it = item(id, where);
    if (!it.shown) {
        return;
    }
    const std::string label = it.label.empty() ? std::string(e.label) : it.label;
    bool close = false;
    switch (e.kind) {
        case menu::Kind::Switch:
            if (menu_switch(label.c_str(), it.on)) {
                close = act(id, 1);
            }
            break;
        case menu::Kind::Choice:
            if (const int by = menu_choice(label.c_str(), it.value, it.can_lower, it.can_raise)) {
                close = act(id, by);
            }
            break;
        case menu::Kind::Slider: {
            const Slider slider = slider_of(id);
            int& value = *slider.value;
            const int next = menu_percent_slider(
                label.c_str(), value, slider.minimum, slider.maximum, slider.step
            );
            // Apply live and save once the mouse drag ends, or immediately for a discrete step.
            const bool edited = next != value;
            if (edited) {
                value = next;
                s.settings_changed = true;
            }
            if (ImGui::IsItemDeactivated() || (edited && !ImGui::IsMouseDown(0))) {
                settings::save();
            }
            break;
        }
        case menu::Kind::Action:
            if (menu_row(label.c_str(), id == Id::Quit ? ui().err : ui().text)) {
                close = act(id, 1);
            }
            break;
        case menu::Kind::Note:
            if (id == Id::FreecamHelp) {
                freecam_help(where);
                if (it.value.empty()) {
                    return;
                }
            }
            menu_note(it.value.c_str());
            return;
        case menu::Kind::Fixes:
            draw_fixes();
            return;
        case menu::Kind::Panel:
            draw_controls_panel(id);
            return;
    }
    note_hover(static_cast<int>(id));
    if (close) {
        set_menu(false);
    }
}

namespace {

bool shown_now(const menu::Entry& e, std::uint8_t where) {
    return menu::offered(e, where) && item(e.id, where).shown;
}

}  // namespace

bool tab_shown(menu::Tab tab, std::uint8_t where) {
    for (const menu::Entry& e : menu::entries()) {
        if (e.tab == tab && shown_now(e, where)) {
            return true;
        }
    }
    return false;
}

bool controller_section(std::string_view section) {
    return section != "Touch controls" && section != "Menu";
}

bool draw_tab(menu::Tab tab, std::uint8_t where, const SectionFilter& only) {
    const auto all = menu::entries();
    bool any = false;
    for (std::size_t i = 0; i < all.size();) {
        const menu::Entry& first = all[i];
        // The section's entries: those after it with its tab and heading.
        std::size_t end = i + 1;
        while (end < all.size() && all[end].tab == first.tab
               && std::string_view(all[end].section) == first.section) {
            ++end;
        }
        if (first.tab == tab && (!only || only(first.section))) {
            bool shown = false;
            for (std::size_t k = i; k < end && !shown; ++k) {
                shown = shown_now(all[k], where);
            }
            if (shown) {
                menu_heading(first.section);
                card_begin();
                for (std::size_t k = i; k < end; ++k) {
                    draw_entry(all[k].id, where);
                }
                card_end();
                any = true;
            }
        }
        i = end;
    }
    return any;
}

int mod_menu_count() {
    return static_cast<int>(mods::developer_menus().size());
}

std::string mod_menu_title(int index) {
    const auto& menus = mods::developer_menus();
    return index >= 0 && index < static_cast<int>(menus.size())
               ? menus[static_cast<std::size_t>(index)].title
               : std::string();
}

// A mod's menu: its items without a group on one card, then a heading and a card for each group,
// in the order they first come. A press queues the item's action for the mod.
void draw_mod_menu(int index) {
    const auto& menus = mods::developer_menus();
    if (index < 0 || index >= static_cast<int>(menus.size())) {
        return;
    }
    const mods::DeveloperMenu& menu = menus[static_cast<std::size_t>(index)];
    const auto row = [&](const mods::DeveloperMenuItem& item) {
        const std::string label =
            item.shortcut.empty() ? item.label : std::format("{}  ({})", item.label, item.shortcut);
        ImGui::PushID(&item);
        if (menu_row(label.c_str())) {
            mods::queue_developer_menu_action(menu.mod, item.action);
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered() || ImGui::IsItemFocused()) {
            s.menu_hover = -1;
        }
    };
    const bool loose = std::any_of(menu.items.begin(), menu.items.end(), [](const auto& item) {
        return item.group.empty();
    });
    if (loose) {
        menu_heading(menu.title.c_str());
        card_begin();
        for (const auto& item : menu.items) {
            if (item.group.empty()) {
                row(item);
            }
        }
        card_end();
    }
    for (std::size_t i = 0; i < menu.items.size(); ++i) {
        const std::string& group = menu.items[i].group;
        const bool first = !group.empty()
                           && std::none_of(
                               menu.items.begin(),
                               menu.items.begin() + static_cast<std::ptrdiff_t>(i),
                               [&](const auto& prior) { return prior.group == group; }
                           );
        if (!first) {
            continue;
        }
        menu_heading(group.c_str());
        card_begin();
        for (const auto& item : menu.items) {
            if (item.group == group) {
                row(item);
            }
        }
        card_end();
    }
}

}  // namespace openrac::frontend::debug_overlay
