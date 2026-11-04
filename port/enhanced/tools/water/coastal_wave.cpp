// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline frequency-domain mild-slope solve for a single incoming ocean mode.
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <complex>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/water/water_bake.h"

namespace openrac::lighting::water {
namespace {
using Wave = std::complex<double>;
constexpr double kGravity = 9.81;  // Prototype treats one world unit as one metre.
constexpr double kTau = 2 * std::numbers::pi;
constexpr unsigned kMaximumSide = 96;  // Bounds the direct banded solve's storage and work.

// Surveyed patch with a finite offshore fallback for missing collision.
struct Patch {
    unsigned side;              // Square row-major grid.
    double cell;                // Cell spacing in world units.
    double offshore_depth;      // Depth at which the coastal loss must vanish.
    std::vector<double> depth;  // Zero represents a solid bank.
};

// Coefficients of div(c cg grad H) + k² c cg H = 0.
struct Medium {
    double k;     // Local wavenumber.
    double flux;  // Phase velocity times group velocity.
};

// Validates the patch schema before allocating the solver matrix.
Patch read_patch(const json::Value& document, double offshore_depth) {
    const auto side = document["size"].number();
    const auto cell = document["cell"].number();
    const auto& origin = document["origin"].items();
    // Eight cells are needed to leave an interior beyond the absorbing edges.
    if (!(side >= 8 && side <= kMaximumSide && std::floor(side) == side)
        || !(cell > 0 && std::isfinite(cell)) || origin.size() != 2 || !origin[0].is_number()
        || !origin[1].is_number() || !std::isfinite(origin[0].number())
        || !std::isfinite(origin[1].number())) {
        throw std::runtime_error("invalid coastal wave grid");
    }
    const auto& banks = document["bank"].items();
    const auto& depths = document["depth"].items();
    const auto count = static_cast<std::size_t>(side * side);
    // Every cell must have one bank flag and one depth value.
    if (banks.size() != count || depths.size() != count) {
        throw std::runtime_error("incomplete coastal wave grid");
    }
    Patch patch{static_cast<unsigned>(side), cell, offshore_depth, {}};
    patch.depth.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const double depth = depths[i].is_null() ? offshore_depth : depths[i].number();
        // Unknown wet depths use the offshore medium, while bank cells are impermeable.
        if (banks[i].type() != json::Value::Type::Bool
            || (!banks[i].boolean() && !(depth > 0 && std::isfinite(depth)))) {
            throw std::runtime_error("invalid coastal wave depth or bank");
        }
        patch.depth.push_back(banks[i].boolean() ? 0. : depth);
    }
    return patch;
}

// Solves the finite-depth dispersion relation, then evaluates phase and group velocity.
Medium medium(double depth, double omega) {
    const double deep_k = omega * omega / kGravity;
    double low = 0.;
    double high = deep_k + std::sqrt(deep_k / depth);
    // Bisection is monotonic across shallow and deep regimes.
    for (int step = 0; step < 48; ++step) {
        const double k = (low + high) * .5;
        if (k * std::tanh(k * depth) < deep_k) {
            low = k;
        } else {
            high = k;
        }
    }
    const double k = (low + high) * .5;
    const double twice_kh = 2 * k * depth;
    const double phase_speed = omega / k;
    // Beyond kh=20 the finite-depth correction is negligible and sinh can be skipped.
    const double correction = twice_kh < 40 ? twice_kh / std::sinh(twice_kh) : 0.;
    return {k, .5 * phase_speed * phase_speed * (1 + correction)};
}

// Stores only the diagonal and upper band of the complex symmetric stencil.
struct System {
    unsigned bandwidth;         // The Y neighbour is one grid row away.
    std::vector<Wave> upper;    // Each row starts with its diagonal.
    std::vector<Wave> forcing;  // Incident wave boundary data.

