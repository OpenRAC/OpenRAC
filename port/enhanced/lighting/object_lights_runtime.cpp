// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Class-bound authored lights become world-space Forward+ records using the displayed joint palette.
#include "common/log.h"
#include "enhanced/lighting/object_lights.h"

namespace openrac::enhanced {
void ObjectLights::load(const scene::LevelData& level) {
    release();
    m_game = level.game;
    auto catalog = load_object_lights(level);

    if (!catalog) {
        log::error("object lights: {}", catalog.error().message);
        catalog = ObjectLightCatalog{};
    }

    publish_object_lights(m_game, std::move(catalog.value()));

    for (unsigned layer = 0; layer < scene::kLayerCount; ++layer) {
        std::vector<unsigned> models;

        if (layer == unsigned(scene::Layer::Mobys)) {
            for (const auto& [id, entry] : level.moby_classes) {
                models.push_back(entry.first);
            }
        } else {
            m_static[layer] = level.instances[layer];

            for (const auto& instance : level.instances[layer]) {
                models.push_back(instance.model);
            }
        }

        for (unsigned model : models) {
            if (model < level.models.size() && !m_models[layer].contains(model)) {
                m_models[layer][model] = {
                    object_light_key(level, scene::Layer(layer), model), level.models[model].bones
                };
            }
        }
    }

    refresh();
}

void ObjectLights::refresh() {
    m_revision = object_lights_revision();
    const auto catalog = current_object_lights(m_game);

    for (unsigned layer = 0; layer < scene::kLayerCount; ++layer) {
        m_bindings[layer].clear();

        if (!catalog) {
            continue;
        }

        for (const auto& [model, entry] : m_models[layer]) {
            const auto found = catalog->objects.find(entry.first);

            if (found != catalog->objects.end() && !found->second.empty()) {
                m_bindings[layer][model] = {entry.second, found->second};
            }
        }

        if (layer != unsigned(scene::Layer::Mobys)) {
            instances(scene::Layer(layer), m_static[layer]);
        }
    }

    m_frame = ~std::uint64_t{0};
}

void ObjectLights::instances(scene::Layer layer, std::span<const scene::Instance> instances) {
    const auto at = unsigned(layer);

    if (m_revision != object_lights_revision()) {
        refresh();
    }

    if (layer != scene::Layer::Mobys && layer != scene::Layer::Animated
        && instances.data() != m_static[at].data()) {
        return;
    }

    m_instances[at].clear();

    if (m_bindings[at].empty()) {
        return;
    }

    for (const auto& instance : instances) {
        // Vendor screens and menu models belong to a different camera space, not the game world.
        if (instance.moby_light[3] > 1.5f || instance.tint[3] <= 0) {
            continue;
        }

        if (m_bindings[at].contains(instance.model)) {
            m_instances[at].push_back(instance);
        }
    }
}

// Resolve each bound emitter once; repeated renderer passes reuse the resulting snapshot.
// Invalid individual attachments are omitted explicitly; capacity failure rejects the whole set.
bool ObjectLights::append_instance(
    const scene::Instance& instance,
    const Binding& binding,
    std::span<const renderer::Mat4> palette,
    std::string& error
) {
    for (const auto& light : binding.lights) {
        if (!light.enabled) {
            continue;
        }

        auto resolved = resolve_attached_light(light, instance, palette, binding.bones);

        if (!resolved) {
            error = resolved.error().message;
            continue;
        }

        m_resolved.push_back(resolved.value());

        if (m_resolved.size() > 4096) {
            error = "more than 4096 attached lights are active; reduce the authored light count";
            return false;
        }
    }

    return true;
}

// Keep persistent bad-profile errors visible without flooding every frame of the log.
void ObjectLights::report_error(std::string error) {
    if (error != m_error && !error.empty()) {
        log::error("object lights: {}", error);
    }

    m_error = std::move(error);
}

std::span<const LocalLight> ObjectLights::prepare(
    std::uint64_t frame, std::span<const renderer::Mat4> palette
) {
    if (m_revision != object_lights_revision()) {
        refresh();
    }

    if (frame == m_frame) {
        return m_resolved;
    }

    m_frame = frame;
    m_resolved.clear();
    std::string error;

    for (unsigned layer = 0; layer < scene::kLayerCount; ++layer) {
        for (const auto& instance : m_instances[layer]) {
            const auto found = m_bindings[layer].find(instance.model);

            if (found == m_bindings[layer].end()) {
                continue;
            }

            if (!append_instance(instance, found->second, palette, error)) {
                m_resolved.clear();
                report_error(std::move(error));
                return m_resolved;
            }
        }
    }

    report_error(std::move(error));
    return m_resolved;
}

void ObjectLights::release() {
    m_bindings = {};
    m_instances = {};
    m_static = {};
    m_models = {};
    m_resolved.clear();
    m_error.clear();
    m_frame = ~std::uint64_t{0};
    m_revision = ~std::uint64_t{0};
}
}  // namespace openrac::enhanced
