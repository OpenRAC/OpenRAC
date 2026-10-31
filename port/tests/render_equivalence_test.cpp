// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// --opt registry-name limits the registry comparisons to one optimisation.
//
// Tests that the render optimisations change no pixel (ctest render_equivalence_vulkan): for
// each scenario of render_fixture.h (the camera walking, the camera still with the mobys moving,
// pictures between ticks, the level uploaded again halfway, mobys entering and leaving at ticks,
// Enhanced on and off) it first draws
// the frames twice with the same settings and retains one verified default run, since every later proof
// rests on the driver drawing alike twice. Then, for each entry of the optimisation registry
// (renderer/render_opt.h), it draws the frames with the entry on and with it off and compares
// them with the entry's tolerance: byte for byte unless the entry says otherwise. The registry's
// self-test entry is compared as well, as the harness's check of itself (its reference path is
// in render_fixture.h). Every scenario is drawn once more with the backend's discarded attachments
// overwritten with garbage (vkgl::test_poison_discards): a pass that read what
// glInvalidateFramebuffer let go would then draw other frames. Under OPENRAC_VULKAN_VALIDATION the validation layer must report nothing.
//
// The frames are compared in memory; nothing is written out. A scene a tenth of the benchmark's,
// at 320 by 180, keeps the run short under the validation layer. Skipped (77) without Vulkan.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/env.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/vulkan/vkgl.h"
#include "tests/check.h"
#include "tests/render_fixture.h"
#include "tests/renderer_gl_context.h"