    // Accesses a coefficient inside the allocated band.
    Wave& at(std::size_t row, std::size_t column) {
        return upper[row * (bandwidth + 1) + column - row];
    }
};

// Eliminates the band in place. Positive damping regularizes the complex pivots.
std::vector<Wave> solve(System system) {
    auto& result = system.forcing;
    for (std::size_t row = 0; row < result.size(); ++row) {
        const auto end = std::min(result.size(), row + system.bandwidth + 1);
        const Wave pivot = system.at(row, row);
        // Reject a singular or nonfinite system before dividing.
        if (!(std::abs(pivot) > 1e-14) || !std::isfinite(std::abs(pivot))) {
            throw std::runtime_error("unstable coastal wave system");
        }
        for (std::size_t target = row + 1; target < end; ++target) {
            const Wave factor = system.at(row, target) / pivot;
            // A zero coupling requires no elimination.
            if (factor == Wave{}) {
                continue;
            }
            result[target] -= factor * result[row];
            for (std::size_t column = target; column < end; ++column) {
                system.at(target, column) -= factor * system.at(row, column);
            }
        }
    }
    for (std::size_t row = result.size(); row-- > 0;) {
        const auto end = std::min(result.size(), row + system.bandwidth + 1);
        for (std::size_t column = row + 1; column < end; ++column) {
            result[row] -= system.at(row, column) * result[column];
        }
        result[row] /= system.at(row, row);
        // Refuse to publish a response with invalid amplitudes.
        if (!std::isfinite(std::abs(result[row]))) {
            throw std::runtime_error("nonfinite coastal wave response");
        }
    }
    return std::move(result);
}

// Returns a normalized distance through the outer eighth of the patch.
double fringe_position(unsigned side, std::size_t row) {
    const auto x = static_cast<unsigned>(row % side);
    const auto y = static_cast<unsigned>(row / side);
    const auto edge = std::min({x, y, side - 1 - x, side - 1 - y});
    return std::min(1., edge / std::max(2., side / 8.));
}

// Assembles one flux-conserving row. Dry neighbours have zero normal flux.
void assemble_row(
    System& system,
    const Patch& patch,
    const std::vector<Medium>& media,
    std::size_t row,
    std::array<double, 2> direction,
    Medium offshore,
    bool flat
) {
    const auto here = media[row];
    // Dry cells retain a zero wave and do not couple to water.
    if (here.flux == 0) {
        system.at(row, row) = 1.;
        return;
    }
    const int side = static_cast<int>(patch.side);
    const int x = static_cast<int>(row % patch.side);
    const int y = static_cast<int>(row / patch.side);
    const auto incident = [&](int px, int py) {
        return std::polar(
            1., offshore.k * patch.cell * (direction[0] * (px + .5) + direction[1] * (py + .5))
        );
    };
    const double sponge = 1. - fringe_position(patch.side, row);
    const double mass = here.flux * std::pow(here.k * patch.cell, 2);
    // Weak interior loss regularizes enclosed cells. The fringe absorbs scattered waves.
    const Wave loss(0., mass * (.002 + 2. * sponge * sponge));
    auto& diagonal = system.at(row, row);
    // Approximate breaking losses in the shallow band without damping the flat reference.
    const double shallow_band = std::min(2.5, patch.offshore_depth);
    const double shallow = flat ? 0. : std::clamp(1. - patch.depth[row] / shallow_band, 0., 1.);
    diagonal = mass + loss + Wave(0., mass * .8 * shallow * shallow);
    system.forcing[row] = loss * incident(x, y);
    constexpr std::array<std::array<int, 2>, 4> kNeighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
    for (const auto& offset : kNeighbours) {
        const int nx = x + offset[0];
        const int ny = y + offset[1];
        // Ghost cells continue the incoming plane wave beyond the absorbing fringe.
        if (nx < 0 || ny < 0 || nx >= side || ny >= side) {
            diagonal -= here.flux;
            system.forcing[row] -= here.flux * incident(nx, ny);
            continue;
        }
        const auto neighbour = static_cast<std::size_t>(ny * side + nx);
        const double other = media[neighbour].flux;
        // A zero harmonic mean imposes the solid wall's zero normal flux.
        const double face = 2 * here.flux * other / (here.flux + other);
        diagonal -= face;
        if (neighbour > row) {
            system.at(row, neighbour) = face;
        }
    }
}

// Solves the same boundary problem with either surveyed or uniform bathymetry.
std::vector<Wave> response(
    const Patch& patch, double omega, std::array<double, 2> direction, Medium offshore, bool flat
) {
    const auto count = patch.depth.size();
    std::vector<Medium> media(count, offshore);
    for (std::size_t i = 0; i < count; ++i) {
        if (!flat) {
            media[i] = patch.depth[i] > 0 ? medium(patch.depth[i], omega) : Medium{};
        }
        // Four cells per local wavelength is the minimum supported by this prototype stencil.
        if (media[i].k * patch.cell > std::numbers::pi / 2) {
            throw std::runtime_error("coastal wave wavelength is too short for the patch grid");
        }
    }
    System system{
        patch.side, std::vector<Wave>(count * (patch.side + 1)), std::vector<Wave>(count)
    };
    for (std::size_t row = 0; row < count; ++row) {
        assemble_row(system, patch, media, row, direction, offshore, flat);
    }
    return solve(std::move(system));
}

// Real/imaginary pairs interpolate without a phase wrap at ±pi.
void write_response(std::ostream& out, const std::vector<Wave>& values, bool compact = false) {
    const auto write_float = [&](double value) {
        char buffer[48];
        const auto result = std::to_chars(buffer, buffer + sizeof(buffer), float(value));
        out.write(buffer, result.ptr - buffer);
    };
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) {
            out << ',';
        }
        if (compact && values[i] == Wave{}) {
            out << "null";
        } else if (compact) {
            out << '[';
            write_float(values[i].real());
            out << ',';
            write_float(values[i].imag());
            out << ']';
        } else {
            out << '[' << values[i].real() << ',' << values[i].imag() << ']';
        }
    }
    out << ']';
}

