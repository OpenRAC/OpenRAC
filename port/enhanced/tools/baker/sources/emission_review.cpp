// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/tools/baker/sources/emission_review.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"
#include "scene/material_overrides.h"
#include "scene/object_picker.h"

namespace openrac::lighting {
namespace {
scene::Vec3 camera_vector(const json::Value& value) {
    scene::Vec3 result{};
    if (value.items().size() != result.size()) {
        throw std::runtime_error("Expected a three-component review camera vector");
    }
    for (unsigned i = 0; i < result.size(); ++i) {
        result[i] = static_cast<float>(value[i].number());
        if (!value[i].is_number() || !std::isfinite(result[i])) {
            throw std::runtime_error("Invalid review camera component");
        }
    }
    return result;
}

void source_json(std::ostream& out, const scene::MaterialSource& source, int instances) {
    const auto& id = source.identity;
    out << "{\"model\":" << coordinator_string(id.model)
        << ",\"material\":" << coordinator_string(id.material_name)
        << ",\"layer\":" << coordinator_string(id.layer) << ",\"class_id\":" << id.class_id
        << ",\"material_slot\":";
    if (id.material_slot) {
        out << *id.material_slot;
    } else {
        out << "null";
    }
    out << ",\"texture_path\":" << coordinator_string(id.texture_path)
        << ",\"texture_hash\":" << coordinator_string(id.texture_hash)
        << ",\"texture_file_hash\":" << coordinator_string(id.texture_file_hash)
        << ",\"exported_instances\":" << instances
        << ",\"static_transport\":" << (source.transport ? "true" : "false") << '}';
}

struct Candidate {
    scene::PickHit hit;
    std::vector<std::array<float, 2>> samples;
};

float aim_distance(const std::array<float, 2>& point) {
    return point[0] * point[0] + point[1] * point[1];
}

std::vector<Candidate> sample_view(const scene::LevelData& level, const renderer::Camera& camera) {
    scene::ObjectPicker picker;
    std::vector<std::array<float, 2>> rays;
    for (int y = -4; y <= 4; ++y) {
        for (int x = -4; x <= 4; ++x) {
            rays.push_back({x * .22f, y * .22f});
        }
    }
    std::stable_sort(rays.begin(), rays.end(), [](auto a, auto b) {
        return aim_distance(a) < aim_distance(b);
    });

    std::vector<Candidate> candidates;
    const auto& mobys = level.instances[unsigned(scene::Layer::Mobys)];
    for (auto ndc : rays) {
        auto hit = picker.cast(level, mobys, {}, camera, 0x3e, ndc);
        if (!hit) {
            continue;
        }
        const auto previous =
            std::find_if(candidates.begin(), candidates.end(), [&](const auto& c) {
                return c.hit.model == hit->model && c.hit.material == hit->material
                       && c.hit.layer == hit->layer && c.hit.instance == hit->instance;
            });
        if (previous == candidates.end()) {
            candidates.push_back({std::move(*hit), {ndc}});
        } else {
            previous->samples.push_back(ndc);
        }
    }
    return candidates;
}
}  // namespace

void inspect_emissive_task(
    const std::filesystem::path& levels,
    const std::filesystem::path& review,
    std::string_view task_id,
    std::ostream& out
) {
    const auto document = bake_json::document(review);
    const auto& tasks = document["tasks"].items();
    const auto task = std::find_if(tasks.begin(), tasks.end(), [&](const auto& entry) {
        return entry["id"].string() == task_id;
    });
    if (task == tasks.end()) {
        throw std::runtime_error("Review task not found: " + std::string(task_id));
    }
    const double number = (*task)["level"].number(-1);
    if (number < 0 || number > 99 || std::floor(number) != number) {
        throw std::runtime_error("Invalid review task level");
    }
    const int level_number = static_cast<int>(number);
    renderer::Camera camera;
    camera.position = camera_vector((*task)["camera"]["position"]);
    const auto forward = renderer::normalize(camera_vector((*task)["camera"]["forward"]));
    if (!std::isfinite(renderer::dot(forward, forward)) || renderer::dot(forward, forward) < .5f) {
        throw std::runtime_error("Invalid review camera direction");
    }
    const scene::Vec3 up =
        std::abs(forward[2]) > .999f ? scene::Vec3{0, 1, 0} : scene::Vec3{0, 0, 1};
    camera.view = renderer::look_along(camera.position, forward, up);
    // Match the NTSC review camera's nominal frustum. NDC is relative to the game picture.
    camera.projection =
        renderer::perspective_reversed(2 * std::atan(.488f), .630f / .488f, .05f, 2000);

    scene::LevelData level;
    std::string error;
    const auto dir = levels / std::format("level_{:02}", level_number);
    if (!scene::load_level(dir, level, error)) {
        throw std::runtime_error(error);
    }
    const auto manifest = bake_json::document(dir / "manifest.json");
    const std::string game = level.game + "-" + manifest["version"].string();
    if (level.level != level_number
        || (!document["game"].is_null() && document["game"].string() != game)) {
        throw std::runtime_error("Level export does not match the review task");
    }
    if (!level.material_bindings) {
        throw std::runtime_error(
            "Cannot resolve exported material identities: " + level.material_error
        );
    }
    const auto& bindings = *level.material_bindings;
    const auto& sources = bindings.assets->sources;
    std::vector<int> instances(level.models.size());
    for (const auto& layer : level.instances) {
        for (const auto& instance : layer) {
            if (!instance.lighting_only && instance.model < instances.size()) {
                ++instances[instance.model];
            }
        }
    }
    const auto candidates = sample_view(level, camera);
    out << "{\"version\":1,\"task_id\":" << coordinator_string(task_id)
        << ",\"camera\":" << bake_json::json_text((*task)["camera"])
        << ",\"level\":" << level_number << ",\"rays\":81,\"candidates\":[";
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& candidate = candidates[i];
        const auto& hit = candidate.hit;
        if (i) {
            out << ',';
        }
        out << "{\"label\":" << i + 1
            << ",\"pick\":" << scene::picker_report_json({true, level_number, 0, hit}, true, game)
            << ",\"samples\":[";
        for (std::size_t j = 0; j < candidate.samples.size(); ++j) {
            const auto& point = candidate.samples[j];
            out << (j ? "," : "") << std::format("[{},{}]", point[0], point[1]);
        }
        out << "],\"source\":";
        const auto source = bindings.lookup.find({hit.model, hit.material});
        if (source == bindings.lookup.end()) {
            out << "null,\"same_level_texture_uses\":[]}";
            continue;
        }
        const auto& material = sources[source->second];
        source_json(out, material, instances[material.model]);
        out << ",\"same_level_texture_uses\":[";
        bool separator = false;
        for (const auto& other : sources) {
            if (&other == &material || material.identity.texture_hash.empty()
                || other.identity.texture_hash != material.identity.texture_hash) {
                continue;
            }
            out << (separator ? "," : "");
            source_json(out, other, instances[other.model]);
            separator = true;
        }
        out << "]}";
    }
    out << "]}\n";
}
}  // namespace openrac::lighting
