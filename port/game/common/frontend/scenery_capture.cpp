// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reads scenery_capture.h's records from a stable game-memory snapshot.
// Record fields preserve the existing frontend capture layout; camera culling stays in the
// visible renderer so offscreen geometry remains available to the shadow passes.

#include "game/common/frontend/scenery_capture.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

#include "openrac/guest.h"

namespace openrac::frontend {
namespace {

// Views valid only for one synchronous capture; all output records are owned by `output`.
struct Capture {
    std::span<const std::uint8_t> ram;          // Stable guest memory owned by the caller.
    const std::array<std::uint32_t, 4>& words;  // Profile-owned pointer-global addresses.
    const host::GameState& state;               // Caller-owned camera snapshot.
    float scale;                                // Draw-distance multiplier; zero is unlimited.
    const std::array<std::size_t, 2>& counts;   // Caller-owned layer instance counts.
    int words_level;                            // The words' program: -1 the executable's.
    std::uint32_t tie_cap_after;                // The ties' cap past the records' word.
    CapturedScenery output;                     // Owned result and resolved caps.

    // A word of the game's, in the loaded program: from the executable's addresses, or from the
    // level's whose program the profile names them in.
    gaddr relocate(std::uint32_t address) const {
        return words_level >= 0 ? openrac_relocate_level_data(words_level, address)
                                : openrac_relocate_data(address);
    }

    // Resolves a record table only when its aligned extent fits in the captured RAM.
    gaddr table(std::uint32_t address, std::size_t count, std::size_t size) const {
        // Missing globals and empty layers have no table to capture.
        if (address == 0 || count == 0) {
            return 0;
        }

        const gaddr word = relocate(address);

        // Pointer globals must fit before their table address can be read.
        if (word + 4 > ram.size()) {
            return 0;
        }

        gaddr at;
        std::memcpy(&at, ram.data() + word, 4);
        return at != 0 && at % 16 == 0 && at + count * size <= ram.size() ? at : 0;
    }

    // Reads a relocated pointer or cap from the captured RAM.
    std::optional<std::uint32_t> word_at_address(std::uint32_t address) const {
        const gaddr at = relocate(address);

        // Unavailable or truncated globals leave their optional cap unset.
        if (address == 0 || at + 4 > ram.size()) {
            return std::nullopt;
        }

        std::uint32_t v;
        std::memcpy(&v, ram.data() + at, 4);
        return v;
    }

    // Measures a sphere centre along the captured game view, in world units.
    std::optional<float> depth(const float* centre) const {
        // A missing captured view leaves distance rejection to the renderer.
        if (!state.moby_view) {
            return std::nullopt;
        }

        const auto& a = state.moby_view->depth_axis;
        const auto& eye = state.camera_position;
        return a[0] * (centre[0] - eye[0]) + a[1] * (centre[1] - eye[1])
               + a[2] * (centre[2] - eye[2]);
    }

    // Retains behind-camera casters while applying the configured far draw distance.
    bool beyond(const float* sphere, float reach) const {
        const auto z = depth(sphere);
        return z && scale > 0.0f && *z > reach * scale;
    }

    // Copies both layers' affine matrices without retaining live game-memory views.
    void matrices() {
        // Matrix globals are ordered as ties followed by shrubs.
        for (std::size_t k = 0; k < counts.size(); ++k) {
            const std::size_t count = counts[k];
            const gaddr at = table(words[k], count, 0x40);

            // Missing tables retain the level's exported placement.
            if (at == 0) {
                continue;
            }

            auto& out = output.scenery.matrices[k];
            out.resize(count);

            // Each record stores four padded columns; capture only their three affine lanes.
            for (std::size_t i = 0; i < count; ++i) {
                const std::uint8_t* m = ram.data() + at + i * 0x40;

                // Padding lanes do not form part of the scene's affine matrix.
                for (std::size_t c = 0; c < 4; ++c) {
                    std::memcpy(&out[i][c * 3], m + c * 0x10, 12);
                }
            }
        }
    }

    // Reads tie lighting and persistent hiding; camera visibility never removes a caster.
    void read_ties() {
        // A missing record table leaves ties as exported.
        if (const gaddr at = table(words[2], counts[0], 0x20); at != 0) {
            output.scenery.tie_spheres.resize(counts[0]);
            output.scenery.tie_lights.resize(counts[0]);
            output.scenery.tie_hidden.resize(counts[0]);
            // The cap, in 16-unit steps (lw s1; srl 4): a tie's own distance is its record's byte at
            // +0x1C in the same steps, and the cull takes the smaller, shifted back (pminw; dsll32 4).
            const auto cap = words[2] != 0 ? word_at_address(words[2] + tie_cap_after) : std::nullopt;
            const std::uint32_t cap_steps = cap ? *cap >> 4 : 0;
            output.tie_cap = cap_steps << 4;

            // Persistent hiding and far distance apply to both visible geometry and shadow casters.
            for (std::size_t i = 0; i < counts[0]; ++i) {
                const std::uint8_t* r = ram.data() + at + i * 0x20;
                std::memcpy(output.scenery.tie_spheres[i].data(), r, 16);
                std::memcpy(&output.scenery.tie_lights[i], r + 0x1A, 2);
                const std::uint32_t steps = std::min<std::uint32_t>(r[0x1C], cap_steps);
                const bool far =
                    cap_steps != 0
                    && beyond(output.scenery.tie_spheres[i].data(), static_cast<float>(steps << 4));
                output.scenery.tie_hidden[i] = (r[0x1E] & 4) || far;
            }
        }
    }

