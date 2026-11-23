// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "scene/gold_case_preview.h"
#include "scene/level.h"
#include "scene/pickup_preview.h"
#include "scene/preview_transform.h"
#include "scene/vendor_preview.h"

int main(int argc, char** argv) {
    using namespace openrac;
    int failures = 0;
    const auto check = [&](bool ok, const char* what) {
        if (!ok) {
            std::printf("FAIL: %s\n", what);
            ++failures;
        }
    };
    {
        namespace preview = scene::vendor_preview;
        check(
            preview::model_class(preview::other_game_visual) == 1134
                && preview::key(preview::other_game_visual, false) != preview::key(1134, false)
                && preview::key(preview::other_game_visual, true) != preview::key(1134, true)
                && preview::defaults(preview::other_game_visual).size
                       != preview::defaults(1134).size,
            "other-game vendor reward shares the Bolt mesh with independent framing profiles"
        );
        for (float angle : {0.0f, 0.5f, 1.5f, 3.0f}) {
            preview::Adjustment native;
            native.pitch = 30;
            native.roll = angle * 180.0f / 3.14159265359f;
            auto source = preview::orient(renderer::identity(), native, {0, 0, 0}, false);
            preview::Adjustment upright;
            upright.yaw = -90;
            for (int id : {645, 1433}) {
                const auto result = preview::orient(
                    preview::can_hologram_basis(source, id), upright, {0, 0, 0}, false
                );
                check(
                    std::abs(result[0]) < .00001f && std::abs(result[1]) < .00001f
                        && std::abs(result[2] - 1) < .00001f,
                    "Nanotech can remains upright through hologram spin"
                );
            }
            check(
                preview::can_hologram_basis(source, 168) == source,
                "retail weapon hologram basis is unchanged"
            );
        }
    }
    {
        namespace cases = scene::gold_case_preview;
        check(
            std::abs(cases::defaults(157).model.size - .849999964f) < .000001f
                && cases::defaults(157).model.yaw == -90 && cases::defaults(157).model.roll == 90,
            "approved weapon case profile is built in"
        );
        check(
            std::abs(cases::defaults(173).z + .148999989f) < .000001f
                && cases::defaults(173).model.pitch == -16.5f,
            "approved gadget case profile is built in"
        );
        check(
            std::abs(cases::defaults(1465).model.size - .951899945f) < .000001f,
            "approved gold weapon case profile is built in"
        );
    }
    {
        scene::pickup_preview::Adjustment a;
        auto source = renderer::translation({20, 30, 40});
        const renderer::Vec3 center{5, -3, 2};
        a.model.pitch = 45;
        a.model.roll = 25;
        a.model.size = .5f;
        a.model.x = 1;
        a.model.y = -2;
        a.z = 3;
        const auto result = scene::pickup_preview::apply(source, center, 0, a);
        const auto position = renderer::transform_point(result, center);
        check(
            std::abs(position[0] - 21) < .0001f && std::abs(position[1] - 28) < .0001f
                && std::abs(position[2] - 43) < .0001f,
            "pickup mesh center stays anchored through rotation and scale"
        );
        check(
            std::abs(scene::pickup_preview::defaults(157).model.size - .507799983f) < .000001f
                && scene::pickup_preview::defaults(157).model.yaw == -90
                && std::abs(scene::pickup_preview::defaults(1465).model.size - .600899994f)
                       < .000001f,
            "approved normal and gold weapon pickup adjustments are built in"
        );
        check(
            scene::pickup_preview::defaults(1134).model.size == 1
                && scene::pickup_preview::defaults(1134).model.pitch == 0
                && !scene::pickup_preview::should_adjust(1134, 1134, 1, false)
                && scene::pickup_preview::should_adjust(1134, 157, 1, false)
                && !scene::pickup_preview::should_adjust(1134, 157, 0, true),
            "Gold Bolt rewards retain native settings while world weapon replacements use pickup "
            "defaults"
        );
        check(
            scene::pickup_preview::should_adjust(1120, 157, 1, false)
                && scene::pickup_preview::should_adjust(1120, 1134, 1, false),
            "Eudora equipment pickup supports weapons and Gold Bolt replacements"
        );
        check(
            scene::pickup_preview::defaults(157, true).z == 1
                && scene::pickup_preview::defaults(157, true).model.roll == 90
                && scene::pickup_preview::defaults(157, false).z == 0
                && scene::pickup_preview::defaults(157, false).model.roll == 0
                && std::abs(scene::pickup_preview::defaults(1134, true).model.size - .617899954f)
                       < .000001f
                && scene::pickup_preview::defaults(1134, false).model.size == 1,
            "approved Infobot replacements are built in without changing other pickup defaults"
        );
        const auto bolt =
            scene::pickup_preview::native_gold({10, 20, 30}, 1, .25f, 2, .5f, {30, 40, 50, 90});
        check(
            std::abs(bolt[14] - 32.5f) < .00001f && bolt[12] == 10 && bolt[13] == 20
                && std::abs(
                       std::sqrt(bolt[0] * bolt[0] + bolt[1] * bolt[1] + bolt[2] * bolt[2]) - .25f
                   ) < .00001f,
            "replacement Gold Bolt uses native scale, hover and bob rather than weapon fitting"
        );
    }
    check(
        scene::vendor_preview::defaults(1464).x == 21
            && scene::vendor_preview::defaults(1465).y == -.25f
            && std::abs(
                   scene::vendor_preview::defaults(scene::vendor_preview::key(1464, true)).x - 8.77f
               ) < .001f,
        "approved gold weapon framing is independent for each display"
    );
    check(
        scene::vendor_preview::defaults(scene::vendor_preview::key(454, true)).x == 15.5f
            && scene::vendor_preview::defaults(scene::vendor_preview::key(454, true)).pivot_enabled,
        "approved Batch 1 RYNO middle change is retained"
    );
    check(
        scene::vendor_preview::defaults(scene::vendor_preview::key(1134, true)).x == 13.5f
            && scene::vendor_preview::defaults(scene::vendor_preview::key(1134, true))
                   .pivot_enabled,
        "Gold Bolt middle placement retains reviewed centering and pivot"
    );
    check(
        scene::vendor_preview::defaults(scene::vendor_preview::key(1461, true)).x == 7.5f,
        "Gold Devastator uses the shared centering verified on three planets"
    );
    check(
        scene::vendor_preview::defaults(scene::vendor_preview::key(607, true)).x == 18
            && scene::vendor_preview::defaults(scene::vendor_preview::key(407, true)).x == 4.0f,
        "Heli-Pack centering and the latest approved Persuader middle placement are retained"
    );
    check(
        scene::vendor_preview::defaults(614).y == -12.75f
            && scene::vendor_preview::defaults(618).y == -4
            && std::abs(scene::vendor_preview::defaults(750).size - .5178f) < .0001f,
        "latest user edits preserve left display position and Infobot size"
    );
    check(
        std::abs(scene::vendor_preview::defaults(66995).x + 13.31f) < .001f
            && scene::vendor_preview::defaults(66995).roll == 0,
        "Gold Glove of Doom retains the user's corrected middle placement"
    );
    {
        namespace preview = scene::vendor_preview;
        {
            const renderer::Vec3 offset{.571f, .967f, -.105f};
            const float reference_horizontal = renderer::dot(offset, preview::reference_right);
            for (const renderer::Vec3 right :
                 {preview::reference_right,
                  renderer::Vec3{-.707456648f, -.706756830f, 0},
                  renderer::Vec3{.540597796f, .841281294f, 0}}) {
                const auto adjusted = preview::pivot_anchor(offset, right);
                check(
                    std::abs(renderer::dot(adjusted, right) - reference_horizontal) < .0001f
                        && adjusted[2] == offset[2],
                    "Novalis, Aridia and Kerwan retain the same camera-relative pivot anchor"
                );
            }
        }

        const auto identity = renderer::identity();
        check(
            preview::projection(identity, {}) == identity, "default tuning preserves native framing"
        );
        const auto scaled = preview::projection(identity, {10, 20, 2});
        const auto center = renderer::transform_point(scaled, {-.75f, .75f, 0});
        check(
            std::abs(center[0] - (-.75f + 10.0f / 256)) < .0001f
                && std::abs(center[1] - (.75f - 20.0f / 256)) < .0001f,
            "size scales around the monitor center; horizontal right and vertical down"
        );
        {
            auto turning = identity;
            turning[0] = 0;
            turning[1] = 1;
            turning[4] = -1;
            turning[5] = 0;
            turning[12] = 10;
            turning[13] = 20;
            preview::Adjustment a;
            a.pivot_x = 2;
            a.pivot_enabled = true;
            const auto adjusted = preview::spin_pivot(turning, 3.14159265359f * .5f, a);
            const auto fixed = renderer::transform_point(adjusted, {2, 0, 0});
            check(
                std::abs(fixed[0] - 12) < .0001f && std::abs(fixed[1] - 20) < .0001f,
                "chosen spin pivot remains at its neutral location while the model turns"
            );
            auto camera = identity;
            camera[5] = camera[10] = 0;
            camera[6] = 1;
            camera[9] = -1;
            const auto in_camera =
                renderer::transform_point(renderer::multiply(camera, adjusted), {2, 0, 0});
            const auto expected = renderer::transform_point(camera, fixed);
            check(
                std::abs(in_camera[0] - expected[0]) < .0001f
                    && std::abs(in_camera[1] - expected[1]) < .0001f
                    && std::abs(in_camera[2] - expected[2]) < .0001f,
                "native pivot remains fixed when converted to a tilted vendor camera"
            );
            const auto wrong_order = renderer::transform_point(
                preview::spin_pivot(renderer::multiply(camera, turning), 3.14159265359f * .5f, a),
                {2, 0, 0}
            );
            check(
                std::abs(wrong_order[2] - expected[2]) > .1f,
                "regression distinguishes pivot-before-camera from the incorrect order"
            );
            a.pivot_enabled = false;
            check(
                preview::spin_pivot(turning, 1, a) == turning,
                "disabled pivot preserves native spin"
            );
        }
        const auto rotated = preview::orient(identity, {0, 0, 2, 0, 0, 90}, {1, 2, 3}, true);
        const auto pivot = renderer::transform_point(rotated, {1, 2, 3});
        const auto point = renderer::transform_point(rotated, {2, 2, 3});
        check(
            std::abs(pivot[0] - 1) < .0001f && std::abs(pivot[1] - 2) < .0001f
                && std::abs(point[0] - 1) < .0001f && std::abs(point[1] - 4) < .0001f,
            "rotation and size preserve the model pivot"
        );
        check(
            preview::get(preview::key(185, true)).x == 22.25f,
            "Morph-o-Ray retains its separately reviewed middle profile"
        );
        {
            const auto left = preview::defaults(609);
            const auto middle = preview::defaults(preview::key(609, true));
            check(
                left.x == 44.5f && left.pitch == -25.5f && left.size > 1.62f,
                "approved Hydro-Pack left framing is available without an override file"
            );
            check(
                std::abs(middle.x - 15.11f) < .001f && middle.roll == 53 && middle.pivot_enabled
                    && middle.pivot_x > 1.02f && middle.spin,
                "approved Hydro-Pack middle framing includes rotation and its spin pivot"
            );
        }
    }
    check(
        !scene::preview_replaces_class(157, -1, 157, true),
        "native Devastator inset retains its authored joint pose"
    );
    check(
        !scene::preview_replaces_class(229, -1, 229, true),
        "native glove inset retains its bent-finger pose"
    );
    check(
        scene::preview_replaces_class(750, -1, 157, true),
        "Infobot replacing a weapon still uses its replacement pose"
    );
    check(
        scene::preview_replaces_class(157, 163, 157, true),
        "explicit model override is compared before the object's own class"
    );
    check(
        scene::preview_replaces_class(157, -1, 157, false),
        "centre hologram keeps its previously tested behavior"
    );
    for (const renderer::Vec3 eye :
         {renderer::Vec3{10, 0, 3}, {-10, 0, 3}, {0, 10, 3}, {0, -10, 3}}) {
        auto source = renderer::identity();
        source[5] = 0;
        source[6] = 1;
        source[9] = -1;
        source[10] = 0;  // tilted source item
        const auto facing = scene::facing_preview(source, eye, {-1, 0, 0}, {0, 0, 1});
        const renderer::Vec3 front{facing[0], facing[1], facing[2]};
        check(renderer::dot(front, eye) > 9.9f, "faces cameras on all four sides");
        check(facing[8] == 0 && facing[9] == 0 && facing[10] == 1, "ignores source item tilt");
    }
    const auto stem =
        std::filesystem::temp_directory_path()
        / ("reward-animation-"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto bin = stem.string() + ".bin";
    const auto gltf = stem.string() + ".gltf";
    const float keys[] = {0, 1, 0, 0, 0, 2, 0, 0};
    {
        std::ofstream out(bin, std::ios::binary);
        out.write(reinterpret_cast<const char*>(keys), sizeof keys);
    }
    {
        std::ofstream out(gltf);
        out << R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":32,"uri":")"
            << std::filesystem::path(bin).filename().string() << R"("}],
        "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":8},{"buffer":0,"byteOffset":8,"byteLength":24}],
        "accessors":[{"bufferView":0,"componentType":5126,"count":2,"type":"SCALAR"},
        {"bufferView":1,"componentType":5126,"count":2,"type":"VEC3"}],
        "nodes":[{"name":"joint_000"}],"skins":[{"joints":[0]}],
        "animations":[{"name":"idle","samplers":[{"input":0,"output":1,"interpolation":"LINEAR"}],
        "channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}]})";
    }
    scene::Model model;
    model.bones.resize(1);
    check(scene::load_preview_animation(gltf, model, "idle"), "load exported animation");
    check(model.preview_poses.size() == 60, "sample one-second loop at 60 Hz");
    if (model.preview_poses.size() == 60) {
        check(std::abs(model.preview_poses[0][0][12]) < 0.0001f, "initial pose");
        check(
            std::abs(model.preview_poses[30][0][12] - 1) < 0.0001f,
            "interpolate translation in model units"
        );
    }
    check(
        !scene::load_preview_animation(gltf, model, "missing"), "missing clip preserves fallback"
    );
    std::filesystem::remove(gltf);
    std::filesystem::remove(bin);
    if (argc > 1) {
        scene::LevelData level;
        std::string error;
        const auto loaded = scene::load_model(argv[1], level, error);
        check(loaded.has_value(), "load real Kerwan Infobot mesh");
        if (loaded) {
            auto& bot = level.models[*loaded];
            const char* clip = argc > 2 ? argv[2] : "seq_00_loop";
            check(
                scene::load_preview_animation(argv[1], bot, clip, &level),
                "load authored display pose"
            );
            check(
                bot.preview_bounds && bot.preview_max[2] > bot.preview_min[2],
                "frame full animated bounds"
            );
            bool changed = false;
            for (const auto& pose : bot.preview_poses) {
                if (pose != bot.preview_poses.front()) {
                    changed = true;
                }
                for (const auto& matrix : pose) {
                    for (float value : matrix) {
                        check(std::isfinite(value), "finite real animation pose");
                    }
                }
            }
            if (argc <= 2) {
                check(changed, "real idle loop animates its joints");
            } else {
                check(bot.preview_poses.size() == 1, "static retail display pose is retained");
            }
        }
    }
    return failures ? 1 : 0;
}
