// SPDX-License-Identifier: GPL-3.0-or-later
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "game/common/settings.h"
#include "tests/check.h"

namespace {
// Temporal presets round-trip by stable file keys, and malformed values retain the enhanced default.
void test_temporal_settings(const std::filesystem::path& file) {
    using namespace openrac;

    // These are the five published player choices, including an explicit opt-out.
    for (std::size_t n = 0; n < std::size(renderer::kTemporalKeys); ++n) {
        std::ofstream(file) << "temporal_quality = " << renderer::kTemporalKeys[n]
                            << "\ntemporal_sharpness = 0.75\n"
                            << "motion_blur = 1\nmotion_blur_strength = 0.8\n";
        settings::load(file);
        CHECK(settings::get().temporal_quality == static_cast<renderer::TemporalQuality>(n));
        CHECK(settings::get().temporal_sharpness == .75f);
        CHECK(settings::get().motion_blur);
        CHECK(settings::get().motion_blur_strength == .8f);
        settings::save();
        settings::load(file);
        CHECK(settings::get().temporal_quality == static_cast<renderer::TemporalQuality>(n));
        CHECK(settings::get().motion_blur);
        CHECK(settings::get().motion_blur_strength == .8f);
    }
    std::ofstream(file) << "temporal_quality = unsupported\ntemporal_sharpness = nan\nmotion_blur "
                           "= invalid\nmotion_blur_strength = nan\n";
    settings::load(file);
    CHECK(settings::get().temporal_quality == settings::Settings{}.temporal_quality);
    CHECK(settings::get().temporal_sharpness == .2f);
    CHECK(settings::get().motion_blur);
    CHECK(settings::get().motion_blur_strength == .5f);
    CHECK(!settings::get().motion_blur_settings.quality);
    for (const bool quality : {true, false}) {
        std::ofstream(file) << "motion_blur_quality = " << (quality ? "quality" : "performance")
                            << "\n";
        settings::load(file);
        CHECK(settings::get().motion_blur_settings.quality == quality);
        settings::save();
        settings::load(file);
        CHECK(settings::get().motion_blur_settings.quality == quality);
    }
}

// Every Draw distance step survives a save and a load, Unlimited (0) too.
void test_draw_distance(const std::filesystem::path& file) {
    using namespace openrac;
    for (const int value : settings::kDrawDistances) {
        std::ofstream(file) << "draw_distance = " << value << "\n";
        settings::load(file);
        CHECK(settings::get().draw_distance == value);
        settings::save();
        settings::load(file);
        CHECK(settings::get().draw_distance == value);
    }
}

void test_lily_momentum(const std::filesystem::path& file) {
    using namespace openrac;
    for (const float value : {0.f, .25f, .65f, 1.f}) {
        std::ofstream(file) << "lily_momentum = " << value << "\n";
        settings::load(file);
        CHECK(settings::get().lily_momentum == value);
        settings::save();
        settings::load(file);
        CHECK(settings::get().lily_momentum == value);
    }
    for (const char* invalid : {"nan", "inf", "-0.1", "1.1", "bad"}) {
        std::ofstream(file) << "lily_momentum = " << invalid << "\n";
        settings::load(file);
        CHECK(settings::get().lily_momentum == .25f);
    }
}

// HDR output: off with no key (the picture as always), its two modes and levels round-trip, and
// values out of range keep the defaults.
void test_hdr_settings(const std::filesystem::path& file) {
    using namespace openrac;
    std::ofstream(file) << "vsync = on\n";
    settings::load(file);
    CHECK(settings::get().hdr == hdr::Mode::Off);
    CHECK(settings::get().hdr_paper_white == 0 && settings::get().hdr_peak == 0);
    for (std::size_t n = 0; n < std::size(hdr::kModeKeys); ++n) {
        std::ofstream(file) << "hdr = " << hdr::kModeKeys[n]
                            << "\nhdr_paper_white = 250\nhdr_peak = 1400\n";
        settings::load(file);
        CHECK(settings::get().hdr == static_cast<hdr::Mode>(n));
        settings::save();
        settings::load(file);
        CHECK(settings::get().hdr == static_cast<hdr::Mode>(n));
        CHECK(settings::get().hdr_paper_white == 250 && settings::get().hdr_peak == 1400);
    }
    // The launcher's word over the file's, and another word changes nothing.
    CHECK(settings::set_hdr_mode("off") && settings::get().hdr == hdr::Mode::Off);
    CHECK(settings::set_hdr_mode("on") && settings::get().hdr == hdr::Mode::On);
    CHECK(!settings::set_hdr_mode("bright") && settings::get().hdr == hdr::Mode::On);
    // The ranges' tops: the brightest screens on sale.
    std::ofstream(file) << "hdr_paper_white = 1000\nhdr_peak = 10000\n";
    settings::load(file);
    CHECK(settings::get().hdr_paper_white == 1000 && settings::get().hdr_peak == 10000);
    std::ofstream(file) << "hdr = bright\nhdr_paper_white = 5000\nhdr_peak = -1\n";
    settings::load(file);
    CHECK(settings::get().hdr == hdr::Mode::Off);
    CHECK(settings::get().hdr_paper_white == 0 && settings::get().hdr_peak == 0);
    // The look's sliders: the plain HDR look by default, kept through a save, the default when
    // out of range.
    std::ofstream(file) << "vsync = on\n";
    settings::load(file);
    CHECK(settings::get().hdr_highlights == 100 && settings::get().hdr_sdr_hue == 0);
    CHECK(settings::get().hdr_sdr_chroma == 0 && settings::get().hdr_blowout == 0);
    std::ofstream(file) << "hdr_highlights = 40\nhdr_sdr_hue = 70\nhdr_sdr_chroma = 30\n"
                        << "hdr_blowout = 100\n";
    settings::load(file);
    settings::save();
    settings::load(file);
    CHECK(settings::get().hdr_highlights == 40 && settings::get().hdr_sdr_hue == 70);
    CHECK(settings::get().hdr_sdr_chroma == 30 && settings::get().hdr_blowout == 100);
    std::ofstream(file) << "hdr_highlights = 120\nhdr_sdr_hue = -1\nhdr_blowout = soft\n";
    settings::load(file);
    CHECK(settings::get().hdr_highlights == 100 && settings::get().hdr_sdr_hue == 0);
    CHECK(settings::get().hdr_blowout == 0);
    // The menu's size: kept through a save, Auto (100) when out of range.
    std::ofstream(file) << "menu_size = 125\n";
    settings::load(file);
    settings::save();
    settings::load(file);
    CHECK(settings::get().menu_size == 125);
    std::ofstream(file) << "menu_size = 900\n";
    settings::load(file);
    CHECK(settings::get().menu_size == 100);
}

void test_reset_enhanced() {
    using namespace openrac;
    settings::Settings s;
    s.enhanced_rendering = true;
    s.enhanced_graphics = false;
    s.volume = 37;
    s.render_scale = 75;
    s.frame_cap = 144;
    s.texture_filter = renderer::TextureFilter::Normal;
    s.shadow_quality = renderer::ShadowQuality::Low;
    s.volumetric_quality = renderer::VolumetricQuality::High;
    s.agx_preview = false;
    s.agx_exposure = 2;
    s.agx_brightness = 1;
    s.sun_direction_override = true;
    s.probe_spheres = true;
    s.motion_blur = false;
    s.motion_blur_settings.samples = 64;
    s.lily_momentum = .8f;
    s.reset_enhanced();
    CHECK(s.enhanced_rendering && s.enhanced_graphics);
    CHECK(s.volume == 37 && s.render_scale == 75 && s.frame_cap == 144);
    CHECK(s.texture_filter == renderer::TextureFilter::EdgeAware);
    CHECK(s.shadow_quality == settings::Settings{}.shadow_quality);
    CHECK(s.volumetric_quality == renderer::VolumetricQuality::Off);
    CHECK(s.agx_preview && s.agx_exposure == .25f && s.agx_brightness == .5f);
    CHECK(!s.sun_direction_override && !s.probe_spheres);
    CHECK(s.motion_blur && s.motion_blur_settings.samples == 16);
    CHECK(s.lily_momentum == .25f);
}

// The presets set the costly options together and leave the other settings alone.
void test_enhanced_presets() {
    using namespace openrac;
    settings::Settings s;
    s.render_scale = 75;
    s.gtao_settings.half_resolution = false;
    s.volumetric_quality = renderer::VolumetricQuality::High;

    // A new file starts at a preset: Medium where the GPU is a low-power one, High elsewhere.
    CHECK(
        settings::Settings{}.enhanced_preset()
        == (settings::kHandheldDefaults ? settings::EnhancedPreset::Medium
                                        : settings::EnhancedPreset::High)
    );

    // Each preset sets its options and is then the one the settings match.
    for (const auto preset :
         {settings::EnhancedPreset::Low,
          settings::EnhancedPreset::Medium,
          settings::EnhancedPreset::High}) {
        s.apply_enhanced_preset(preset);
        CHECK(s.enhanced_preset() == preset);
    }

    s.apply_enhanced_preset(settings::EnhancedPreset::Low);
    CHECK(s.temporal_quality == renderer::TemporalQuality::Performance);
    CHECK(s.shadow_quality == renderer::ShadowQuality::Low);
    CHECK(s.shadow_filter == renderer::ShadowFilter::Fast);
    CHECK(s.gtao_settings.quality == renderer::GtaoQuality::Low);
    CHECK(!s.motion_blur);
    CHECK(s.effects_detail == renderer::render_opt::EffectsDetail::Low);

    // What a preset does not cover stays as the player set it.
    CHECK(s.render_scale == 75 && s.gtao && s.enhanced_shadows && s.agx_preview);
    CHECK(!s.gtao_settings.half_resolution);
    CHECK(s.volumetric_quality == renderer::VolumetricQuality::High);

    // Any other mix is Custom, and applying Custom changes nothing.
    s.shadow_filter = renderer::ShadowFilter::Soft;
    CHECK(s.enhanced_preset() == settings::EnhancedPreset::Custom);
    s.apply_enhanced_preset(settings::EnhancedPreset::Custom);
    CHECK(s.shadow_quality == renderer::ShadowQuality::Low);
}

// A file from before effects detail takes the detail of the preset its other values match.
void test_effects_detail_default(const std::filesystem::path& file) {
    using namespace openrac;
    std::ofstream(file) << "temporal_quality = performance\nshadow_quality = low\n"
                        << "shadow_filter = fast\ngtao_quality = low\nmotion_blur = 0\n"
                        << "texture_filter = normal\n";
    settings::load(file);
    CHECK(settings::get().effects_detail == renderer::render_opt::EffectsDetail::Low);
    CHECK(settings::get().enhanced_preset() == settings::EnhancedPreset::Low);
}

void test_agx_preview(const std::filesystem::path& file) {
    using namespace openrac;
    CHECK(settings::Settings{}.agx_preview);
    CHECK(settings::Settings{}.agx_exposure == .25f);
    CHECK(settings::Settings{}.agx_brightness == .5f);
    for (float brightness : {0.f, .5f, 1.f}) {
        std::ofstream(file) << "agx_preview = 1\nagx_exposure = 1.5\nagx_brightness = "
                            << brightness << "\n";
        settings::load(file);
        for (int n = 0; n < 2; ++n) {
            const auto& s = settings::get();
            CHECK(s.agx_exposure == 1.5f);
            CHECK(s.agx_brightness == brightness);
            CHECK(std::abs(s.adjusted_agx_exposure() - (1.5f + brightness * 1.6f - .8f)) < 1e-6f);
            settings::save();
            settings::load(file);
        }
    }
    for (const char* invalid : {"nan", "-0.01", "1.01"}) {
        std::ofstream(file) << "agx_exposure = 0.25\nagx_brightness = " << invalid << "\n";
        settings::load(file);
        CHECK(settings::get().agx_brightness == .5f);
        CHECK(settings::get().adjusted_agx_exposure() == .25f);
    }
    std::ofstream(file) << "agx_preview = invalid\nagx_exposure = nan\n";
    settings::load(file);
    CHECK(settings::get().agx_preview);
    CHECK(settings::get().agx_exposure == .25f);
    CHECK(settings::get().agx_brightness == .5f);
    std::ofstream(file) << "agx_preview = 0\nagx_exposure = 0\nagx_brightness = 0\n";
    settings::load(file);
    settings::save();
    settings::load(file);
    CHECK(!settings::get().agx_preview);
    CHECK(settings::get().agx_exposure == 0.f);
    CHECK(settings::get().agx_brightness == 0.f);
}

void test_motion_blur_tuning(const std::filesystem::path& file) {
    using namespace openrac;
    std::ofstream(
        file
    ) << "motion_blur_radius = 96\nmotion_blur_samples = 48\n"
      << "motion_blur_min_motion = 1.25\nmotion_blur_spread = 0.75\n"
      << "motion_blur_depth_tolerance = 0.025\nmotion_blur_cutscene_multiplier = 1.5\n";
    settings::load(file);
    for (int roundtrip = 0; roundtrip < 2; ++roundtrip) {
        const auto& blur = settings::get().motion_blur_settings;
        CHECK(blur.max_radius == 96 && blur.samples == 48);
        CHECK(blur.min_motion == 1.25f && blur.spread == .75f);
        CHECK(blur.depth_tolerance == .025f && blur.cutscene_multiplier == 1.5f);
        settings::save();
        settings::load(file);
    }
    std::ofstream(
        file
    ) << "motion_blur_radius = 129\nmotion_blur_samples = 7\n"
      << "motion_blur_min_motion = nan\nmotion_blur_spread = 2\n"
      << "motion_blur_depth_tolerance = -1\nmotion_blur_cutscene_multiplier = inf\n";
    settings::load(file);
    const auto& blur = settings::get().motion_blur_settings;
    CHECK(blur.max_radius == 64 && blur.samples == 16);
    CHECK(blur.min_motion == 1.f && blur.spread == .5f);
    CHECK(blur.depth_tolerance == .009f && blur.cutscene_multiplier == 1.f);
}

// Missing/invalid keys leave experimental volumetrics off; every tier survives save/load.
void test_volumetric_quality(const std::filesystem::path& file) {
    using namespace openrac;
    for (std::size_t n = 0; n < std::size(renderer::kVolumetricQualityKeys); ++n) {
        std::ofstream(file) << "volumetric_quality = " << renderer::kVolumetricQualityKeys[n]
                            << "\n";
        settings::load(file);
        CHECK(settings::get().volumetric_quality == static_cast<renderer::VolumetricQuality>(n));
        settings::save();
        settings::load(file);
        CHECK(settings::get().volumetric_quality == static_cast<renderer::VolumetricQuality>(n));
    }
    std::ofstream(file) << "volumetric_quality = invalid\n";
    settings::load(file);
    CHECK(settings::get().volumetric_quality == renderer::VolumetricQuality::Off);
    std::ofstream(file) << "";
    settings::load(file);
    CHECK(settings::get().volumetric_quality == renderer::VolumetricQuality::Off);
}

void test_shadow_quality(const std::filesystem::path& file) {
    using namespace openrac;
    // Low-power GPUs (Android, Apple Silicon) start at Medium.
    CHECK(
        settings::Settings{}.shadow_quality
        == (settings::kHandheldDefaults ? renderer::ShadowQuality::Medium
                                        : renderer::ShadowQuality::High)
    );
    for (const auto filter : {renderer::ShadowFilter::Fast, renderer::ShadowFilter::Soft}) {
        std::ofstream(file) << "shadow_filter = "
                            << renderer::kShadowFilterKeys[static_cast<int>(filter)] << "\n";
        settings::load(file);
        CHECK(settings::get().shadow_filter == filter);
        settings::save();
        settings::load(file);
        CHECK(settings::get().shadow_filter == filter);
    }
    settings::get().shadow_filter = renderer::ShadowFilter::Soft;
    for (std::size_t n = 0; n < std::size(renderer::kShadowQualityKeys); ++n) {
        std::ofstream(file) << "shadow_quality = " << renderer::kShadowQualityKeys[n] << "\n";
        settings::load(file);
        CHECK(settings::get().shadow_quality == static_cast<renderer::ShadowQuality>(n));
        settings::save();
        settings::load(file);
        CHECK(settings::get().shadow_quality == static_cast<renderer::ShadowQuality>(n));
    }
    std::ofstream(file) << "shadow_quality = invalid\n";
    settings::load(file);
    CHECK(settings::get().shadow_quality == settings::Settings{}.shadow_quality);
}

void test_sunlight_settings(const std::filesystem::path& file) {
    namespace settings = openrac::settings;
    CHECK(!settings::Settings{}.sunlight_direction());
    std::ofstream(file) << "sun_direction_override = 1\nsun_azimuth = 90\nsun_elevation = 0\n";
    settings::load(file);
    auto direction = settings::get().sunlight_direction();
    CHECK(direction.has_value());
    CHECK(std::abs((*direction)[0]) < 1e-6f);
    CHECK(std::abs((*direction)[1] + 1.f) < 1e-6f);
    CHECK(std::abs((*direction)[2]) < 1e-6f);
    settings::get().sun_elevation = 90;
    settings::save();
    settings::load(file);
    CHECK(settings::get().sun_direction_override);
    CHECK(settings::get().sun_azimuth == 90);
    direction = settings::get().sunlight_direction();
    CHECK(std::abs((*direction)[2] + 1.f) < 1e-6f);
    settings::get().sun_direction_override = false;
    CHECK(!settings::get().sunlight_direction());
    std::ofstream(file) << "sun_direction_override = bad\nsun_azimuth = nan\nsun_elevation = 91\n";
    settings::load(file);
    CHECK(!settings::get().sun_direction_override);
    CHECK(settings::get().sun_azimuth == 0);
    CHECK(settings::get().sun_elevation == 45);
}

// Persists all counter modes, wraps both directions and accepts existing boolean settings.
void test_frame_counter_modes(const std::filesystem::path& file) {
    namespace settings = openrac::settings;
    using settings::FrameCounter;
    settings::get().show_fps = FrameCounter::Off;
    for (const auto expected : {FrameCounter::Basic, FrameCounter::Timings, FrameCounter::Off}) {
        settings::step_frame_counter(1);
        CHECK(settings::get().show_fps == expected);
        settings::save();
        settings::load(file);
        CHECK(settings::get().show_fps == expected);
    }
    settings::step_frame_counter(-1);
    CHECK(settings::get().show_fps == FrameCounter::Timings);
    CHECK(std::string(settings::name(FrameCounter::Timings)) == "CPU / GPU");
    for (const char* value : {"1", "on", "true", "yes"}) {
        std::ofstream(file) << "show_fps = " << value << "\n";
        settings::load(file);
        CHECK(settings::get().show_fps == FrameCounter::Basic);
    }
    for (const char* value : {"0", "off", "3", "-1", "invalid"}) {
        std::ofstream(file) << "show_fps = " << value << "\n";
        settings::load(file);
        CHECK(settings::get().show_fps == FrameCounter::Off);
    }
}

void test_gpu_breadcrumbs_setting(const std::filesystem::path& file) {
    namespace settings = openrac::settings;
    std::ofstream(file) << "show_fps = 0\n";
    settings::load(file);
    CHECK(settings::get().gpu_breadcrumbs);
    settings::get().gpu_breadcrumbs = false;
    settings::save();
    settings::load(file);
    CHECK(!settings::get().gpu_breadcrumbs);
}

// Returns the whole of `file` as text.
std::string read_file(const std::filesystem::path& file) {
    std::ifstream in(file);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// A game's own options (keys game_*) load from the file and come back as written; a number past
// the file's range, or not a number, is no option and the caller's fallback answers.
void test_game_options_load_from_the_file(const std::filesystem::path& file) {
    namespace settings = openrac::settings;
    std::ofstream(file) << "game_language = 3\ngame_music_volume = 1024\n"
                           "game_sound_volume = 2000000\ngame_playback = loud\n";
    settings::load(file);
    CHECK(settings::game_option("game_language", 1) == 3);
    CHECK(settings::game_option("game_music_volume", -1) == 0x400);
    CHECK(settings::game_option("game_sound_volume", -1) == -1);
    CHECK(settings::game_option("game_playback", -1) == -1);
    CHECK(settings::game_option("game_missing", 7) == 7);
}

// An option the game sets is saved and read back at the next load; setting the value it already
// has leaves the file as it is (the game sets its options every frame).
void test_game_options_are_saved_when_they_change(const std::filesystem::path& file) {
    namespace settings = openrac::settings;
    std::ofstream(file) << "";
    settings::load(file);
    settings::set_game_option("game_language", 2);
    settings::load(file);
    CHECK(settings::game_option("game_language", 1) == 2);
    CHECK(read_file(file).find("game_language = 2\n") != std::string::npos);
    // A file changed behind the settings' back is not written again for an unchanged value.
    std::ofstream(file) << "# untouched\n";
    settings::set_game_option("game_language", 2);
    CHECK(read_file(file) == "# untouched\n");
    settings::set_game_option("game_language", 4);
    CHECK(read_file(file).find("game_language = 4\n") != std::string::npos);
}

// The player's bindings load as the file wrote them, and survive a save the game makes for another
// setting: save() rewrites the whole file, so a key it did not write back would be lost.
void test_bindings_survive_a_save(const std::filesystem::path& file) {
    namespace settings = openrac::settings;
    std::ofstream(file) << "bind_cross = KeyF,PadWest\nbind_l2 = Digit1,PadLeftTrigger\n"
                           "bind_square =\ngame_language = 1\n";
    settings::load(file);
    const auto& binds = settings::get().bindings;
    CHECK(binds.size() == 3);
    CHECK(binds.at("bind_cross") == "KeyF,PadWest");
    CHECK(binds.at("bind_l2") == "Digit1,PadLeftTrigger");
    // An empty value is kept as it is: it means the port's default (platform/bindings.h).
    CHECK(binds.at("bind_square").empty());

    // The game sets one of its own options, which rewrites the file.
    settings::set_game_option("game_language", 4);
    const std::string written = read_file(file);
    CHECK(written.find("bind_cross = KeyF,PadWest\n") != std::string::npos);
    CHECK(written.find("bind_l2 = Digit1,PadLeftTrigger\n") != std::string::npos);
    settings::load(file);
    CHECK(settings::get().bindings.at("bind_cross") == "KeyF,PadWest");
}

// A first start covers the screen; a file that already says how the window is keeps it.
void test_first_start_display(const std::filesystem::path& file) {
    using namespace openrac;
    using settings::DisplayMode;
    const auto mode_after = [&](const char* text, bool scripted) {
        std::filesystem::remove(file);
        if (text != nullptr) {
            std::ofstream(file) << text;
        }
        settings::load(file);
        settings::first_start_display(scripted);
        return settings::get().display_mode;
    };
    // No file at all, or one without the key (the launcher writes some keys before a first game).
    CHECK(mode_after(nullptr, false) == DisplayMode::Borderless);
    CHECK(mode_after("enhanced_rendering = 1\n", false) == DisplayMode::Borderless);
    // A player's own choice stays, whatever it is.
    CHECK(mode_after("display_mode = windowed\n", false) == DisplayMode::Windowed);
    // The old exclusive full screen is borderless now.
    CHECK(mode_after("display_mode = fullscreen\n", false) == DisplayMode::Borderless);
    CHECK(mode_after("display_mode = something\n", false) == DisplayMode::Windowed);
    // The older key: false is a choice of the window too.
    CHECK(mode_after("fullscreen = false\n", false) == DisplayMode::Windowed);
    CHECK(mode_after("fullscreen = true\n", false) == DisplayMode::Borderless);
    // Checks and tools keep the window.
    CHECK(mode_after(nullptr, true) == DisplayMode::Windowed);
    // Once saved, the file has the key, and a later start reads it as it is.
    mode_after(nullptr, false);
    settings::get().display_mode = DisplayMode::Windowed;  // the player pressed F11
    settings::save();
    settings::load(file);
    CHECK(!settings::first_start_display(false));
    CHECK(settings::get().display_mode == DisplayMode::Windowed);
    std::filesystem::remove(file);
}

// The menu bar's key: the one under Escape unless the file names another, which a save keeps.
void test_menu_bar_key(const std::filesystem::path& file) {
    using namespace openrac;
    std::filesystem::remove(file);
    settings::load(file);
    CHECK(settings::get().menu_bar_key == "`");
    settings::get().menu_bar_key = "F4";
    settings::save();
    settings::load(file);
    CHECK(settings::get().menu_bar_key == "F4");
    std::ofstream(file) << "menu_bar_key = Alt\n";
    settings::load(file);
    CHECK(settings::get().menu_bar_key == "Alt");
    std::ofstream(file) << "menu_bar_key = \n";
    settings::load(file);
    CHECK(settings::get().menu_bar_key == "`");
    // The controller button: Start+Select by default, any name kept, "none" too.
    CHECK(settings::get().menu_bar_button == "start+back");
    settings::get().menu_bar_button = "none";
    settings::save();
    settings::load(file);
    CHECK(settings::get().menu_bar_button == "none");
    std::ofstream(file) << "menu_button = misc1\n";
    settings::load(file);
    CHECK(settings::get().menu_bar_button == "misc1");
    // An older file's menu_bar_button: its old default, L3, becomes Start+Select; another pick
    // carries over; menu_button wins over it, before or after it.
    std::ofstream(file) << "menu_bar_button = leftstick\n";
    settings::load(file);
    CHECK(settings::get().menu_bar_button == "start+back");
    std::ofstream(file) << "menu_bar_button = paddle1\n";
    settings::load(file);
    CHECK(settings::get().menu_bar_button == "paddle1");
    std::ofstream(file) << "menu_button = leftstick\nmenu_bar_button = paddle1\n";
    settings::load(file);
    CHECK(settings::get().menu_bar_button == "leftstick");
    std::filesystem::remove(file);
}

}  // namespace

int main() {
    using namespace openrac;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto folder =
        std::filesystem::temp_directory_path() / ("openrac-settings-" + std::to_string(stamp));
    if (!std::filesystem::create_directory(folder)) {
        return 1;
    }
    const auto file = folder / "settings.ini";
    settings::load(file);
    CHECK(settings::get().gtao);
    CHECK(!settings::get().ceiling_area_lights);
    CHECK(!settings::get().medial_axis_debug);
    CHECK(!settings::get().environment_probe_debug);
    settings::get().medial_axis_debug = true;
    settings::get().ceiling_area_lights = true;
    settings::save();
    settings::load(file);
    CHECK(settings::get().ceiling_area_lights);
    CHECK(settings::get().medial_axis_debug);
    CHECK(settings::get().temporal_quality == settings::Settings{}.temporal_quality);
    test_temporal_settings(file);
    test_lily_momentum(file);
    test_hdr_settings(file);
    test_reset_enhanced();
    test_enhanced_presets();
    test_effects_detail_default(file);
    test_agx_preview(file);
    test_motion_blur_tuning(file);
    test_shadow_quality(file);
    test_volumetric_quality(file);
    test_sunlight_settings(file);
    test_frame_counter_modes(file);
    test_gpu_breadcrumbs_setting(file);
    test_game_options_load_from_the_file(file);
    test_game_options_are_saved_when_they_change(file);
    test_bindings_survive_a_save(file);
    test_first_start_display(file);
    test_menu_bar_key(file);
    test_draw_distance(file);
    CHECK(settings::get().texture_filter == renderer::TextureFilter::EdgeAware);
    CHECK(settings::get().anisotropy == 0);
    CHECK(settings::get().anisotropy_samples(true) == 4);
    CHECK(settings::get().anisotropy_samples(false) == 1);
    // Saving another setting keeps the backend-dependent default automatic.
    settings::save();
    settings::load(file);
    CHECK(settings::get().anisotropy == 0);
    for (int samples : settings::kAnisotropies) {
        std::ofstream(file) << "anisotropy = " << samples << "\n";
        settings::load(file);
        CHECK(settings::get().anisotropy_samples(true) == samples);
        CHECK(settings::get().anisotropy_samples(false) == samples);
        settings::save();
        settings::load(file);
        CHECK(settings::get().anisotropy == samples);
    }
    for (const char* value : {"auto", "invalid"}) {
        std::ofstream(file) << "anisotropy = " << value << "\n";
        settings::load(file);
        CHECK(settings::get().anisotropy_samples(true) == 4);
        CHECK(settings::get().anisotropy_samples(false) == 1);
    }

    for (auto filter : {renderer::TextureFilter::Normal, renderer::TextureFilter::EdgeAware}) {
        settings::get().texture_filter = filter;
        settings::save();
        settings::get().texture_filter = renderer::TextureFilter::Normal;
        settings::load(file);
        CHECK(settings::get().texture_filter == filter);
    }
    CHECK(std::string(settings::name(renderer::TextureFilter::EdgeAware)) == "Edge-aware (2x)");
    CHECK(std::string(settings::name(renderer::TextureFilter::Count)) == "Edge-aware (2x)");
    std::ofstream(file) << "texture_filter = bspline\n";
    settings::load(file);
    CHECK(settings::get().texture_filter == renderer::TextureFilter::EdgeAware);
    std::ofstream(file) << "texture_filter = catmull-rom\n";
    settings::load(file);
    CHECK(settings::get().texture_filter == renderer::TextureFilter::EdgeAware);
    std::ofstream(file) << "texture_filter = unknown\n";
    settings::load(file);
    CHECK(settings::get().texture_filter == renderer::TextureFilter::EdgeAware);
    // Enhanced rendering is off until the player turns it on (the launcher's Mods entry).
    CHECK(!settings::get().enhanced_rendering);
    CHECK(settings::get().enhanced_graphics);
    CHECK(settings::get().enhanced_shadows);
    CHECK(!settings::get().probe_spheres);
    CHECK(settings::get().rim_shadow_lift);
    CHECK(settings::get().rim_lift_exponent == 0.09f);
    CHECK(settings::get().rim_lift_max_gain == 7.1f);
    CHECK(settings::get().rim_light);
    CHECK(settings::get().rim_light_strength == 0.30f);
    CHECK(settings::get().rim_light_falloff == 6.46f);
    CHECK(settings::get().probe_gi_multiplier == 1.0f);
    CHECK(settings::get().probe_normal_bias == 0.5f);
    CHECK(settings::get().probe_view_bias == 0.5f);
    CHECK(settings::get().gtao_settings.strength == 1.0f);
    CHECK(settings::get().gtao_settings.quality == renderer::GtaoQuality::Performance);
    // An old file keeps the defaults for keys it does not contain.
    std::ofstream(file) << "gtao = 1\ngtao_strength = 0.7\n";
    settings::load(file);
    CHECK(settings::get().texture_filter == renderer::TextureFilter::EdgeAware);
    auto& options = settings::get();
    CHECK(options.enhanced_graphics && options.gtao);
    CHECK(!options.enhanced_rendering);
    CHECK(options.gtao_settings.quality == renderer::GtaoQuality::Performance);
    CHECK(options.enhanced_shadows);
    // A legacy disabled probe preference cannot disable required enhanced lighting.
    std::ofstream(file) << "enhanced_graphics = 1\nprobe_lighting = 0\n";
    settings::load(file);
    CHECK(options.enhanced_graphics);
    settings::save();
    const std::string migrated{std::istreambuf_iterator<char>(std::ifstream(file).rdbuf()), {}};
    CHECK(migrated.find("probe_lighting =") == std::string::npos);
    options.gtao_settings.strength = .7f;
    options.enhanced_rendering = true;
    options.enhanced_graphics = false;
    options.enhanced_shadows = false;
    options.probe_spheres = true;
    options.environment_probe_debug = true;
    CHECK(options.probe_normal_bias == 0.5f && options.probe_view_bias == 0.5f);
    options.rim_shadow_lift = false;
    options.rim_lift_exponent = 0.09f;
    options.rim_lift_max_gain = 8.0f;
    options.rim_light = false;
    options.rim_light_strength = 0.4f;
    options.rim_light_falloff = 6.0f;
    options.probe_gi_multiplier = 2.5f;
    options.probe_normal_bias = 0.25f;
    options.probe_view_bias = 1.25f;
    settings::save();
    options = settings::Settings{};
    settings::load(file);
    CHECK(options.enhanced_rendering);
    CHECK(!options.enhanced_graphics);
    CHECK(options.probe_spheres);
    CHECK(options.environment_probe_debug);
    CHECK(!options.enhanced_shadows);
    CHECK(!options.rim_shadow_lift);
    CHECK(options.rim_lift_exponent == 0.09f);
    CHECK(options.rim_lift_max_gain == 8.0f);
    CHECK(!options.rim_light);
    CHECK(options.rim_light_strength == 0.4f);
    CHECK(options.rim_light_falloff == 6.0f);
    CHECK(options.probe_gi_multiplier == 2.5f);
    CHECK(options.probe_normal_bias == 0.25f);
    CHECK(options.probe_view_bias == 1.25f);
    CHECK(options.gtao && std::abs(options.gtao_settings.strength - 0.7f) < 0.0001f);
    options.enhanced_graphics = true;
    settings::save();
    settings::load(file);
    CHECK(options.enhanced_graphics && options.gtao);
    CHECK(!options.enhanced_shadows);
    CHECK(options.probe_gi_multiplier == 2.5f);
    options.enhanced_shadows = true;
    settings::save();
    settings::load(file);
    CHECK(options.enhanced_shadows);
    std::ofstream(file) << "rim_light = bad\nrim_light_strength = nan\n"
                           "rim_light_strength = -1\nrim_light_strength = 2\n"
                           "rim_light_falloff = inf\nrim_light_falloff = 0\n"
                           "rim_light_falloff = 9\nrim_shadow_lift = bad\n"
                           "rim_lift_exponent = nan\nrim_lift_exponent = 0\n"
                           "rim_lift_exponent = 2\nrim_lift_max_gain = inf\n"
                           "rim_lift_max_gain = 0\nrim_lift_max_gain = 17\n";
    settings::load(file);
    CHECK(options.rim_shadow_lift);
    CHECK(options.rim_lift_exponent == 0.09f);
    CHECK(options.rim_lift_max_gain == 7.1f);
    CHECK(options.rim_light);
    CHECK(options.rim_light_strength == 0.30f);
    CHECK(options.rim_light_falloff == 6.46f);
    std::ofstream(file) << "probe_gi_multiplier = nan\nprobe_gi_multiplier = 6\n"
                           "probe_gi_multiplier = -1\n";
    settings::load(file);
    CHECK(options.probe_gi_multiplier == 1.0f);
    for (float multiplier : {0.0f, 5.0f}) {
        options.probe_gi_multiplier = multiplier;
        settings::save();
        settings::load(file);
        CHECK(options.probe_gi_multiplier == multiplier);
    }
    std::ofstream(file) << "probe_normal_bias = nan\nprobe_normal_bias = 3\n"
                           "probe_normal_bias = -1\nprobe_view_bias = inf\n"
                           "probe_view_bias = 3\nprobe_view_bias = -1\n";
    settings::load(file);
    CHECK(options.probe_normal_bias == 0.5f && options.probe_view_bias == 0.5f);
    for (float bias : {0.0f, 2.0f}) {
        options.probe_normal_bias = bias;
        options.probe_view_bias = 2.0f - bias;
        settings::save();
        settings::load(file);
        CHECK(options.probe_normal_bias == bias);
        CHECK(options.probe_view_bias == 2.0f - bias);
    }
    for (auto quality :
         {renderer::GtaoQuality::Quality,
          renderer::GtaoQuality::Low,
          renderer::GtaoQuality::Performance}) {
        options.gtao_settings.quality = quality;
        settings::save();
        settings::load(file);
        CHECK(options.gtao_settings.quality == quality);
    }
    // The depth hierarchy remains independently switchable and survives a save/load cycle.
    for (bool enabled : {false, true}) {
        options.gtao_settings.hi_z = enabled;
        settings::save();
        settings::load(file);
        CHECK(options.gtao_settings.hi_z == enabled);
    }
    std::ofstream(file) << "gtao_quality = quality\n";
    settings::load(file);
    CHECK(options.gtao_settings.quality == renderer::GtaoQuality::Quality);
    std::ofstream(file) << "gtao_quality = unknown\n";
    settings::load(file);
    CHECK(options.gtao_settings.quality == renderer::GtaoQuality::Performance);
    // The on-screen pad: its size and camera speed within the drawer's lists' ends, and where
    // the player moved its controls, kept as written (touch_pad.cpp reads it).
    CHECK(options.touch_scale == 75 && options.touch_camera == 100 && options.touch_layout.empty());
    std::ofstream(file) << "touch_scale = 49\ntouch_camera = 301\n";
    settings::load(file);
    CHECK(options.touch_scale == 75 && options.touch_camera == 100);
    options.touch_scale = 65;  // between two of the list's: a file may hold it
    options.touch_camera = 300;
    options.touch_layout = "dpad 0.0200 -0.0500, face 0.1000 0.0000";
    settings::save();
    options = settings::Settings{};
    settings::load(file);
    CHECK(options.touch_scale == 65 && options.touch_camera == 300);
    CHECK(options.touch_layout == "dpad 0.0200 -0.0500, face 0.1000 0.0000");
    // The drawer's ← and →: to the next of the list, to the nearer one that way from between
    // two, and no further than the ends.
    CHECK(settings::step_listed(settings::kTouchScales, 75, 1) == 80);
    CHECK(settings::step_listed(settings::kTouchScales, 75, -1) == 70);
    CHECK(settings::step_listed(settings::kTouchScales, 65, 1) == 70);
    CHECK(settings::step_listed(settings::kTouchScales, 65, -1) == 60);
    CHECK(settings::step_listed(settings::kTouchScales, 150, 1) == 150);
    CHECK(settings::step_listed(settings::kTouchScales, 50, -1) == 50);
    CHECK(settings::step_listed(settings::kTouchCameraSpeeds, 300, 1) == 300);
    // Only this test's file and newly created empty folder are removed.
    std::filesystem::remove(file);
    std::filesystem::remove(folder);
    return test::result();
}