// Fades the difference into the offshore wave so the renderer can add it once.
std::vector<Wave> correction(
    const Patch& patch, const std::vector<Wave>& coast, const std::vector<Wave>& reference
) {
    std::vector<Wave> result(coast.size());
    for (std::size_t i = 0; i < result.size(); ++i) {
        // The water mesh masks solid banks independently of the wave correction.
        if (patch.depth[i] > 0) {
            const double t = fringe_position(patch.side, i);
            const double weight = t * t * (3. - 2. * t);  // Smoothstep across the overlap.
            result[i] = weight * (coast[i] - reference[i]);
        }
    }
    return result;
}
}  // namespace

Result<std::string> ocean_spectrum(const std::filesystem::path& path, double offshore_depth) {
    try {
        // Match the standalone bake's finite-depth limits.
        if (!(offshore_depth >= .1 && offshore_depth <= 10000)) {
            throw std::runtime_error("invalid offshore depth");
        }
        // Period in seconds, travel heading in degrees, and amplitude in world units.
        constexpr std::array<std::array<double, 3>, 2> kModes{{
            {3.2, 180., .075},
            {2.65, 155., .045},
        }};
        const auto document = bake_json::document(path);
        const auto patch = read_patch(document, offshore_depth);
        const double mean_height = document["mean_height"].number();
        if (patch.side != 64 || patch.cell != .5 || mean_height < 222.99 || mean_height > 223.01) {
            throw std::runtime_error("ocean spectrum requires a 64-cell Pokitaru runtime patch");
        }
        std::ostringstream out;
        out << std::setprecision(17)
            << "{\"version\":3,\"patch\":" << bake_json::json_text(document)
            << ",\"offshore_depth\":" << offshore_depth << ",\"modes\":[";
        for (std::size_t i = 0; i < kModes.size(); ++i) {
            const auto [period, heading, amplitude] = kModes[i];
            const double omega = kTau / period;
            const auto offshore = medium(offshore_depth, omega);
            if (offshore.k > 3) {
                throw std::runtime_error("offshore depth is too shallow for the ocean spectrum");
            }
            out << (i ? ",{" : "{") << "\"period\":" << period << ",\"heading\":" << heading
                << ",\"wavenumber\":" << offshore.k << ",\"amplitude\":" << amplitude;
            // Each response uses the precise period and heading stored with its offshore mode.
            const double angle = heading * kTau / 360.;
            const std::array direction{std::cos(angle), std::sin(angle)};
            const auto coast = response(patch, omega, direction, offshore, false);
            const auto reference = response(patch, omega, direction, offshore, true);
            out << ",\"correction\":";
            write_response(out, correction(patch, coast, reference), true);
            out << '}';
        }
        out << "]}";
        return out.str();
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Result<std::string> coastal_wave(
    const std::filesystem::path& path, double period, double heading, double offshore_depth
) {
    try {
        // Bounds avoid degenerate frequency and depth inputs to the dispersion solve.
        if (!(period >= .25 && period <= 60 && std::isfinite(heading) && offshore_depth >= .1
              && offshore_depth <= 10000)) {
            throw std::runtime_error("invalid coastal wave period, heading or offshore depth");
        }
        const auto document = bake_json::document(path);
        const auto patch = read_patch(document, offshore_depth);
        const auto omega = kTau / period;
        const auto angle = std::remainder(heading, 360.) * kTau / 360.;
        const std::array direction{std::cos(angle), std::sin(angle)};
        const auto offshore = medium(offshore_depth, omega);
        const auto coast = response(patch, omega, direction, offshore, false);
        const auto reference = response(patch, omega, direction, offshore, true);
        std::ostringstream out;
        out << std::setprecision(17)
            << "{\"version\":1,\"patch\":" << bake_json::json_text(document)
            << ",\"period\":" << period << ",\"heading\":" << heading
            << ",\"offshore_depth\":" << offshore_depth << ",\"wavenumber\":" << offshore.k
            << ",\"coast\":";
        write_response(out, coast);
        out << ",\"reference\":";
        write_response(out, reference);
        out << ",\"correction\":";
        write_response(out, correction(patch, coast, reference));
        out << '}';
        return out.str();
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting::water
