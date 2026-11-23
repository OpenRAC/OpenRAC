// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Heuristic multi-start OBB fitting, restricted to yaw/pitch (no roll).
#include "enhanced/lighting/probe_fit.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

namespace openrac::lighting {
namespace {
constexpr double radians = std::numbers::pi / 180;
using Angles = std::array<double, 3>;

Angles canonical(Angles a, bool pitch, bool roll = false) {
    a[2] = roll ? std::remainder(a[2], 180.) : 0;
    if (roll) {
        a[0] = std::remainder(a[0], 360.);
        a[1] = pitch ? std::remainder(a[1], 360.) : 0;
        return a;
    }
    a[0] = std::remainder(a[0], 360.);
    a[1] = pitch ? std::remainder(a[1], 180.) : 0;
    if (a[0] < -90) {
        a[0] += 180;
        a[1] = -a[1];
    }
    if (a[0] >= 90) {
        a[0] -= 180;
        a[1] = -a[1];
    }
    return a;
}

ProbeFrame frame(Angles a) {
    return {
        {float(std::cos(a[0] * radians)), float(std::sin(a[0] * radians))},
        {float(std::cos(a[1] * radians)), float(std::sin(a[1] * radians))},
        {float(std::cos(a[2] * radians)), float(std::sin(a[2] * radians))}
    };
}

Angles angles(ProbeFrame f) {
    return {
        std::atan2(f.axis[1], f.axis[0]) / radians,
        std::atan2(f.pitch[1], f.pitch[0]) / radians,
        std::atan2(f.roll[1], f.roll[0]) / radians
    };
}

double cross(Float3 a, Float3 b, Float3 c) {
    return double(b[0] - a[0]) * (c[1] - a[1]) - double(b[1] - a[1]) * (c[0] - a[0]);
}

std::vector<Angles> guesses(std::span<const Float3> points) {
    std::vector<Angles> result;
    const auto direction = [&](Float3 d) {
        if (double(d[0]) * d[0] + double(d[1]) * d[1] + double(d[2]) * d[2] > 1e-10) {
            const double yaw = std::atan2(d[1], d[0]) / radians;
            result.push_back({yaw, std::atan2(d[2], std::hypot(d[0], d[1])) / radians});
            result.push_back({yaw, 0});
        }
    };
    if (points.size() < 2) {
        return result;
    }
    // Covariance uses centred doubles so translation and distant world origins
    // cannot alter the guess. Jacobi supplies all principal directions.
    double mean[3]{}, covariance[3][3]{}, vectors[3][3]{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for (auto p : points) {
        for (unsigned a = 0; a < 3; ++a) {
            mean[a] += double(p[a]) / double(points.size());
        }
    }
    for (auto p : points) {
        for (unsigned a = 0; a < 3; ++a) {
            for (unsigned b = 0; b < 3; ++b) {
                covariance[a][b] += (p[a] - mean[a]) * (p[b] - mean[b]);
            }
        }
    }
    for (unsigned n = 0; n < 24; ++n) {
        unsigned a = 0, b = 1;
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = i + 1; j < 3; ++j) {
                if (std::abs(covariance[i][j]) > std::abs(covariance[a][b])) {
                    a = i;
                    b = j;
                }
            }
        }
        if (std::abs(covariance[a][b]) < 1e-12) {
            break;
        }
        const double theta =
            .5 * std::atan2(2 * covariance[a][b], covariance[b][b] - covariance[a][a]);
        const double c = std::cos(theta), s = std::sin(theta);
        for (unsigned k = 0; k < 3; ++k) {
            const double x = covariance[k][a], y = covariance[k][b];
            covariance[k][a] = c * x - s * y;
            covariance[k][b] = s * x + c * y;
            const double vx = vectors[k][a], vy = vectors[k][b];
            vectors[k][a] = c * vx - s * vy;
            vectors[k][b] = s * vx + c * vy;
        }
        for (unsigned k = 0; k < 3; ++k) {
            const double x = covariance[a][k], y = covariance[b][k];
            covariance[a][k] = c * x - s * y;
            covariance[b][k] = s * x + c * y;
        }
    }
    for (unsigned a = 0; a < 3; ++a) {
        direction({float(vectors[0][a]), float(vectors[1][a]), float(vectors[2][a])});
    }
    // An approximate diameter is a useful independent seed on curved or uneven
    // corridors where covariance is biased by sample density.
    auto farthest = [&](Float3 from) {
        auto best = points.front();
        double distance = -1;
        for (auto p : points) {
            double d = 0;
            for (unsigned a = 0; a < 3; ++a) {
                d += double(p[a] - from[a]) * (p[a] - from[a]);
            }
            if (d > distance) {
                distance = d;
                best = p;
            }
        }
        return best;
    };
    const auto a = farthest(points.front()), b = farthest(a);
    direction({b[0] - a[0], b[1] - a[1], b[2] - a[2]});
    // Monotone-chain XY hull: boundary directions are better guesses than a
    // sampled angular grid. Keep low Z for repeated XY floor/ceiling columns.
    std::vector<Float3> sorted(points.begin(), points.end());
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(
        std::unique(
            sorted.begin(),
            sorted.end(),
            [](auto a, auto b) { return a[0] == b[0] && a[1] == b[1]; }
        ),
        sorted.end()
    );
    std::vector<Float3> hull;
    for (auto p : sorted) {
        while (hull.size() > 1 && cross(hull[hull.size() - 2], hull.back(), p) <= 0) {
            hull.pop_back();
        }
        hull.push_back(p);
    }
    const auto lower = hull.size();
    for (auto i = sorted.rbegin() + 1; i != sorted.rend(); ++i) {
        while (hull.size() > lower && cross(hull[hull.size() - 2], hull.back(), *i) <= 0) {
            hull.pop_back();
        }
        hull.push_back(*i);
    }
    std::vector<std::pair<double, Float3>> edges;
    for (unsigned i = 1; i < hull.size(); ++i) {
        Float3 d{
            hull[i][0] - hull[i - 1][0], hull[i][1] - hull[i - 1][1], hull[i][2] - hull[i - 1][2]
        };
        edges.push_back({double(d[0]) * d[0] + double(d[1]) * d[1], d});
    }
    std::stable_sort(edges.begin(), edges.end(), [](auto a, auto b) { return a.first > b.first; });
    for (unsigned i = 0; i < std::min<std::size_t>(16, edges.size()); ++i) {
        direction(edges[i].second);
    }
    return result;
}
}  // namespace

