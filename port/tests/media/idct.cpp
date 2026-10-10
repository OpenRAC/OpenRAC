// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/idct.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The inverse DCT against IEEE 1180-1990: the standard's random generator,
// its three input ranges with both signs, 10,000 blocks each, a double
// precision reference, and the standard's limits (peak error 1, per-pixel
// mean square error 0.06, overall 0.02, per-pixel mean error 0.015, overall
// 0.0015).

#include "media/idct.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numbers>

#include "tests/check.h"

using namespace openrac::media;

namespace {

using Table = std::array<std::array<double, 8>, 8>;
using Samples = std::array<double, 64>;

Table table() {
    Table t{};
    for (int k = 0; k < 8; ++k) {
        const double c = k == 0 ? 1.0 / std::numbers::sqrt2 : 1.0;
        for (int n = 0; n < 8; ++n) {
            t[k][n] = c / 2.0 * std::cos((2 * n + 1) * k * std::numbers::pi / 16.0);
        }
    }
    return t;
}

// F[v][u] = sum over y, x of f[y][x] T[u][x] T[v][y].
Samples forward_dct(const Table& t, const Samples& f) {
    Samples rows{};
    for (int y = 0; y < 8; ++y) {
        for (int u = 0; u < 8; ++u) {
            double s = 0;
            for (int x = 0; x < 8; ++x) {
                s += f[y * 8 + x] * t[u][x];
            }
            rows[y * 8 + u] = s;
        }
    }
    Samples out{};
    for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
            double s = 0;
            for (int y = 0; y < 8; ++y) {
                s += rows[y * 8 + u] * t[v][y];
            }
            out[v * 8 + u] = s;
        }
    }
    return out;
}

// f[y][x] = sum over v, u of F[v][u] T[u][x] T[v][y].
Samples reference_idct(const Table& t, const Samples& big_f) {
    Samples rows{};
    for (int v = 0; v < 8; ++v) {
        for (int x = 0; x < 8; ++x) {
            double s = 0;
            for (int u = 0; u < 8; ++u) {
                s += big_f[v * 8 + u] * t[u][x];
            }
            rows[v * 8 + x] = s;
        }
    }
    Samples out{};
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            double s = 0;
            for (int v = 0; v < 8; ++v) {
                s += rows[v * 8 + x] * t[v][y];
            }
            out[y * 8 + x] = s;
        }
    }
    return out;
}

// The IEEE 1180 generator.
struct Random {
    std::int64_t state = 1;

    std::int64_t next(std::int64_t low, std::int64_t high) {
        state = (state * 1103515245 + 12345) & 0xffff'ffff;
        const std::int64_t i = state & 0x7fff'fffe;
        const double x = static_cast<double>(i) / 0x7fff'ffff * static_cast<double>(low + high + 1);
        return static_cast<std::int64_t>(x) - low;
    }
};

void run(std::int64_t low, std::int64_t high, std::int64_t sign) {
    Random rng;
    const Table t = table();
    constexpr int kBlocks = 10'000;
    std::int64_t peak = 0;
    std::array<std::int64_t, 64> sum_error{};
    std::array<std::int64_t, 64> sum_square{};
    for (int n = 0; n < kBlocks; ++n) {
        Samples f{};
        for (double& v : f) {
            v = static_cast<double>(rng.next(low, high) * sign);
        }
        const Samples coefficients = forward_dct(t, f);
        Samples rounded{};
        std::array<std::int32_t, 64> test{};
        for (int k = 0; k < 64; ++k) {
            const auto c = std::clamp<std::int64_t>(std::llround(coefficients[k]), -2048, 2047);
            rounded[k] = static_cast<double>(c);
            test[k] = static_cast<std::int32_t>(c);
        }
        const Samples reference = reference_idct(t, rounded);
        inverse_dct(test);
        for (int k = 0; k < 64; ++k) {
            const auto ref = std::clamp<std::int64_t>(std::llround(reference[k]), -256, 255);
            const std::int64_t e = std::clamp<std::int64_t>(test[k], -256, 255) - ref;
            peak = std::max(peak, std::abs(e));
            sum_error[k] += e;
            sum_square[k] += e * e;
        }
    }
    double pmse = 0;
    double pme = 0;
    std::int64_t total_square = 0;
    std::int64_t total_error = 0;
    for (int k = 0; k < 64; ++k) {
        pmse = std::max(pmse, static_cast<double>(sum_square[k]) / kBlocks);
        pme = std::max(pme, std::abs(static_cast<double>(sum_error[k]) / kBlocks));
        total_square += sum_square[k];
        total_error += sum_error[k];
    }
    const double omse = static_cast<double>(total_square) / (64.0 * kBlocks);
    const double ome = std::abs(static_cast<double>(total_error) / (64.0 * kBlocks));
    std::printf(
        "IEEE 1180 L=%lld H=%lld sign=%lld: peak %lld, pmse %.5f, omse %.5f, pme %.5f, ome %.6f\n",
        static_cast<long long>(low),
        static_cast<long long>(high),
        static_cast<long long>(sign),
        static_cast<long long>(peak),
        pmse,
        omse,
        pme,
        ome
    );
    CHECK(peak <= 1);
    CHECK(pmse <= 0.06);
    CHECK(omse <= 0.02);
    CHECK(pme <= 0.015);
    CHECK(ome <= 0.0015);
}

}  // namespace

int main() {
    for (const auto& [low, high] : {std::pair{256, 255}, std::pair{5, 5}, std::pair{300, 300}}) {
        for (const int sign : {1, -1}) {
            run(low, high, sign);
        }
    }
    // Zero in, zero out; a DC coefficient alone is a flat block of DC / 8.
    std::array<std::int32_t, 64> z{};
    inverse_dct(z);
    CHECK(std::all_of(z.begin(), z.end(), [](std::int32_t v) { return v == 0; }));
    std::array<std::int32_t, 64> dc{};
    dc[0] = 800;
    inverse_dct(dc);
    CHECK(std::all_of(dc.begin(), dc.end(), [](std::int32_t v) { return v == 100; }));
    return openrac::test::result();
}