    // Reads terrain occlusion words and far-distance flags from the captured records.
    void read_terrain() {
        // The terrain's occlusion words: CullAndEmitTfrags (0x2EF480) walks the 0x40-byte tfrag records
        // from the word at gp + 0x3180 (0x44 before the ties' records' word), as many as gp + 0x3184
        // says, and leaves out one whose bit at +0x3A is clear in the mask's byte at +0x3B.
        // Games without tie records expose no associated terrain table.
        if (words[2] == 0) {
            return;
        }

        const auto count = word_at_address(words[2] - 0x40);

        // Reject missing or implausibly large fragment counts before resolving the table.
        if (!count || *count == 0 || *count >= 0x10000) {
            return;
        }

        const gaddr at = table(words[2] - 0x44, *count, 0x40);

        // A missing table leaves the exported terrain visibility unchanged.
        if (at == 0) {
            return;
        }

        // The far cap is stored in 1/1024 world units beside the tie-record pointer.
        float far_cull = 0.0f;

        if (const auto bits = word_at_address(words[2] - 0x14)) {
            std::memcpy(&far_cull, &*bits, 4);
        }

        output.scenery.terrain_occlusion.resize(*count);
        output.scenery.terrain_far.resize(*count);

        // Copy the occlusion word and compare each sphere's near side with the scaled far cap.
        for (std::size_t i = 0; i < *count; ++i) {
            const std::uint8_t* r = ram.data() + at + i * 0x40;
            output.scenery.terrain_occlusion[i] =
                static_cast<std::uint16_t>(r[0x3B] << 8 | r[0x3A]);
            float sphere_units[4];
            std::memcpy(sphere_units, r, sizeof(sphere_units));
            const float sphere[4] =
                {sphere_units[0] / 1024.0f,
                 sphere_units[1] / 1024.0f,
                 sphere_units[2] / 1024.0f,
                 sphere_units[3] / 1024.0f};
            const auto z = depth(sphere);
            output.scenery.terrain_far[i] =
                far_cull > 0.0f && scale > 0.0f && z && *z - sphere[3] > far_cull / 1024.0f * scale;
        }
    }

    // Reads shrub lighting and persistent hiding without camera near-plane rejection.
    void read_shrubs() {
        // A missing record table leaves shrubs as exported.
        if (const gaddr at = table(words[3], counts[1], 0x20); at != 0) {
            output.scenery.shrub_spheres.resize(counts[1]);
            output.scenery.shrub_lights.resize(counts[1]);
            output.scenery.shrub_hidden.resize(counts[1]);
            // The cap, a float; a shrub's own distance is its record's float at +0x10 as the loader
            // left it, and the cull takes the smaller (pminw on their bits).
            float cap = 0.0f;

            if (const auto bits = words[3] != 0 ? word_at_address(words[3] + 0x10) : std::nullopt) {
                std::memcpy(&cap, &*bits, 4);
            }

            output.shrub_cap = cap;

            // Keep offscreen shrubs resident while respecting authored hiding and far distance.
            for (std::size_t i = 0; i < counts[1]; ++i) {
                const std::uint8_t* r = ram.data() + at + i * 0x20;
                std::memcpy(output.scenery.shrub_spheres[i].data(), r, 16);
                std::memcpy(&output.scenery.shrub_lights[i], r + 0x1E, 2);
                float own;
                std::memcpy(&own, r + 0x10, 4);
                const float reach = std::min(own, cap);
                const bool far =
                    cap > 0.0f && beyond(output.scenery.shrub_spheres[i].data(), reach);
                output.scenery.shrub_hidden[i] = (r[0x16] & 1) || far;
            }
        }
    }
};

}  // namespace

CapturedScenery capture_scenery(
    std::span<const std::uint8_t> ram,
    const std::array<std::uint32_t, 4>& words,
    const host::GameState& state,
    float scale,
    const std::array<std::size_t, 2>& counts,
    int words_level,
    std::uint32_t tie_cap_after
) {
    Capture capture{ram, words, state, scale, counts, words_level, tie_cap_after, {}};
    capture.matrices();
    capture.read_ties();
    capture.read_terrain();
    capture.read_shrubs();
    return std::move(capture.output);
}

}  // namespace openrac::frontend