ProbeFrame search_probe_frame(
    std::span<const Float3> points,
    const std::function<double(ProbeFrame)>& score,
    std::span<const ProbeFrame> seeds,
    bool allow_pitch,
    bool allow_roll
) {
    struct Trial {
        Angles angles;
        double value;
    };

    std::map<std::array<int, 3>, double> cache;
    std::vector<Trial> trials;
    Trial best{{0, 0}, INFINITY};
    const auto evaluate = [&](Angles a) {
        a = canonical(a, allow_pitch, allow_roll);
        const std::array<int, 3> key{
            int(std::lround(a[0] * 10000)),
            int(std::lround(a[1] * 10000)),
            int(std::lround(a[2] * 10000))
        };
        if (const auto found = cache.find(key); found != cache.end()) {
            return Trial{a, found->second};
        }
        const double value = score(frame(a));
        cache.emplace(key, value);
        Trial trial{a, value};
        trials.push_back(trial);
        if (value < best.value - 1e-9) {
            best = trial;
        }
        return trial;
    };
    evaluate({0, 0});
    for (auto seed : seeds) {
        evaluate(angles(seed));
    }
    const auto heuristic_guesses = guesses(points);
    for (auto guess : heuristic_guesses) {
        evaluate(guess);
    }
    for (int yaw = -90; yaw < 90; yaw += 15) {
        for (int pitch = allow_pitch ? -90 : 0; pitch <= (allow_pitch ? 75 : 0); pitch += 15) {
            evaluate({double(yaw), double(pitch)});
        }
    }
    if (allow_roll) {
        auto rolls = heuristic_guesses;
        auto ranked = trials;
        std::stable_sort(ranked.begin(), ranked.end(), [](auto a, auto b) {
            return a.value < b.value;
        });
        for (unsigned i = 0; i < std::min<std::size_t>(6, ranked.size()); ++i) {
            rolls.push_back(ranked[i].angles);
        }
        for (auto a : rolls) {
            for (int r = -90; r < 90; r += 15) {
                a[2] = r;
                evaluate(a);
            }
        }
    }
    std::stable_sort(trials.begin(), trials.end(), [](auto a, auto b) {
        return a.value < b.value;
    });
    std::vector<Trial> starts;
    for (auto trial : trials) {
        if (!std::isfinite(trial.value)) {
            continue;
        }
        const bool diverse = std::none_of(starts.begin(), starts.end(), [&](auto start) {
            auto d = canonical(
                {trial.angles[0] - start.angles[0],
                 trial.angles[1] - start.angles[1],
                 trial.angles[2] - start.angles[2]},
                allow_pitch,
                allow_roll
            );
            return std::hypot(d[0], d[1], d[2]) < 10;
        });
        if (diverse) {
            starts.push_back(trial);
        }
        if (starts.size() == 6) {
            break;
        }
    }
    // Pattern search handles non-differentiable hull supports and integer grid
    // dimensions. Multiple basins avoid committing to a single PCA/local guess.
    for (auto current : starts) {
        for (double step : {5., 1., .2, .05}) {
            for (unsigned iteration = 0; iteration < 16; ++iteration) {
                auto next = current;
                for (int y = -1; y <= 1; ++y) {
                    for (int p = allow_pitch ? -1 : 0; p <= (allow_pitch ? 1 : 0); ++p) {
                        for (int r = allow_roll ? -1 : 0; r <= (allow_roll ? 1 : 0); ++r) {
                            if (!y && !p && !r) {
                                continue;
                            }
                            const auto trial = evaluate(
                                {current.angles[0] + y * step,
                                 current.angles[1] + p * step,
                                 current.angles[2] + r * step}
                            );
                            if (trial.value < next.value - 1e-9) {
                                next = trial;
                            }
                        }
                    }
                }
                if (next.value >= current.value - 1e-9) {
                    break;
                }
                current = next;
            }
        }
    }
    return frame(best.angles);
}

ProbeFrame fit_probe_frame(std::span<const Float3> points) {
    if (points.size() < 3) {
        return {};
    }
    return search_probe_frame(points, [&](ProbeFrame f) {
        Float3 lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
        for (auto p : points) {
            for (unsigned a = 0; a < 3; ++a) {
                p[a] -= points.front()[a];
            }
            p = f.to_local(p);
            for (unsigned a = 0; a < 3; ++a) {
                lo[a] = std::min(lo[a], p[a]);
                hi[a] = std::max(hi[a], p[a]);
            }
        }
        return double(std::max(.1f, hi[0] - lo[0])) * std::max(.1f, hi[1] - lo[1])
               * std::max(.1f, hi[2] - lo[2]);
    });
}
}  // namespace openrac::lighting
