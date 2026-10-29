// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements reflection capture replacement for environment_probe_merge_internal.h.

#include <algorithm>
#include <iterator>
#include <limits>
#include <numeric>

#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/environment_probe_merge_internal.h"

namespace openrac::lighting::bake_detail {

bool similar_environment_capture_light(Float3 a, Float3 b, const EnvironmentBakeSettings& options) {
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(a[axis]) || !std::isfinite(b[axis]) || a[axis] < 0 || b[axis] < 0) {
            return false;
        }
    }

    const auto luminance = [](Float3 c) {
        return .2126f * c[0] + .7152f * c[1] + .0722f * c[2];
    };
    const float ya = luminance(a), yb = luminance(b);

    // The small absolute allowance avoids unstable ratios near black, not bright/dark room joins.
    if (std::max(ya, yb) > options.merge_brightness_ratio * std::min(ya, yb) + .02f) {
        return false;
    }

    if (std::max(ya, yb) <= .002f) {
        return true;
    }

    // Compare hue/chroma at a common 18% luminance, independently of HDR exposure.
    // Oklab matrices: https://bottosson.github.io/posts/oklab/ (public domain).
    const auto lab = [](Float3 c, float y) {
        c = mul(c, .18f / std::max(y, .002f));
        const float l = std::cbrt(.4122214708f * c[0] + .5363325363f * c[1] + .0514459929f * c[2]);
        const float m = std::cbrt(.2119034982f * c[0] + .6806995451f * c[1] + .1073969566f * c[2]);
        const float s = std::cbrt(.0883024619f * c[0] + .2817188376f * c[1] + .6299787005f * c[2]);
        return Float3{
            .2104542553f * l + .7936177850f * m - .0040720468f * s,
            1.9779984951f * l - 2.4285922050f * m + .4505937099f * s,
            .0259040371f * l + .7827717662f * m - .8086757660f * s
        };
    };
    const auto d = add(lab(a, ya), mul(lab(b, yb), -1));
    return dot(d, d) <= options.merge_colour_distance * options.merge_colour_distance;
}

bool environment_merge_loss_allowed(
    std::span<const unsigned> misses,
    std::span<const unsigned> newly_lost,
    const EnvironmentMergeCoverage& coverage
) {
    if (misses.size() > kGlobalMissLimit || newly_lost.size() > coverage.options.merge_max_misses
        || (!coverage.spacing_merge
            && area_sum(newly_lost, coverage.floors)
                   > coverage.options.merge_loss_fraction * coverage.target_area + 1e-5f)
        || area_sum(misses, coverage.floors) > kGlobalLossFraction * coverage.total_area + 1e-5f) {
        return false;
    }

    for (unsigned i = 0; i < misses.size(); ++i) {
        const auto& a = coverage.receivers[misses[i]];

        if (a.height != 0) {
            return false;
        }

        for (unsigned j = 0; j < i; ++j) {
            const auto d = add(a.position, mul(coverage.receivers[misses[j]].position, -1));

            if (std::abs(d[2]) <= 2 && d[0] * d[0] + d[1] * d[1] <= 6.25f) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace openrac::lighting::bake_detail
