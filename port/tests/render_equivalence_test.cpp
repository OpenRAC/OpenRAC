// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// --opt registry-name limits the registry comparisons to one optimisation.
// --shard index count balances registry entries by measured cost; all scenarios stay in each shard.
//
// Compares scene-owned render optimisations against verified defaults in seven scenarios,
// enhanced on/off. A CTest fixture checks fresh-scene determinism, synchronous/batched captures
// and poisoned discards once, then shares checked ephemeral snapshots with both registry shards.
// Scrolling shadows share only compatibility snapshots and keep independent enhanced oracles.
// Feature-owned optimisations are checked by required output/counter fixtures named in
// render_equivalence_policy.h. No retail data or persistent reference images are used.
// Comparisons are exact unless an optimisation's registry entry explicitly permits a tolerance.

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
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
#include "tests/render_baseline_file.h"
#include "tests/render_equivalence_costs.h"
#include "tests/render_equivalence_inputs.h"
#include "tests/render_equivalence_policy.h"
#include "tests/render_fixture.h"
#include "tests/render_frame_batch.h"
#include "tests/render_test_shards.h"
#include "tests/render_test_timing.h"
#include "tests/renderer_gl_context.h"

namespace {

namespace render = openrac::test::render;
namespace render_opt = openrac::renderer::render_opt;

// The picture's size and the frames each scenario draws.
constexpr int kWidth = 80;
constexpr int kHeight = 45;
constexpr std::uint64_t kFrames = 8;

// The frames of one run, each the picture's RGBA bytes.
using Frames = std::vector<std::vector<std::uint8_t>>;

using openrac::test::FixtureInputs;

// Owns no resources; an optional renderer is borrowed for one fixed shader configuration.
struct RunSetup {
    const openrac::scene::LevelData& level;     // Fixture data owned by the test driver.
    const render::SceneSize& size;              // Parameters for deterministic moby movement.
    bool enhanced;                              // Mode used to build shaders and draw frames.
    render::SceneRenderer* reusable = nullptr;  // Borrowed only within one fixed option state.
    bool batched = false;  // Snapshot ordinary frames without a host wait per frame.
    openrac::test::RenderFrameBatch* snapshots = nullptr;  // Storage reused after readback.
    const FixtureInputs* inputs = nullptr;  // Immutable CPU inputs owned by the selection.
};

// Keep multiple textured, cutout and blended meshes and skinned classes in the pixel matrix.
// The benchmark smoke test covers the larger instance and upload workload.
render::SceneSize test_size() {
    render::SceneSize size;
    size.static_instances = 120;
    size.triangles = 30'000;
    size.materials = 30;
    size.textures = 60;
    size.mobys = 18;
    return size;
}

// Draws a scenario and returns synchronous or batched frames; empty if scene setup fails.
Frames draw_run(const RunSetup& setup, const render::Scenario& scenario, std::string& error) {
    const auto& level = setup.level;
    const auto& size = setup.size;
    const bool enhanced = setup.enhanced;
    std::optional<render::SceneRenderer> fresh;

    // Baselines use fresh scenes; registry runs can share setup across their scenarios.
    if (!setup.reusable) {
        fresh.emplace();
        if (!fresh->open(level, {kWidth, kHeight, 1, enhanced}, error)) {
            return {};
        }
    }
    auto& scene = setup.reusable ? *setup.reusable : *fresh;

    openrac::test::RenderFrameBatch local_batch;
    auto& batch = setup.snapshots ? *setup.snapshots : local_batch;
    if (setup.batched && !batch.create(kWidth, kHeight, static_cast<int>(kFrames), error)) {
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
        const auto scenario_index = static_cast<std::size_t>(
            render::find_scenario(scenario.name) - render::kScenarios.data()
        );
        const openrac::test::FixtureFrame* prepared =
            setup.inputs ? &(*setup.inputs)[scenario_index][index] : nullptr;
        render::MobyFrame generated;
        if (!prepared) {
            generated = render::mobys_at(level, size, shown);
        }

        // A churn scenario: mobys enter and leave at each tick (render_fixture.h, churned).
        if (!prepared && scenario.churn) {
            generated = render::churned(std::move(generated), shown.tick);
        }
        const auto& mobys = prepared ? prepared->mobys : generated;

        if (check_scroll) {
            openrac::stats::begin_frame(openrac::stats::FrameKind::Tick, index);
        }
        // The scrolling fixture checks full-size maps and both resize directions.
        using openrac::renderer::ShadowQuality;
        const auto shadow_quality = check_scroll && (index == 0 || index == kFrames - 1)
                                        ? ShadowQuality::High
                                        : ShadowQuality::Medium;
        const auto camera = prepared
                                ? prepared->camera
                                : render::camera_at(walk, scenario.camera, tick, ticks, aspect);
        // Another view in the same recording must not lend its visibility to this camera.
        if (index == 4) {
            auto other = camera;
            other.position[0] += 4000.0f;  // Outside the fixture's populated area.
            other.view = openrac::renderer::look_along(other.position, {0, 0, -1}, {0, 1, 0});
            scene.draw(other, mobys, index, shadow_quality);
        }

        scene.draw(camera, mobys, index, shadow_quality, enhanced && index == 3);
        if (setup.batched) {
            CHECK(batch.capture(scene.framebuffer(), static_cast<int>(index)));
        } else {
            frames.push_back(scene.read_rgba());
        }
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

    return setup.batched ? batch.read() : frames;
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

        // Most frames are identical. Vector equality uses the library's bulk
        // byte comparison; only mismatches need per-pixel tolerance diagnostics.
        if (left == right) {
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
    const auto before = openrac::vkgl::submission_counters();
    const Frames frames = draw_run({level, size, false}, render::kScenarios[0], error);
    const auto after = openrac::vkgl::submission_counters();
    CHECK(!frames.empty());
    const double share = frames.empty() ? 0.0 : drawn_share(frames.front());
    std::printf("walk: %.3f of the first frame's pixels drawn over the clear colour\n", share);
    CHECK(share > kLeast);
    CHECK(after.texture_upload_bytes > before.texture_upload_bytes);
    if (openrac::vkgl::gpu_driven_active()) {
        CHECK(after.gpu_payload_bytes > before.gpu_payload_bytes);
    }
}

// Tests that `scenario` draws the same frames twice; reports the noise floor when it does not.
// Returns false when it does not, since no comparison can rest on it then.
bool test_the_driver_draws_alike_twice(
    const RunSetup& setup, const render::Scenario& scenario, Frames& baseline, bool fresh_programs
) {
    std::string error;
    // The first scenario in each mode creates fresh native objects. Later scenarios
    // keep independent scenes while borrowing only matching immutable programs.
    openrac::vkgl::test_reuse_programs(true, !fresh_programs);
    const auto before = openrac::vkgl::submission_counters().test_program_reuses;
    Frames first = draw_run(setup, scenario, error);
    if (fresh_programs) {
        CHECK(openrac::vkgl::submission_counters().test_program_reuses == before);
    }
    openrac::vkgl::test_reuse_programs(true);
    auto batched = setup;
    batched.batched = true;
    const Frames second = draw_run(batched, scenario, error);
    CHECK(!first.empty());
    const Difference noise = compare(first, second);
    const bool alike = !first.empty() && within(noise, render_opt::Tolerance{});
    std::printf(
        "%s%s: drawn twice, %s (worst frame %.6f of its pixels, channel by %d)%s%s\n",
        std::string(scenario.name).c_str(),
        setup.enhanced ? " (enhanced)" : "",
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

// Reuses identical initialisation across scenarios, comparing each against fresh-scene defaults.
void test_an_entry_changes_no_frame(
    const RunSetup& setup,
    const std::array<Frames, render::kScenarios.size()>& baselines,
    const render_opt::Entry& entry
) {
    openrac::test::RenderTestTiming timing(entry.name);
    std::string error;
    const bool baseline_on = render_opt::enabled(entry.opt);
    render_opt::set(entry.opt, !baseline_on);
    const auto before = openrac::vkgl::submission_counters();
    render::SceneRenderer scene;
    const bool opened = scene.open(setup.level, {kWidth, kHeight, 1, setup.enhanced}, error);
    CHECK(opened);
    if (!opened) {
        std::fprintf(stderr, "%s\n", error.c_str());
        render_opt::reset();
        return;
    }
    openrac::test::RenderFrameBatch snapshots;
    const RunSetup reused{
        setup.level, setup.size, setup.enhanced, &scene, true, &snapshots, setup.inputs
    };
    timing.lap(setup.enhanced ? "enhanced setup" : "compatibility setup");
    for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
        const auto& scenario = render::kScenarios[index];
        const auto& baseline = baselines[index];
        const Frames changed = draw_run(reused, scenario, error);
        // Every comparison keeps both paths; unchanged default frames were already checked twice.
        const Frames& on = baseline_on ? baseline : changed;
        const Frames& off = baseline_on ? changed : baseline;
        const Difference difference = compare(on, off);
        const bool equivalent = !on.empty() && within(difference, entry.tolerance);
        std::printf(
            "%s%s: %s on against off, %s (worst frame %.6f of its pixels, channel by %d)\n",
            std::string(scenario.name).c_str(),
            setup.enhanced ? " (enhanced)" : "",
            std::string(entry.name).c_str(),
            equivalent ? "within its tolerance" : "DIFFERENT",
            difference.worst_fraction,
            difference.worst_channel
        );
        CHECK(equivalent);
    }
    timing.lap(setup.enhanced ? "enhanced draw/readback" : "compatibility draw/readback");
    // Even a borrowed driver program must compile fresh SPIR-V with the disk cache off.
    if (entry.opt == render_opt::Opt::SpirvDiskCache && baseline_on) {
        CHECK(openrac::vkgl::submission_counters().shader_compiles > before.shader_compiles);
    }
    scene.release();
    render_opt::reset();
}

// Draws every scenario with one fixed configuration, preserving the explicit reupload case.
std::vector<Frames> draw_scenarios(const RunSetup& setup, std::string& error) {
    render::SceneRenderer scene;
    if (!scene.open(setup.level, {kWidth, kHeight, 1, setup.enhanced}, error)) {
        return {};
    }
    openrac::test::RenderFrameBatch snapshots;
    const RunSetup reused{
        setup.level, setup.size, setup.enhanced, &scene, true, &snapshots, setup.inputs
    };
    std::vector<Frames> frames;
    frames.reserve(render::kScenarios.size());
    for (const auto& scenario : render::kScenarios) {
        frames.push_back(draw_run(reused, scenario, error));
    }
    return frames;
}

// Compares poisoned discards with verified default pixels in every scenario and mode.
void test_no_pass_reads_what_it_discards(
    const RunSetup& setup, const std::array<Frames, render::kScenarios.size()>& baselines
) {
    openrac::test::RenderTestTiming timing("discard comparisons");
    std::string error;
    CHECK(openrac::vkgl::test_poison_discards(true));
    openrac::vkgl::test_reuse_programs(true, false);
    const auto poisoned_frames = draw_scenarios(setup, error);
    openrac::vkgl::test_poison_discards(false);
    timing.lap(setup.enhanced ? "enhanced draw/readback" : "compatibility draw/readback");
    render_opt::reset();
    openrac::vkgl::test_reuse_programs(false);
    CHECK(poisoned_frames.size() == render::kScenarios.size());
    if (poisoned_frames.size() != render::kScenarios.size()) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
        const auto& scenario = render::kScenarios[index];
        const auto& poisoned = poisoned_frames[index];
        const auto& kept = baselines[index];
        const Difference difference = compare(poisoned, kept);
        const bool equivalent = !poisoned.empty() && within(difference, render_opt::Tolerance{});
        std::printf(
            "%s%s: discarded attachments poisoned against verified defaults, %s (worst frame %.6f "
            "of its "
            "pixels, "
            "channel by %d)\n",
            std::string(scenario.name).c_str(),
            setup.enhanced ? " (enhanced)" : "",
            equivalent ? "identical" : "DIFFERENT",
            difference.worst_fraction,
            difference.worst_channel
        );
        CHECK(equivalent);
    }
}

// Selects the mode before each baseline or fixed-configuration renderer builds its shaders.
void use_enhanced(bool on) {
    openrac::gl::set_enhanced_rendering(on);
}

struct Selection {
    const render_opt::Entry* entry = nullptr;
    std::size_t shard = 0, shards = 1;
    std::filesystem::path baselines;  // Ephemeral fixture output consumed only by this build.
    bool compatibility_baselines = false;  // Only compatibility pixels ignore scrolling shadows.
    bool prepare = false;  // Verifies defaults and discarded attachments once for all shards.
};

// Both modes keep all scenario snapshots; an unavailable enhanced mode remains empty.
using Baselines = std::array<std::array<Frames, render::kScenarios.size()>, 2>;

// Binds ephemeral pixels to this executable, driver, fixture and initial optimisation settings.
std::string baseline_identity(std::uint64_t executable, bool compatibility_only = false) {
    std::string identity = std::format(
        "equivalence-v2/{}/{}/{}/{}/{}/{}/{}/{}",
        executable,
        openrac::vkgl::device_name(),
        openrac::vkgl::driver_version(),
        openrac::vkgl::api_version(),
        kWidth,
        kHeight,
        kFrames,
        !compatibility_only && openrac::env::flag(openrac::env::Var::ScrollingShadows)
    );
    // This separate file contains no enhanced snapshots or scrolling-shadow reference pixels.
    if (compatibility_only) {
        identity += "/compatibility-only";
    }
    identity += openrac::vkgl::tile_based() ? "/tiled" : "/immediate";
    for (const auto& entry : render_opt::entries()) {
        identity += std::format("/{}/{}", entry.name, render_opt::enabled(entry.opt));
    }
    for (const auto& variable : openrac::env::all()) {
        // Each process deliberately owns a different pipeline cache; rendering overrides must match.
        if (variable.var == openrac::env::Var::VulkanPipelineCache) {
            continue;
        }
        // Compatibility rendering cannot use enhanced scrolling shadows.
        if (compatibility_only && variable.var == openrac::env::Var::ScrollingShadows) {
            continue;
        }
        const auto value = openrac::env::value(variable.var);
        const auto text = value.value_or("");
        const auto bytes =
            std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
        identity += std::format(
            "/{}/{}/{}", variable.name, bool(value), openrac::test::baseline_checksum(bytes)
        );
    }
    return identity;
}

// Flattens only available modes in the deterministic mode/scenario/frame order.
std::vector<std::uint8_t> flatten(const Baselines& baselines, const std::vector<bool>& modes) {
    std::vector<std::uint8_t> bytes;
    for (bool enhanced : modes) {
        for (const auto& frames : baselines[enhanced ? 1 : 0]) {
            for (const auto& frame : frames) {
                bytes.insert(bytes.end(), frame.begin(), frame.end());
            }
        }
    }
    return bytes;
}

// Loads exactly the expected shape, rejecting another build/device/configuration before rendering.
bool load_baselines(
    const std::filesystem::path& path,
    const std::vector<bool>& modes,
    Baselines& out,
    std::uint64_t executable,
    bool compatibility_only = false
) {
    constexpr std::size_t kFrameBytes =
        static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4;
    const auto expected = modes.size() * render::kScenarios.size() * kFrames * kFrameBytes;
    const auto bytes = openrac::test::read_render_baseline(
        path, baseline_identity(executable, compatibility_only), expected
    );
    if (bytes.size() != expected) {
        return false;
    }
    auto at = bytes.begin();
    for (bool enhanced : modes) {
        for (auto& frames : out[enhanced ? 1 : 0]) {
            for (std::uint64_t frame = 0; frame < kFrames; ++frame) {
                frames.emplace_back(at, at + kFrameBytes);
                at += kFrameBytes;
            }
        }
    }
    return true;
}

// Verifies defaults with fresh renderers, synchronous/batched capture and poisoned attachments.
void prepare_baselines(
    const RunSetup& setup, const std::vector<bool>& modes, Baselines& baselines, bool witness = true
) {
    openrac::test::RenderTestTiming timing("equivalence baselines");
    // The fixture already checked the scene witness when lending compatibility snapshots.
    if (witness) {
        use_enhanced(false);
        test_the_scene_is_drawn(setup.level, setup.size);
        timing.lap("scene witness");
    }
    for (bool enhanced : modes) {
        use_enhanced(enhanced);
        for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
            test_the_driver_draws_alike_twice(
                {setup.level, setup.size, enhanced, nullptr, false, nullptr, setup.inputs},
                render::kScenarios[index],
                baselines[enhanced ? 1 : 0][index],
                index == 0
            );
        }
        timing.lap(enhanced ? "enhanced determinism" : "compatibility determinism");
        test_no_pass_reads_what_it_discards(
            {setup.level, setup.size, enhanced, nullptr, false, nullptr, setup.inputs},
            baselines[enhanced ? 1 : 0]
        );
    }
    openrac::vkgl::test_reuse_programs(false);
    CHECK(openrac::vkgl::submission_counters().validation_errors == 0);
}

// Balances only scene-owned work; focused comparisons already ran as required fixtures.
auto world_shard_assignments(std::size_t count) {
    auto costs = openrac::test::equivalence_entry_costs();
    for (const auto& entry : render_opt::entries()) {
        // Focused owners do not contribute world rendering cost to either shard.
        if (!openrac::test::focused_equivalence_test(entry.opt).empty()) {
            costs[static_cast<std::size_t>(entry.opt)] = 0;
        }
    }
    return openrac::test::balanced_test_shards(costs, count);
}

// Runs every scene-owned entry in its assigned shard while preserving feature-owner reporting.
void compare_registry(
    const Selection& chosen,
    const RunSetup& setup,
    const std::vector<bool>& modes,
    const Baselines& baselines
) {
    const auto entries = render_opt::entries();
    const auto assignments = world_shard_assignments(chosen.shards);

    // Each entry, the self-test included, on against off in every scenario.
    for (const bool enhanced : modes) {
        use_enhanced(enhanced);

        // Each entry keeps its own initialisation and shader configuration.
        for (std::size_t entry_index = 0; entry_index < entries.size(); ++entry_index) {
            const auto& entry = entries[entry_index];
            if ((chosen.entry && entry.opt != chosen.entry->opt)
                || assignments[entry_index] != chosen.shard) {
                continue;
            }
            const auto owner = openrac::test::focused_equivalence_test(entry.opt);
            // Feature-specific fixtures compare actual outputs and assert their path was exercised.
            if (!owner.empty()
                && !(chosen.entry && entry.opt == render_opt::Opt::ShadowScrollReuse)) {
                std::printf(
                    "%s: feature fixture %.*s (outside the generic scene)\n",
                    std::string(entry.name).c_str(),
                    static_cast<int>(owner.size()),
                    owner.data()
                );
                continue;
            }
            // Cache and lifecycle switches retain their original fresh-program behaviour.
            openrac::vkgl::test_reuse_programs(openrac::test::may_reuse_test_programs(entry.opt));
            test_an_entry_changes_no_frame(
                {setup.level, setup.size, enhanced, nullptr, false, nullptr, setup.inputs},
                baselines[enhanced ? 1 : 0],
                entry
            );
        }
    }
}

// Returns the requested registry subset, or nothing for invalid arguments.
std::optional<Selection> selection(int argc, char** argv) {
    Selection chosen;
    if (argc == 1) {
        return chosen;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--prepare") {
        chosen.prepare = true;
        chosen.baselines = argv[2];
        return chosen;
    }
    if (argc == 6 && std::string_view(argv[1]) == "--baseline") {
        chosen.baselines = argv[2];
        argv += 2;
        argc -= 2;
    }
    // Scrolling shadows retain their own enhanced oracles; only compatibility setup is shared.
    if (argc == 5 && std::string_view(argv[1]) == "--compatibility-baseline") {
        chosen.baselines = argv[2];
        chosen.compatibility_baselines = true;
        argv += 2;
        argc -= 2;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--opt") {
        chosen.entry = render_opt::find(argv[2]);
        if (chosen.entry) {
            return chosen;
        }
    } else if (argc == 4 && std::string_view(argv[1]) == "--shard") {
        const auto integer = [](std::string_view text, std::size_t& value) {
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            return result.ec == std::errc{} && result.ptr == text.data() + text.size();
        };
        if (integer(argv[2], chosen.shard) && integer(argv[3], chosen.shards) && chosen.shards > 0
            && chosen.shards <= render_opt::entries().size() && chosen.shard < chosen.shards) {
            return chosen;
        }
    }
    return std::nullopt;
}

// Obtains verified snapshots locally or from the setup fixture; no failed producer is reusable.
bool acquire_baselines(
    const Selection& chosen,
    const RunSetup& setup,
    const std::vector<bool>& modes,
    std::uint64_t executable,
    Baselines& baselines
) {
    if (chosen.compatibility_baselines) {
        // Reject stale, corrupt or incompatible fixture output before any comparison.
        if (!load_baselines(chosen.baselines, {false}, baselines, executable, true)) {
            return false;
        }
        // Every enhanced determinism, batched capture and discard check still runs with scrolling.
        if (std::find(modes.begin(), modes.end(), true) != modes.end()) {
            prepare_baselines(setup, {true}, baselines, false);
        }
        return openrac::test::failures() == 0;
    }
    if (chosen.prepare || chosen.baselines.empty()) {
        prepare_baselines(setup, modes, baselines);
        // A failed comparison must not publish a new shared reference.
        if (openrac::test::failures() != 0) {
            return false;
        }
        if (chosen.prepare) {
            const bool full = openrac::test::write_render_baseline(
                chosen.baselines, baseline_identity(executable), flatten(baselines, modes)
            );
            return full
                   && openrac::test::write_render_baseline(
                       chosen.baselines.string() + ".compatibility",
                       baseline_identity(executable, true),
                       flatten(baselines, {false})
                   );
        }
        return true;
    }
    if (!load_baselines(chosen.baselines, modes, baselines, executable)) {
        std::fprintf(
            stderr, "missing, stale or corrupt equivalence baselines; run the CTest setup fixture\n"
        );
        return false;
    }
    return true;
}

// Executes one request with a single current Vulkan context and checks validation before closing it.
int run_selection(const Selection& chosen, const char* executable_path) {
    openrac::test::GlContext context;
    std::string error;

    // The registry's optimisations are the Vulkan renderer's; nothing to compare without it.
    if (!context.open(error, kWidth, kHeight) || !openrac::vkgl::active()) {
        std::printf("SKIP: Vulkan unavailable: %s\n", error.c_str());
        return openrac::test::kSkip;
    }

    const render::SceneSize size = test_size();
    const openrac::scene::LevelData level = render::make_level(size);
    const auto fixture_inputs =
        openrac::test::prepare_inputs(level, size, kWidth, kHeight, kFrames);
    const RunSetup setup{level, size, false, nullptr, false, nullptr, &fixture_inputs};
    std::vector<bool> modes{false};
    openrac::gl::set_enhanced_rendering(true);

    // Enhanced as well where this build and its renderer have it.
    if (openrac::gl::enhanced_rendering_available()) {
        modes.push_back(true);
    }

    const auto executable = chosen.baselines.empty()
                                ? std::optional<std::uint64_t>(0)
                                : openrac::test::baseline_executable_checksum(executable_path);
    if (!executable) {
        std::fprintf(stderr, "cannot identify the equivalence executable for shared baselines\n");
        return 1;
    }
    Baselines baselines;
    if (!acquire_baselines(chosen, setup, modes, *executable, baselines)) {
        std::fprintf(stderr, "cannot obtain verified equivalence baselines\n");
        return 1;
    }
    if (chosen.prepare) {
        return openrac::test::result();
    }

    compare_registry(chosen, setup, modes, baselines);

    // The validation layer, where it ran, saw nothing wrong.
    if (openrac::env::value(openrac::env::Var::VulkanValidation)) {
        CHECK(openrac::vkgl::submission_counters().validation_errors == 0);
    }

    return openrac::test::result();
}

// The most pixels of a frame that differ from OpenGL's by more than `channel`.
std::size_t worst_pixels_beyond(const Frames& a, const Frames& b, int channel) {
    std::size_t worst = 0;
    for (std::size_t frame = 0; frame < std::min(a.size(), b.size()); ++frame) {
        std::size_t beyond = 0;
        for (std::size_t at = 0; at + 4 <= std::min(a[frame].size(), b[frame].size()); at += 4) {
            bool over = false;
            for (std::size_t c = 0; c < 4; ++c) {
                over |= std::abs(int{a[frame][at + c]} - int{b[frame][at + c]}) > channel;
            }
            beyond += over ? 1 : 0;
        }
        worst = std::max(worst, beyond);
    }
    return worst;
}

// --opengl-frames FILE writes the compatibility scenarios' frames as OpenGL draws them;
// --against-opengl FILE draws them on this process's API (Metal) and requires the same bytes. Two
// processes, since a process opens one API.
int run_against_opengl(const std::filesystem::path& path, bool save, const char* executable_path) {
    openrac::test::GlContext context;
    std::string error;
    if (!context.open(error, kWidth, kHeight)) {
        std::printf("SKIP: no window: %s\n", error.c_str());
        return openrac::test::kSkip;
    }
    const auto executable = openrac::test::baseline_executable_checksum(executable_path);
    if (!executable) {
        std::fprintf(stderr, "cannot identify the equivalence executable\n");
        return 1;
    }
    const std::string identity =
        std::format("opengl-frames/{}/{}/{}/{}", *executable, kWidth, kHeight, kFrames);
    const render::SceneSize size = test_size();
    const openrac::scene::LevelData level = render::make_level(size);
    use_enhanced(false);

    Baselines drawn;
    for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
        drawn[0][index] = draw_run({level, size, false}, render::kScenarios[index], error);
        CHECK(drawn[0][index].size() == kFrames);
    }
    // A frame of the clear colour alone would match any API's.
    CHECK(!drawn[0][0].empty() && drawn_share(drawn[0][0].front()) > 0.05);
    if (save) {
        CHECK(openrac::test::write_render_baseline(path, identity, flatten(drawn, {false})));
        return openrac::test::result();
    }
    // Where OpenGL drew nothing (no window for it), there is nothing to compare with.
    if (!std::filesystem::exists(path)) {
        std::printf("SKIP: no OpenGL frames\n");
        return openrac::test::kSkip;
    }
    const auto expected = flatten(drawn, {false});
    const auto opengl = openrac::test::read_render_baseline(path, identity, expected.size());
    if (opengl.empty()) {
        std::fprintf(stderr, "missing or stale OpenGL frames; run render_equivalence_opengl\n");
        return 1;
    }
    constexpr std::size_t kFrameBytes = std::size_t{kWidth} * kHeight * 4;
    // Apple's OpenGL runs on Metal, so the two agree exactly. AMD's OpenGL (R9 M370X) filters
    // textures in eighths of a level, moving channels by up to 5, and resolves a triangle edge
    // through a pixel centre otherwise in up to 2 pixels a frame (tools/remote-mac.sh measures it).
    const std::string_view renderer =
        reinterpret_cast<const char*>(openrac::gl::glGetString(openrac::gl::GL_RENDERER));
    const bool apple = renderer.starts_with("Apple");
    for (std::size_t index = 0; index < render::kScenarios.size(); ++index) {
        Frames theirs;
        for (std::size_t frame = 0; frame < kFrames; ++frame) {
            const auto at = opengl.begin()
                            + static_cast<std::ptrdiff_t>((index * kFrames + frame) * kFrameBytes);
            theirs.emplace_back(at, at + static_cast<std::ptrdiff_t>(kFrameBytes));
        }
        const Difference difference = compare(drawn[0][index], theirs);
        std::printf(
            "%s: %.4f of a frame's pixels differ from OpenGL's, by up to %d\n",
            std::string(render::kScenarios[index].name).c_str(),
            difference.worst_fraction,
            difference.worst_channel
        );
        CHECK(difference.same_shape);
        CHECK(
            apple ? difference.worst_channel == 0
                  : worst_pixels_beyond(drawn[0][index], theirs, 5) <= 2
        );
    }
    return openrac::test::result();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3
        && (std::string_view(argv[1]) == "--opengl-frames"
            || std::string_view(argv[1]) == "--against-opengl")) {
        return run_against_opengl(argv[2], std::string_view(argv[1]) == "--opengl-frames", argv[0]);
    }
    const auto chosen = selection(argc, argv);
    if (!chosen) {
        std::fprintf(
            stderr,
            "usage: render_equivalence_test [--prepare file | [--baseline file] --shard index "
            "count | [--compatibility-baseline file] --opt registry-name]\n"
        );
        return 1;
    }
    if (chosen->entry) {
        const auto owner = openrac::test::focused_equivalence_test(chosen->entry->opt);
        if (chosen->entry->opt == render_opt::Opt::ShadowScrollReuse
            && !openrac::env::flag(openrac::env::Var::ScrollingShadows)) {
            std::fprintf(stderr, "shadow-scroll-reuse requires OPENRAC_SCROLLING_SHADOWS=1\n");
            return 1;
        }
        if (!owner.empty() && chosen->entry->opt != render_opt::Opt::ShadowScrollReuse) {
            std::fprintf(
                stderr,
                "%s requires its feature fixture: ctest -R '^%.*s$'\n",
                std::string(chosen->entry->name).c_str(),
                static_cast<int>(owner.size()),
                owner.data()
            );
            return 1;
        }
    }
    if (chosen->prepare) {
        std::error_code error;
        std::filesystem::remove(chosen->baselines, error);
        std::filesystem::remove(chosen->baselines.string() + ".compatibility", error);
    }
    return run_selection(*chosen, argv[0]);
}
