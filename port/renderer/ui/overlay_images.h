// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The pictures mods give for their overlay (openrac_api.overlay_image_create, interface 1.40):
// RGBA pictures kept in memory by id, each its owner's (a mod), until the owner destroys it or
// unloads. Plain memory and a lock: the mods' interface adds and takes them on any thread, and
// OverlayDraw reads them on the graphics thread to make its textures. A library of its own, so the
// mods' interface builds without a window.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace openrac::renderer::ui::overlay_images {

// A picture's largest side, in pixels.
constexpr int kSideMax = 4096;
// The most pictures an owner keeps, and the most bytes of them.
constexpr std::size_t kPerOwnerMax = 256;
constexpr std::size_t kBytesPerOwnerMax = std::size_t{128} << 20;

// One picture: `width` x `height` pixels, R G B A bytes, top row first, straight alpha.
struct Picture {
    int width = 0;
    int height = 0;
    // Sampled smoothly (bilinear, with mipmaps); otherwise the nearest pixel.
    bool smooth = true;
    std::vector<std::uint8_t> rgba;
};

// Why create refused.
enum class Refusal {
    None,
    Bad,   // no owner, a side out of 1..kSideMax, or no pixels
    Full,  // the owner keeps kPerOwnerMax pictures or kBytesPerOwnerMax bytes already
};

// Keeps a copy of `rgba` (width * height * 4 bytes) as `owner`'s picture. Its id (1 up, never given
// again in this run), or 0 with `why` set.
int create(
    const void* owner,
    int width,
    int height,
    std::span<const std::uint8_t> rgba,
    bool smooth,
    Refusal* why = nullptr
);

// Lets go of picture `id` if `owner` has it. False for an id that is not the owner's.
bool destroy(const void* owner, int id);

// Lets go of all of `owner`'s pictures (its unload).
void drop(const void* owner);

// Picture `id`, null when there is none (never made, or let go). The picture stays valid for
// whoever holds it.
std::shared_ptr<const Picture> find(int id);

// Whether `owner` has picture `id`: only its own picture is drawn for a mod.
bool owns(const void* owner, int id);

// How many pictures there are, and how many changes (creations and let-gos) so far: a texture
// cache sweeps its textures when the count of changes moves.
std::size_t count();
std::uint64_t changes();

}  // namespace openrac::renderer::ui::overlay_images