namespace {

namespace render = openrac::test::render;
namespace render_opt = openrac::renderer::render_opt;

// The picture's size and the frames each scenario draws.
constexpr int kWidth = 320;
constexpr int kHeight = 180;
constexpr std::uint64_t kFrames = 8;

// The frames of one run, each the picture's RGBA bytes.
using Frames = std::vector<std::vector<std::uint8_t>>;

// Returns the scene the test draws: the benchmark's at a tenth of its size.
render::SceneSize test_size() {
    render::SceneSize size;
    size.static_instances = 400;
    size.triangles = 100'000;
    size.materials = 40;
    size.textures = 100;
    size.mobys = 30;
    return size;
}

// Draws `scenario` over `level` (`enhanced` on or off) and returns its frames, read back one by
// one. Empty with `error` when the scene cannot be made.
Frames draw_run(
    const openrac::scene::LevelData& level,
    const render::SceneSize& size,
    const render::Scenario& scenario,
    bool enhanced,
    std::string& error
) {
    render::SceneRenderer scene;
    const render::DrawOptions options{kWidth, kHeight, 1, enhanced};

    // The scene could not be made: nothing to compare.
    if (!scene.open(level, options, error)) {
        return {};
    }

    const render::Walk walk = render::walk_over(level);
    const auto aspect = static_cast<float>(kWidth) / static_cast<float>(kHeight);
    const double ticks = static_cast<double>(kFrames) / static_cast<double>(scenario.between + 1);
    Frames frames;
    const bool check_scroll = enhanced && openrac::env::flag(openrac::env::Var::ScrollingShadows);
    if (check_scroll) {
        openrac::stats::set_enabled(openrac::stats::Client::Dump, true);
    }

    // Each picture of the run, drawn, shown and read back.
    for (std::uint64_t index = 0; index < kFrames; ++index) {
        // Halfway, a level change.
        if (scenario.reupload && index == kFrames / 2 && !scene.reupload(level, error)) {
            return {};
        }

        const render::Picture shown = render::picture(index, scenario.between);
        const double tick = static_cast<double>(shown.tick - 1) + shown.alpha;
        render::MobyFrame mobys = render::mobys_at(level, size, shown);

        // A churn scenario: mobys enter and leave at each tick (render_fixture.h, churned).
        if (scenario.churn) {
            mobys = render::churned(std::move(mobys), shown.tick);
        }

        if (check_scroll) {
            openrac::stats::begin_frame(openrac::stats::FrameKind::Tick, index);
        }
        const auto shadow_quality = index >= 2 && index < 6
                                        ? openrac::renderer::ShadowQuality::Medium
                                        : openrac::renderer::ShadowQuality::High;
        const auto camera = render::camera_at(walk, scenario.camera, tick, ticks, aspect);
        // Another view in the same recording must not lend its visibility to this camera.
        if (index == 4) {
            auto other = camera;
            other.position[0] += 4000.0f;  // Outside the fixture's populated area.
            other.view = openrac::renderer::look_along(other.position, {0, 0, -1}, {0, 1, 0});
            scene.draw(other, mobys, index, shadow_quality);
        }

        scene.draw(camera, mobys, index, shadow_quality, enhanced && index == 3);
        frames.push_back(scene.read_rgba());
        openrac::vkgl::present();
        if (check_scroll) {
            openrac::stats::begin_frame(openrac::stats::FrameKind::Tick, index);
            const auto* measured = openrac::stats::history(0);
            CHECK(measured != nullptr);
            if (measured) {
                const auto active = std::find_if(
                    measured->gauges.begin(), measured->gauges.end(), [](const auto& gauge) {
                        return gauge.first == "shadow scroll active";
                    }
                );
                const auto atlas = std::find_if(
                    measured->gauges.begin(), measured->gauges.end(), [](const auto& gauge) {
                        return gauge.first == "shadow atlas";
                    }
                );
                CHECK(atlas != measured->gauges.end());
                for (const auto& gauge : measured->gauges) {
                    const int divisor =
                        openrac::renderer::shadow_resolution_divisor(shadow_quality);
                    if (gauge.first == "shadow atlas size") {
                        CHECK(gauge.second == 3072 / divisor);
                    } else if (gauge.first == "sun shadow near size") {
                        CHECK(gauge.second == 2048 / divisor);
                    } else if (gauge.first == "hero shadow size") {
                        CHECK(
                            gauge.second
                            == openrac::renderer::hero_shadow_resolution(shadow_quality)
                        );
                    }
                }
                // The separate-target/CPU fallback deliberately bypasses the experiment.
                if (atlas != measured->gauges.end() && atlas->second == 1) {
                    CHECK(active != measured->gauges.end() && active->second == 1);
                    const auto cascades = std::find_if(
                        measured->gauges.begin(), measured->gauges.end(), [](const auto& gauge) {
                            return gauge.first == "shadow scroll active cascades";
                        }
                    );
                    CHECK(cascades != measured->gauges.end() && cascades->second == 4);
                } else {
                    CHECK(active == measured->gauges.end() || active->second == 0);
                }
            }
        }
    }
    if (check_scroll) {
        openrac::stats::set_enabled(openrac::stats::Client::Dump, false);
    }

    return frames;
}

// What two runs' frames differ by: the share of differing pixels in the worst frame, and the
// largest difference of one channel anywhere.
struct Difference {
    double worst_fraction = 0.0;  // Of one frame's pixels, the most that differ.
    int worst_channel = 0;        // The largest difference of a channel, 0 to 255.
    bool same_shape = true;       // False when the runs drew different counts or sizes of frames.
};

// Returns how `a` and `b` differ.
Difference compare(const Frames& a, const Frames& b) {
    Difference difference;
    difference.same_shape = a.size() == b.size();

    // Frame by frame, as far as both go.
    for (std::size_t frame = 0; frame < std::min(a.size(), b.size()); ++frame) {
        const auto& left = a[frame];
        const auto& right = b[frame];

        // A frame of another size cannot be compared pixel by pixel.
        if (left.size() != right.size()) {
            difference.same_shape = false;
            continue;
        }

        std::size_t differing = 0;

        // Pixel by pixel, four channels each.
        for (std::size_t at = 0; at + 4 <= left.size(); at += 4) {
            int worst = 0;

            // The channels of the pixel.
            for (std::size_t c = 0; c < 4; ++c) {
                worst = std::max(worst, std::abs(int{left[at + c]} - int{right[at + c]}));
            }

            differing += worst != 0 ? 1 : 0;
            difference.worst_channel = std::max(difference.worst_channel, worst);
        }

        const double pixels = static_cast<double>(std::max<std::size_t>(left.size() / 4, 1));
        difference.worst_fraction =
            std::max(difference.worst_fraction, static_cast<double>(differing) / pixels);
    }

    return difference;
}

// Returns whether `difference` is within `tolerance`.
bool within(const Difference& difference, const render_opt::Tolerance& tolerance) {
    return difference.same_shape && difference.worst_fraction <= tolerance.max_differing_fraction
           && difference.worst_channel <= tolerance.max_channel_delta;
}

// Returns the share of `frame`'s pixels whose colour is not the clear colour's (render_fixture.h's
// SceneRenderer::draw clears to 0.3, 0.4, 0.6), by more than a step of rounding.
double drawn_share(const std::vector<std::uint8_t>& frame) {
    constexpr std::array<int, 3> kClear = {77, 102, 153};  // 0.3, 0.4 and 0.6 of 255, rounded
    constexpr int kRounding = 2;
    std::size_t drawn = 0;

    // Pixel by pixel.
    for (std::size_t at = 0; at + 4 <= frame.size(); at += 4) {
        bool cleared = true;

        // Its three colour channels against the clear colour's.
        for (std::size_t c = 0; c < 3; ++c) {
            cleared = cleared && std::abs(int{frame[at + c]} - kClear[c]) <= kRounding;
        }

        drawn += cleared ? 0 : 1;
    }

    return static_cast<double>(drawn)
           / static_cast<double>(std::max<std::size_t>(frame.size() / 4, 1));
}

// Tests that the walk's first frame shows the scene: a frame of nothing but the clear colour would
// be the same with any entry on or off, and prove nothing.
void test_the_scene_is_drawn(
    const openrac::scene::LevelData& level, const render::SceneSize& size
) {
    constexpr double kLeast = 0.05;  // of the pixels: the ground alone covers more
    std::string error;
    const Frames frames = draw_run(level, size, render::kScenarios[0], false, error);
    CHECK(!frames.empty());
    const double share = frames.empty() ? 0.0 : drawn_share(frames.front());
    std::printf("walk: %.3f of the first frame's pixels drawn over the clear colour\n", share);
    CHECK(share > kLeast);
}

// Tests that `scenario` draws the same frames twice; reports the noise floor when it does not.
// Returns false when it does not, since no comparison can rest on it then.
bool test_the_driver_draws_alike_twice(
    const openrac::scene::LevelData& level,
    const render::SceneSize& size,
    const render::Scenario& scenario,
    bool enhanced,
    Frames& baseline
) {
    std::string error;
    Frames first = draw_run(level, size, scenario, enhanced, error);
    const Frames second = draw_run(level, size, scenario, enhanced, error);
    CHECK(!first.empty());
    const Difference noise = compare(first, second);
    const bool alike = !first.empty() && within(noise, render_opt::Tolerance{});
    std::printf(
        "%s%s: drawn twice, %s (worst frame %.6f of its pixels, channel by %d)%s%s\n",
        std::string(scenario.name).c_str(),
        enhanced ? " (enhanced)" : "",
        alike ? "identical" : "NOT identical",
        noise.worst_fraction,
        noise.worst_channel,
        error.empty() ? "" : ": ",
        error.c_str()
    );
    CHECK(alike);
    baseline = std::move(first);
    return alike;
}

// Tests that `entry` changes no frame of `scenario` beyond its tolerance, on against off.
void test_an_entry_changes_no_frame(
    const openrac::scene::LevelData& level,
    const render::SceneSize& size,
    const render::Scenario& scenario,
    const Frames& baseline,
    const render_opt::Entry& entry
) {
    std::string error;
    const bool enhanced = openrac::gl::enhanced_rendering_available();
    const bool baseline_on = render_opt::enabled(entry.opt);
    render_opt::set(entry.opt, !baseline_on);
    const Frames changed = draw_run(level, size, scenario, enhanced, error);
    render_opt::reset();
    // Every comparison keeps both paths; unchanged default frames were already checked twice.
    const Frames& on = baseline_on ? baseline : changed;
    const Frames& off = baseline_on ? changed : baseline;
    const Difference difference = compare(on, off);
    const bool equivalent = !on.empty() && within(difference, entry.tolerance);
    std::printf(
        "%s%s: %s on against off, %s (worst frame %.6f of its pixels, channel by %d)\n",
        std::string(scenario.name).c_str(),
        enhanced ? " (enhanced)" : "",
        std::string(entry.name).c_str(),
        equivalent ? "within its tolerance" : "DIFFERENT",
        difference.worst_fraction,
        difference.worst_channel
    );
    CHECK(equivalent);
}

// Tests that no pass of `scenario` reads what glInvalidateFramebuffer let go: with the backend's
// poison on (vkgl::test_poison_discards), every discarded attachment holds garbage instead of
// being left undefined, so a pass that reads one draws other frames than attachment-discard off.
void test_no_pass_reads_what_it_discards(
    const openrac::scene::LevelData& level,
    const render::SceneSize& size,
    const render::Scenario& scenario,
    bool enhanced
) {
    std::string error;
    CHECK(openrac::vkgl::test_poison_discards(true));
    const Frames poisoned = draw_run(level, size, scenario, enhanced, error);
    openrac::vkgl::test_poison_discards(false);
    render_opt::set(render_opt::Opt::AttachmentDiscard, false);
    const Frames kept = draw_run(level, size, scenario, enhanced, error);
    render_opt::reset();
    const Difference difference = compare(poisoned, kept);
    const bool equivalent = !poisoned.empty() && within(difference, render_opt::Tolerance{});
    std::printf(
        "%s%s: discarded attachments poisoned against kept, %s (worst frame %.6f of its pixels, "
        "channel by %d)\n",
        std::string(scenario.name).c_str(),
        enhanced ? " (enhanced)" : "",
        equivalent ? "identical" : "DIFFERENT",
        difference.worst_fraction,
        difference.worst_channel
    );
    CHECK(equivalent);
}

// Chooses Enhanced rendering on or off for the runs that follow. A run makes its scene and its
// renderers afresh, so their shaders are built for the choice (renderer/gl.h asks for it before
// the first shader is made, which holds for each run).
void use_enhanced(bool on) {
    openrac::gl::set_enhanced_rendering(on);
}

}  // namespace

