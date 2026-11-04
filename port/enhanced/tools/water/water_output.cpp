// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Existing little-endian water field format, with validation before publication by the caller.
#include <bit>
#include <fstream>
#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water {
void write_word(std::ostream& output, std::uint32_t value) {
    std::array<char, 4> bytes{};

    // Explicit little-endian words keep output independent of host representation and aliasing.
    for (unsigned byte = 0; byte < 4; ++byte) {
        bytes[byte] = static_cast<char>((value >> (8 * byte)) & 255);
    }

    output.write(bytes.data(), bytes.size());
}

void write_float(std::ostream& output, double value) {
    write_word(output, std::bit_cast<std::uint32_t>(static_cast<float>(value)));
}

namespace {
// Emits cell and hydraulic sections without retaining another flattened atlas copy.
void write_cells(std::ostream& output, std::span<const Body> bodies) {
    // Cell body indices follow the stable physical-body order.
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        // Stored cells acquire their atlas body index only during writing.
        for (auto row : bodies[index].cells) {
            row[3] = static_cast<double>(index);

            // Each field texel keeps its original four-channel layout.
            for (double value : row) {
                write_float(output, value);
            }
        }
    }

    // Hydraulic data follows every body cell in one contiguous section.
    for (const auto& body : bodies) {
        // Hydraulic texels retain receiver order.
        for (const auto& row : body.hydraulics) {
            // Optical depth stays separate from the solver-depth channel.
            for (double value : row) {
                write_float(output, value);
            }
        }
    }
}
}  // namespace

void write_fields(
    const std::filesystem::path& path, std::span<const Body> bodies, const Input& input
) {
    const profiling::Zone timing("Write water fields");
    std::ofstream output(path, std::ios::binary);
    std::size_t cells = 0;

    // The complete atlas count precedes its fixed binary header.
    for (const auto& body : bodies) {
        cells += body.cells.size();
    }

    // Atlas limits are checked before writing any dependent offsets.
    if (cells > kAtlasCells) {
        throw std::runtime_error("water atlas exceeds budget");
    }

    output.write("ORWATER1", 8);
    write_word(output, 3);
    write_word(output, static_cast<std::uint32_t>(bodies.size()));
    write_word(output, static_cast<std::uint32_t>(cells));
    // Fingerprints are byte arrays rather than host numeric objects.
    output.write(reinterpret_cast<const char*>(input.fingerprint.data()), input.fingerprint.size());
    std::size_t offset = 0;

    // Metadata order retains the original body numbering.
    for (const auto& body : bodies) {
        auto metadata = body.metadata;
        metadata[3][3] = static_cast<double>(offset);

        // Each body keeps the existing five-texel metadata layout.
        for (const auto& row : metadata) {
            // All channels narrow at the same float32 format boundary.
            for (double value : row) {
                write_float(output, value);
            }
        }

        offset += body.cells.size();
    }

    write_cells(output, bodies);
    output.close();

    // Delayed writes must not be accepted as a completed water asset.
    if (!output) {
        throw std::runtime_error("cannot write water field output");
    }
}
}  // namespace openrac::lighting::water
