// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Sharper pictures of parts of the 2D layer's textures (sharp_cells.h).

#include "sharp_cells.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>

namespace openrac::renderer::sharp_cells {
namespace {

// One owner's pages, never empty, shared with a picture() that reads them.
struct Owned {
    Owner owner = kBuiltIn;
    std::shared_ptr<const std::vector<Page>> pages;
};

std::mutex g_lock;
// The built-ins' first, then the mods' in the order they first set pages.
std::vector<Owned> g_owners;

bool matches(const Page& page, int width, int height, const std::uint8_t* rgba) {
    if (page.width != width || page.height != height || page.cells.empty()) {
        return false;
    }
    for (const Cell& c : page.cells) {
        if (c.x < 0 || c.y < 0 || c.x + c.w > width || c.y + c.h > height
            || c.rgb.size() != static_cast<std::size_t>(c.w * c.h * 3)) {
            return false;
        }
        for (int y = 0; y < c.h; ++y) {
            for (int x = 0; x < c.w; ++x) {
                const std::uint8_t* t =
                    rgba
                    + (static_cast<std::size_t>(c.y + y) * static_cast<std::size_t>(width)
                       + static_cast<std::size_t>(c.x + x))
                          * 4;
                const std::uint8_t* want = c.rgb.data() + static_cast<std::size_t>(y * c.w + x) * 3;
                if (t[0] != want[0] || t[1] != want[1] || t[2] != want[2]) {
                    return false;
                }
            }
        }
    }
    return true;
}

// The page's cells' own pictures put in `out`, the texture `s` times as big.
void draw_cells(const Page& page, int s, Picture& out) {
    for (const Cell& c : page.cells) {
        if (c.sharp.size() != static_cast<std::size_t>(c.w * s * c.h * s * 3)) {
            continue;
        }
        for (int y = 0; y < c.h * s; ++y) {
            for (int x = 0; x < c.w * s; ++x) {
                const std::uint8_t* from =
                    c.sharp.data() + static_cast<std::size_t>(y * c.w * s + x) * 3;
                std::uint8_t* o =
                    out.pixels.data()
                    + (static_cast<std::size_t>(c.y * s + y) * static_cast<std::size_t>(out.width)
                       + static_cast<std::size_t>(c.x * s + x))
                          * 4;
                o[0] = from[0];
                o[1] = from[1];
                o[2] = from[2];
            }
        }
    }
}

}  // namespace

void set(Owner owner, std::vector<Page> pages) {
    std::shared_ptr<const std::vector<Page>> kept;
    if (!pages.empty()) {
        kept = std::make_shared<const std::vector<Page>>(std::move(pages));
    }
    // The pages replaced, freed after the lock.
    std::shared_ptr<const std::vector<Page>> old;
    const std::lock_guard guard(g_lock);
    const auto found = std::find_if(g_owners.begin(), g_owners.end(), [&](const Owned& o) {
        return o.owner == owner;
    });
    if (found != g_owners.end()) {
        old = std::move(found->pages);
        if (kept) {
            found->pages = std::move(kept);
        } else {
            g_owners.erase(found);
        }
    } else if (kept) {
        g_owners.insert(owner == kBuiltIn ? g_owners.begin() : g_owners.end(), {owner, kept});
    }
}

void set(std::vector<Page> pages) {
    set(kBuiltIn, std::move(pages));
}

bool picture(int width, int height, const std::uint8_t* rgba, Picture& out) {
    std::vector<std::shared_ptr<const std::vector<Page>>> owned;
    {
        const std::lock_guard guard(g_lock);
        owned.reserve(g_owners.size());
        for (const Owned& o : g_owners) {
            owned.push_back(o.pages);
        }
    }
    // The matching pages, first first: the first sets the scale, the others of that scale join.
    std::vector<const Page*> found;
    int s = 0;
    for (const auto& pages : owned) {
        for (const Page& page : *pages) {
            const int scale = std::clamp(page.scale, 1, 16);
            if ((s == 0 || scale == s) && matches(page, width, height, rgba)) {
                s = scale;
                found.push_back(&page);
            }
        }
    }
    if (found.empty()) {
        return false;
    }
    out.width = width * s;
    out.height = height * s;
    out.pixels.resize(
        static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height) * 4
    );
    // Each column's two source columns and its weight, the same in every row (in 1/256ths).
    const auto taps = [s](int size,
                          int out,
                          std::vector<int>& first,
                          std::vector<int>& second,
                          std::vector<int>& weight) {
        first.resize(static_cast<std::size_t>(out));
        second.resize(static_cast<std::size_t>(out));
        weight.resize(static_cast<std::size_t>(out));
        for (int i = 0; i < out; ++i) {
            const float at = (static_cast<float>(i) + 0.5f) / static_cast<float>(s) - 0.5f;
            const int lo = static_cast<int>(std::floor(at));
            first[static_cast<std::size_t>(i)] = std::clamp(lo, 0, size - 1);
            second[static_cast<std::size_t>(i)] = std::clamp(lo + 1, 0, size - 1);
            weight[static_cast<std::size_t>(i)] =
                static_cast<int>(std::lround((at - static_cast<float>(lo)) * 256.0f));
        }
    };
    std::vector<int> x0, x1, wx, y0, y1, wy;
    taps(width, out.width, x0, x1, wx);
    taps(height, out.height, y0, y1, wy);
    // Every texel scaled up as the GPU's linear filter reads it, so the rest of the texture looks
    // as it did.
    const auto at = [&](int x, int y) {
        return rgba
               + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
                  + static_cast<std::size_t>(x))
                     * 4;
    };
    std::uint8_t* o = out.pixels.data();
    for (int y = 0; y < out.height; ++y) {
        const int ty = wy[static_cast<std::size_t>(y)];
        for (int x = 0; x < out.width; ++x, o += 4) {
            const int tx = wx[static_cast<std::size_t>(x)];
            const std::uint8_t* a =
                at(x0[static_cast<std::size_t>(x)], y0[static_cast<std::size_t>(y)]);
            const std::uint8_t* b =
                at(x1[static_cast<std::size_t>(x)], y0[static_cast<std::size_t>(y)]);
            const std::uint8_t* c =
                at(x0[static_cast<std::size_t>(x)], y1[static_cast<std::size_t>(y)]);
            const std::uint8_t* d =
                at(x1[static_cast<std::size_t>(x)], y1[static_cast<std::size_t>(y)]);
            for (int k = 0; k < 4; ++k) {
                const int top = a[k] * (256 - tx) + b[k] * tx;
                const int bottom = c[k] * (256 - tx) + d[k] * tx;
                o[k] = static_cast<std::uint8_t>((top * (256 - ty) + bottom * ty + 32768) >> 16);
            }
        }
    }
    // The cells' own pictures, over their colours; their opacity stays the texture's. The last
    // page's first, so an earlier page's are drawn over them.
    for (auto page = found.rbegin(); page != found.rend(); ++page) {
        draw_cells(**page, s, out);
    }
    return true;
}

}  // namespace openrac::renderer::sharp_cells
