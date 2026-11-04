// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Declarative masked tile tables; edge-connected patches form separate physical pond bodies.
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "enhanced/tools/water/water_source_internal.h"

namespace openrac::lighting::water::source {
namespace {
using Square = std::array<Point, 4>;

// One parsed tile layout, independent of source addresses or game identity.
struct Layout {
    std::size_t address;           // First origin record in the retained image.
    std::size_t count;             // Number of origin records.
    std::size_t stride;            // Byte distance between origins.
    std::size_t masks;             // First authored mask word.
    std::size_t mask_stride;       // Byte distance between mask words.
    unsigned columns;              // Mask bits per row.
    unsigned rows;                 // Mask rows, with at most 32 bits in total.
    std::array<double, 2> offset;  // First tile's offset from its patch origin.
    std::array<double, 2> step;    // Positive tile dimensions in world units.
};

// Parses only fields needed to expand rectangular tile bits.
Layout layout(const Image& image, const json::Value& table) {
    keys(
        table,
        {"kind",
         "source",
         "address",
         "count",
         "record_stride",
         "mask_address",
         "mask_stride",
         "columns",
         "rows",
         "offset",
         "step"}
    );
    const auto columns = integer(table["columns"], 32);
    const auto rows = integer(table["rows"], 32);

    // Offsets and dimensions have exactly two independently configured coordinates.
    if (!columns || !rows || columns * rows > 32 || table["offset"].items().size() != 2
        || table["step"].items().size() != 2) {
        throw std::runtime_error("invalid water source tile dimensions");
    }

    Layout result{
        integer(table["address"], image.bytes.size()),
        integer(table["count"], 4096),
        integer(table["record_stride"], 65'536),
        integer(table["mask_address"], image.bytes.size()),
        integer(table["mask_stride"], 4096),
        static_cast<unsigned>(columns),
        static_cast<unsigned>(rows),
        {scalar(table["offset"][0]), scalar(table["offset"][1])},
        {scalar(table["step"][0]), scalar(table["step"][1])}
    };

    // Empty or overlapping record layouts cannot define a stable patch sequence.
    if (!result.count || result.stride < 3 * sizeof(float)
        || result.mask_stride < sizeof(std::uint32_t) || result.step[0] <= 0
        || result.step[1] <= 0) {
        throw std::runtime_error("invalid water source tile table");
    }

    return result;
}

// Expands a single authored mask, retaining increasing bit order.
void patch(
    std::vector<Square>& squares, const Image& image, const Layout& layout, std::size_t index
) {
    const auto address = layout.address + index * layout.stride;
    const Point origin{image.scalar(address), image.scalar(address + 4), image.scalar(address + 8)};
    const auto mask = image.word(layout.masks + index * layout.mask_stride);

    // Only configured bits represent tiles; other source flags have no geometry meaning.
    for (unsigned bit = 0; bit < layout.columns * layout.rows; ++bit) {
        // Unset mask bits do not contain authored water geometry.
        if ((mask & (std::uint32_t{1} << bit)) == 0) {
            continue;
        }

        // Integer mask coordinates select a row before conversion to world-space distance.
        const auto row = bit / layout.columns;
        const double x = origin[0] + layout.offset[0] + (bit % layout.columns) * layout.step[0];
        const double y = origin[1] + layout.offset[1] + row * layout.step[1];
        squares.push_back(
            {Point{x, y, origin[2]},
             Point{x + layout.step[0], y, origin[2]},
             Point{x, y + layout.step[1], origin[2]},
             Point{x + layout.step[0], y + layout.step[1], origin[2]}}
        );

        // Bound graph work independently of a profile's record and mask dimensions.
        if (squares.size() > 4096) {
            throw std::runtime_error("water source tile budget exceeded");
        }
    }
}

// Matches only shared edges at the same elevation; corner contacts cannot join different bodies.
bool adjacent(const Square& square, const Square& other) {
    // Height separation protects distinct storeys with overlapping XY coverage.
    if (std::abs(square[0][2] - other[0][2]) >= .05) {
        return false;
    }

    unsigned shared = 0;

    // Exact source coordinates retain the reference's shared-corner equality semantics.
    for (const auto& point : square) {
        shared += std::find(other.begin(), other.end(), point) != other.end();
    }

    return shared >= 2;
}

// Tests one square against a connected component with an explicit worst-case work budget.
bool touches(const Square& square, const std::vector<Square>& group, std::size_t& work) {
    // Stable group order preserves deterministic art-triangle ordering.
    for (const auto& other : group) {
        // Pathological disconnected tiles must not consume unbounded connectivity work.
        if (++work > 16'777'216) {
            throw std::runtime_error("water source tile connectivity budget exceeded");
        }

        // A shared edge joins the square to this physical body.
        if (adjacent(square, other)) {
            return true;
        }
    }

    return false;
}

// Removes one stable edge-connected component from the remaining tiles.
std::vector<Square> component(std::vector<Square>& squares, std::size_t& work) {
    std::vector<Square> group{squares.back()};
    squares.pop_back();
    bool changed = true;

    // Repeated forward scans match the reference component ordering, bounded by tile removals.
    while (changed) {
        changed = false;

        // Forward scans preserve reference ordering while each match removes one tile.
        for (std::size_t index = 0; index < squares.size();) {
            // Only edge-connected squares may extend the current component.
            if (touches(squares[index], group, work)) {
                group.push_back(squares[index]);
                squares.erase(squares.begin() + static_cast<std::ptrdiff_t>(index));
                changed = true;
            } else {
                ++index;
            }
        }
    }

    return group;
}
}  // namespace

std::vector<Surface> tiles(const Image& image, const json::Value& table) {
    const auto config = layout(image, table);
    const auto source = static_cast<unsigned>(integer(table["source"], 65'535));
    std::vector<Square> squares;

    // Source patch order and mask order jointly determine component triangle order.
    for (std::size_t index = 0; index < config.count; ++index) {
        patch(squares, image, config, index);
    }

    // An empty mask table usually identifies an incorrect profile or an uninitialised source.
    if (squares.empty()) {
        throw std::runtime_error("water source tile table has no active tiles");
    }

    std::vector<Surface> result;
    std::size_t work = 0;

    // Components remain distinct even when they share a runtime source identifier.
    while (!squares.empty()) {
        Surface surface{source, true, false, {}, {}};

        // Each rectangle uses the same two-triangle diagonal as its authored water patch.
        for (const auto& square : component(squares, work)) {
            surface.triangles.push_back({square[0], square[1], square[2]});
            surface.triangles.push_back({square[2], square[1], square[3]});
        }

        result.push_back(std::move(surface));

        // Disconnected patches cannot exceed the runtime body limit.
        if (result.size() > 64) {
            throw std::runtime_error("water source body budget exceeded");
        }
    }

    return result;
}
}  // namespace openrac::lighting::water::source