int main(int argc, char** argv) {
    const render_opt::Entry* selected = nullptr;
    if (argc != 1) {
        if (argc != 3 || std::string_view(argv[1]) != "--opt"
            || !(selected = render_opt::find(argv[2]))) {
            std::fprintf(stderr, "usage: render_equivalence_test [--opt registry-name]\n");
            return 1;
        }
    }
    openrac::test::GlContext context;
    std::string error;

    // The registry's optimisations are the Vulkan renderer's; nothing to compare without it.
    if (!context.open(error, kWidth, kHeight) || !openrac::vkgl::active()) {
        std::printf("SKIP: Vulkan unavailable: %s\n", error.c_str());
        return openrac::test::kSkip;
    }

    const render::SceneSize size = test_size();
    const openrac::scene::LevelData level = render::make_level(size);
    std::vector<bool> modes{false};
    openrac::gl::set_enhanced_rendering(true);

    // Enhanced as well where this build and its renderer have it.
    if (openrac::gl::enhanced_rendering_available()) {
        modes.push_back(true);
    }

    use_enhanced(false);
    test_the_scene_is_drawn(level, size);
    bool deterministic = true;
    std::array<std::array<Frames, render::kScenarios.size()>, 2> baselines;

    // Determinism first, every scenario in each mode: the comparisons below rest on it.
    for (const bool enhanced : modes) {
        use_enhanced(enhanced);

        // Each scenario of the fixture.
        for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
            deterministic = test_the_driver_draws_alike_twice(
                                level,
                                size,
                                render::kScenarios[index],
                                enhanced,
                                baselines[enhanced ? 1 : 0][index]
                            )
                            && deterministic;
        }
    }

    // A driver that does not draw alike twice: the noise floor is printed above, and no entry can
    // be judged against it.
    if (!deterministic) {
        return openrac::test::result();
    }

    // Each entry, the self-test included, on against off in every scenario.
    for (const bool enhanced : modes) {
        use_enhanced(enhanced);

        // Each scenario of the fixture.
        for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
            const auto& scenario = render::kScenarios[index];
            const auto& baseline = baselines[enhanced ? 1 : 0][index];
            // Each entry of the registry.
            for (const render_opt::Entry& entry : render_opt::entries()) {
                if (selected && entry.opt != selected->opt) {
                    continue;
                }
                test_an_entry_changes_no_frame(level, size, scenario, baseline, entry);
            }

            test_no_pass_reads_what_it_discards(level, size, scenario, enhanced);
        }
    }

    // The validation layer, where it ran, saw nothing wrong.
    if (openrac::env::value(openrac::env::Var::VulkanValidation)) {
        CHECK(openrac::vkgl::submission_counters().validation_errors == 0);
    }

    return openrac::test::result();
}
