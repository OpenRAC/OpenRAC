// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

#include "common/env.h"
#include "common/profiler.h"
#include "common/stats.h"
#include "enhanced/renderer/water_impulse_contact.h"
#include "enhanced/renderer/water_internal.h"
#include "enhanced/renderer/water_window.h"

namespace openrac::enhanced::water_detail {

// Each pass needs a fresh scene snapshot, even when the simulation tick is already prepared.
void Water::prepare(const renderer::FrameInput& input) {
    m_background_textures = {};
    prepare_frame(input, true);
}

// Prepare the simulation once per tick and apply session or scripted diagnostic views.
void Water::prepare_frame(const renderer::FrameInput& input, bool lighting) {
    const profiling::Zone trace("Prepare enhanced water");

    m_mesh->begin_pass();
    m_ocean_drawn = false;
    m_ocean_eye = input.camera.position;
    m_view_projection = renderer::multiply(input.camera.projection, input.camera.view);
    m_bound = false;
    m_effect_setups = 0;
    const auto options = water_debug_settings();
    const auto epoch = water_reset_epoch.load();
    const bool debug_reset =
        epoch != m_reset_epoch || options.current_scale != m_debug.current_scale;
    m_debug = options;
    const auto scripted_view = env::integer(env::Var::WaterView, -1);
    if (scripted_view >= 0) {
        m_debug.view = int(std::clamp<std::int64_t>(scripted_view, 0, 17));
    }
    m_enabled = input.enhanced_water && !water_disabled();
    water_ready.store(m_enabled);
    if (!m_enabled) {
        m_lighting_pending = false;
        m_impulse_frame = input.frame;
        m_last_frame = m_prepared_frame = m_clock_frame = std::numeric_limits<std::uint64_t>::max();
        m_sim_body = -1;
        m_exit_time = -1;
        m_sim_weight = 1;
        m_reset_epoch = epoch;
        return;
    }
    if (input.frame == m_prepared_frame && !debug_reset) {
        if (lighting && m_lighting_pending) {
            update_lighting(input);
            m_lighting_pending = false;
        }
        return;
    }
    m_prepared_frame = input.frame;
    m_lighting_pending = false;
    if (m_clock_frame != input.frame) {
        if (!m_debug.paused && m_clock_frame != std::numeric_limits<std::uint64_t>::max()
            && input.frame > m_clock_frame) {
            m_time += double(std::min<std::uint64_t>(input.frame - m_clock_frame, 4)) / 60.;
        }
        m_clock_frame = input.frame;
    }
    update_ocean();
    std::array<Texel, renderer::kWaterImpulseLimit> impulse_starts{}, impulse_ends{};
    unsigned impulse_count = 0;
    if (m_impulse_frame != input.frame && !m_debug.paused) {
        for (auto impulse : input.water_impulses) {
            if (impulse_count == renderer::kWaterImpulseLimit) {
                break;
            }
            if (!renderer::valid_water_impulse(impulse)) {
                continue;
            }
            // Capture supplies zero-length explosions and nonzero swept projectiles.
            // Tune before both activation/contact queries and GPU kicks, so the slider's
            // radius and zero-strength opt-out agree even for an idle pond.
            const bool explosion = impulse.from == impulse.to;
            impulse.strength = std::min(
                4.f,
                impulse.strength
                    * (explosion ? m_debug.explosion_strength : m_debug.projectile_strength)
            );
            impulse.radius = std::clamp(
                impulse.radius * (explosion ? m_debug.explosion_radius : m_debug.projectile_radius),
                .05f,
                4.f
            );
            if (impulse.strength == 0) {
                continue;
            }
            impulse_starts[impulse_count] =
                {impulse.from[0], impulse.from[1], impulse.from[2], impulse.radius};
            impulse_ends[impulse_count++] =
                {impulse.to[0], impulse.to[1], impulse.to[2], impulse.strength};
        }
    }
    m_impulse_frame = input.frame;
    auto hero = input.water_interactor.value_or(renderer::Vec3{0, 0, 100000});
    unsigned visible_surfaces = 0;
    float ocean_height = std::abs(m_coast.front()[0]) - 512.f;
    const auto view_projection = renderer::multiply(input.camera.projection, input.camera.view);
    for (const auto& strip : input.strips) {
        if (strip.surface == 0 || strip.surface > 7 || strip.count < 3
            || std::size_t(strip.first) + strip.count > input.strip_vertices.size()) {
            continue;
        }
        renderer::Vec3 low{1e20f, 1e20f, 1e20f}, high{-1e20f, -1e20f, -1e20f};
        for (const auto& v : input.strip_vertices.subspan(strip.first, strip.count)) {
            const renderer::Vec3 p{v.x, v.y, v.z};
            for (unsigned axis = 0; axis < 3; ++axis) {
                low[axis] = std::min(low[axis], p[axis]);
                high[axis] = std::max(high[axis], p[axis]);
            }
        }
        if (m_ocean_spectra[0] && strip.surface == 1) {
            ocean_height = input.strip_vertices[strip.first].z;
        }
        if (water_window_visible(view_projection, low, high)) {
            visible_surfaces |= 1u << strip.surface;
        }
    }
    int selected = -1;
    bool contact_water = false;
    float selected_height = m_sim_height;
    float best = std::numeric_limits<float>::max();
    // Only static bake metadata is queried here. The dynamic state stays on the GPU.
    for (unsigned b = 0; b < m_bodies.size() / 5; ++b) {
        const auto* body = m_bodies.data() + static_cast<std::size_t>(b) * 5;
        const int width = int(body[1][3]), height = int(body[2][3]);
        const int x = std::clamp(int(std::floor((hero[0] - body[0][0]) / .5f)), 0, width - 1);
        const int y = std::clamp(int(std::floor((hero[1] - body[0][1]) / .5f)), 0, height - 1);
        const bool visible = (visible_surfaces & (1u << unsigned(body[0][3]))) != 0;
        // Clipped bounds visit each edge cell once, retaining scan/tie order.
        for (int cy = std::max(y - 3, 0); cy <= std::min(y + 3, height - 1); ++cy) {
            for (int cx = std::max(x - 3, 0); cx <= std::min(x + 3, width - 1); ++cx) {
                const auto& cell = m_coast[unsigned(body[3][3]) + cy * width + cx];
                if (cell[0] < .5f) {
                    continue;
                }
                const float px = body[0][0] + (float(cx) + .5f) * .5f;
                const float py = body[0][1] + (float(cy) + .5f) * .5f;
                const float dz = hero[2] - (cell[0] - 512.f);
                const float xy = (hero[0] - px) * (hero[0] - px) + (hero[1] - py) * (hero[1] - py);
                const bool contact = xy < 4.f && dz > -1.8f && dz < .5f;
                if ((!visible && !contact)
                    || xy > (kWindowSize * kWindowSpacing * .46875f)
                                * (kWindowSize * kWindowSpacing * .46875f)
                    || std::abs(dz) > 12.f) {
                    continue;
                }
                contact_water = contact_water || contact;
                float score = xy + dz * dz * 4.f;
                if (int(b) == m_sim_body) {
                    score *= .9f;
                }
                if (score < best) {
                    best = score;
                    selected = int(b);
                    selected_height = cell[0] - 512.f;
                }
            }
        }
    }
    const auto open_ocean = [&](const renderer::Vec3& point) {
        const bool outside = point[0] < m_ocean_grid[0] || point[1] < m_ocean_grid[1]
                             || point[0] >= m_ocean_grid[0] + kOceanSide * .5f
                             || point[1] >= m_ocean_grid[1] + kOceanSide * .5f;
        const auto end = m_ocean_dry_regions.begin() + m_ocean_dry_count;
        const bool dry = std::any_of(m_ocean_dry_regions.begin(), end, [&](const Texel& rect) {
            return point[0] >= rect[0] && point[1] >= rect[1] && point[0] < rect[2]
                   && point[1] < rect[3];
        });
        return m_ocean_spectra[0] && outside && !dry;
    };
    if (!contact_water && open_ocean(hero)) {
        const float dz = hero[2] - ocean_height;
        if (dz > -1.8f && dz < .5f) {
            selected = 0;
            selected_height = ocean_height;
            contact_water = true;
        }
    }
    // Hits wake water even when Ratchet is on the bank or above the pond. Keep an active
    // swimming window in preference to remote hits; one bounded local grid remains in use.
    bool effect_contact = false;
    if (!contact_water && impulse_count) {
        for (unsigned i = 0; i < impulse_count; ++i) {
            const auto& a = impulse_starts[i];
            const auto& b = impulse_ends[i];
            const renderer::WaterImpulse impulse{
                {a[0], a[1], a[2]}, {b[0], b[1], b[2]}, a[3], b[3]
            };
            if (const auto hit = water_impulse_contact(m_bodies, m_coast, impulse)) {
                hero = hit->point;
                selected = hit->body;
                selected_height = hero[2];
                contact_water = effect_contact = true;
                break;
            }
            if (m_ocean_spectra[0]) {
                const float dz = b[2] - a[2];
                const float t =
                    std::abs(dz) > 1e-5f ? std::clamp((ocean_height - a[2]) / dz, 0.f, 1.f) : 0.f;
                const renderer::Vec3 point{
                    std::lerp(a[0], b[0], t), std::lerp(a[1], b[1], t), ocean_height
                };
                if (std::abs(std::lerp(a[2], b[2], t) - ocean_height) <= b[3]
                    && open_ocean(point)) {
                    hero = point;
                    selected = 0;
                    selected_height = ocean_height;
                    contact_water = effect_contact = true;
                    break;
                }
            }
        }
    }
    // Let the last wake settle for five seconds after contact/relevance ends.
    // Keep its window fixed so Ratchet walking away cannot drag or reset it.
    const bool retiring = m_sim_body >= 0 && !contact_water;
    if (contact_water) {
        m_exit_time = -1;
        m_sim_weight = 1;
    } else if (retiring) {
        if (m_exit_time < 0) {
            m_exit_time = m_time;
        }
        const float elapsed = float((m_time - m_exit_time) / 5.);
        const float t = std::clamp(elapsed, 0.f, 1.f);
        m_sim_weight = 1.f - t * t * (3.f - 2.f * t);
        selected = t < 1.f ? m_sim_body : -1;
        selected_height = m_sim_height;
        hero = m_hero;
    } else if (m_exit_time >= 0) {
        selected = -1;
    }
    const bool compatible =
        selected >= 0 && m_sim_body >= 0 && water_window_same_height(selected_height, m_sim_height);
    const bool changed = selected != m_sim_body && !compatible;
    m_sim_body = selected;
    m_sim_height = selected_height;
    stats::gauge("water simulation cells", selected >= 0 ? double(kWindowCells) : 0.);
    stats::gauge("water simulation body", double(selected));
    if (visible_surfaces != 0 || selected >= 0) {
        if (lighting) {
            update_lighting(input);
        } else {
            m_lighting_pending = true;
        }
    }
    if (selected < 0) {
        remember_tick(input, hero, epoch);
        m_last_frame = std::numeric_limits<std::uint64_t>::max();
        return;
    }
    const std::array<int, 2> window =
        retiring ? m_window_origin
                 : std::array<int, 2>{
                       int(std::floor(hero[0] / kWindowSpacing)) - int(kWindowSize / 2),
                       int(std::floor(hero[1] / kWindowSpacing)) - int(kWindowSize / 2)
                   };
    float distance = 0;
    for (unsigned a = 0; a < 3; ++a) {
        distance += (hero[a] - m_hero[a]) * (hero[a] - m_hero[a]);
    }
    const bool reset =
        changed || debug_reset || m_last_frame == std::numeric_limits<std::uint64_t>::max()
        || input.frame < m_last_frame || input.frame - m_last_frame > 8 || distance > 144;
    unsigned steps = 0;
    if (reset) {
        steps = 1;
    } else if (!m_debug.paused) {
        steps = unsigned(std::min<std::uint64_t>(input.frame - m_last_frame, 4));
    }
    if (steps == 0) {
        remember_tick(input, hero, epoch);
        return;
    }
    const float impulse =
        input.water_interactor && !reset && !retiring && !effect_contact
            ? std::min(1.0f, std::sqrt(distance) * 2.5f) * m_debug.interaction_strength
            : 0.f;
    std::array<float, 2> hand_impulses{};
    for (unsigned hand = 0; hand < hand_impulses.size(); ++hand) {
        if (reset || retiring || effect_contact || !input.water_hands[hand] || !m_hands[hand]) {
            continue;
        }
        float moved = 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float delta = (*input.water_hands[hand])[axis] - (*m_hands[hand])[axis];
            moved += delta * delta;
        }
        // Ignore discontinuous poses; a hand cannot sweep metres in one tick.
        if (moved < 4.f) {
            hand_impulses[hand] =
                std::min(1.f, std::sqrt(moved) * 5.f) * m_debug.interaction_strength * .5f;
        }
    }
    const SimulationUpdate update{
        input,
        hero,
        window,
        hand_impulses,
        std::span<const Texel>(impulse_starts).first(impulse_count),
        std::span<const Texel>(impulse_ends).first(impulse_count),
        impulse,
        steps,
        reset
    };
    if (!simulate(update)) {
        m_enabled = false;
        water_ready.store(false);
        return;
    }
    water_ready.store(true);
    m_window_origin = window;
    remember_tick(input, hero, epoch);
    stats::gauge("water simulation state bytes", double(kWindowCells * sizeof(Texel) * 2));
    stats::gauge("water projectile and blast impulses", double(impulse_count));
    stats::gauge("water impulse", double(impulse));
    stats::gauge("water hand impulse", double(hand_impulses[0] + hand_impulses[1]));
}

void Water::remember_tick(
    const renderer::FrameInput& input, const renderer::Vec3& hero, std::uint64_t epoch
) {
    m_reset_epoch = epoch;
    m_last_frame = input.frame;
    m_hero = hero;
    m_hands = input.water_hands;
}
}  // namespace openrac::enhanced::water_detail
