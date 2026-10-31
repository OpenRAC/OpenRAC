// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Pointer-based vertex strips with explicit field layouts, transforms and rest-height adjustments.
#include <cmath>
#include <stdexcept>

#include "enhanced/tools/water/water_source_internal.h"

namespace openrac::lighting::water::source {
namespace {
// One parsed table layout, reused without per-vertex JSON lookups.
struct Layout {
    std::size_t stride;                  // Byte distance between vertex records.
    std::array<std::size_t, 3> offsets;  // Float32 XYZ component byte offsets.
    bool average;        // Height averages the configured second endpoint before its offset.
    std::size_t second;  // Second height byte offset when average is enabled.
    double height;       // Rest-height offset, rounded to float32 like authored initialisation.
    Point scale;         // Per-axis scale after rest-height decoding.
    Point translation;   // Per-axis world-space translation.
};

// Validates component positions before any source pointer is followed.
Layout layout(const json::Value& table) {
    const auto stride = integer(table["vertex_stride"], 4096);
    (void)point(table["position_offsets"]);
    Layout result{stride, {}, false, 0, 0, {1, 1, 1}, {}};

    // Component offsets must denote complete float32 values inside each vertex record.
    for (unsigned axis = 0; axis < 3; ++axis) {
        result.offsets[axis] = integer(table["position_offsets"][axis], stride);

        // Every component must fit inside its vertex record.
        if (stride < sizeof(float) || result.offsets[axis] > stride - sizeof(float)) {
            throw std::runtime_error("water source component exceeds vertex stride");
        }
    }

    // A second height component models art records with two authored endpoints.
    if (!table["second_height_offset"].is_null()) {
        result.average = true;
        result.second = integer(table["second_height_offset"], stride - sizeof(float));
    }

    // Missing transforms retain the source's world-space geometry.
    if (!table["height_offset"].is_null()) {
        result.height = scalar(table["height_offset"]);
    }

    // Unspecified scale preserves the authored dimensions.
    if (!table["scale"].is_null()) {
        result.scale = point(table["scale"]);
    }

    // Unspecified translation preserves the authored world origin.
    if (!table["translation"].is_null()) {
        result.translation = point(table["translation"]);
    }

    return result;
}

// Decodes one rest vertex, narrowing only the configured initialisation offset to source precision.
Point vertex(const Image& image, std::size_t at, const Layout& layout) {
    Point result{};

    // Source component order is explicit rather than inferred from the game's identity.
    for (unsigned axis = 0; axis < 3; ++axis) {
        result[axis] = image.scalar(at + layout.offsets[axis]);
    }

    // Endpoint averaging precedes height adjustment, matching the declared record semantics.
    if (layout.average) {
        result[2] = (result[2] + image.scalar(at + layout.second)) * .5;
    }

    // Source initialisation stores adjusted heights as float32 before rendering or baking.
    if (layout.height != 0) {
        result[2] = static_cast<float>(result[2] + layout.height);
    }

    // Table transforms apply consistently to all decoded surfaces.
    for (unsigned axis = 0; axis < 3; ++axis) {
        result[axis] = result[axis] * layout.scale[axis] + layout.translation[axis];

        // Transforms cannot bypass the coordinate limits applied to raw source values.
        if (!std::isfinite(result[axis]) || std::abs(result[axis]) >= 100'000) {
            throw std::runtime_error("water source transform exceeds coordinate limits");
        }
    }

    return result;
}

// Rejects degenerate art triangles using the reference's squared cross-product tolerance.
bool has_area(const Triangle& triangle) {
    Point a{}, b{};

    // Winding and authored normals do not affect two-sided water geometry.
    for (unsigned axis = 0; axis < 3; ++axis) {
        a[axis] = triangle[1][axis] - triangle[0][axis];
        b[axis] = triangle[2][axis] - triangle[0][axis];
    }

    const Point cross{
        a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]
    };
    return cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2] > 1e-8;
}

// Appends one bounded strip in its original consecutive-triangle order.
void append_strip(
    Surface& result,
    const Image& image,
    std::size_t record,
    const json::Value& table,
    const Layout& layout
) {
    const auto pointer = image.word(record + integer(table["pointer_offset"], 4096));
    const auto count = image.word(record + integer(table["count_offset"], 4096));

    // A strip cannot allocate arbitrary geometry from a corrupt source count.
    if (count < 3 || count > 4096) {
        throw std::runtime_error("invalid water source strip vertex count");
    }

    image.check(pointer, count * layout.stride);
    Point first = vertex(image, pointer, layout);
    Point second = vertex(image, pointer + layout.stride, layout);

    // Consecutive strip triangles retain original order without trusting face normals.
    for (std::size_t index = 2; index < count; ++index) {
        const auto third = vertex(image, pointer + index * layout.stride, layout);
        const Triangle triangle{first, second, third};

        // Degenerate strip triangles are art padding rather than physical surfaces.
        if (has_area(triangle)) {
            result.triangles.push_back(triangle);
        }

        first = second;
        second = third;
    }
}
}  // namespace

Surface strips(const Image& image, const json::Value& table) {
    keys(
        table,
        {"kind",
         "source",
         "flowing",
         "flow",
         "address",
         "count",
         "record_stride",
         "pointer_offset",
         "count_offset",
         "vertex_stride",
         "position_offsets",
         "second_height_offset",
         "height_offset",
         "scale",
         "translation"}
    );
    const auto address = integer(table["address"], image.bytes.size());
    const auto count = integer(table["count"], 64);
    const auto stride = integer(table["record_stride"], 4096);
    const auto vertex_layout = layout(table);

    // Whole record spans are validated before following any embedded pointer.
    if (!count || !stride || table["flowing"].type() != json::Value::Type::Bool) {
        throw std::runtime_error("invalid water source strip table");
    }

    // Embedded pointer and count fields must fit their own records, not neighbouring records.
    if (stride < sizeof(std::uint32_t)
        || integer(table["pointer_offset"], stride) > stride - sizeof(std::uint32_t)
        || integer(table["count_offset"], stride) > stride - sizeof(std::uint32_t)) {
        throw std::runtime_error("water source pointer fields exceed record stride");
    }

    image.check(address, count * stride);
    Surface result{
        static_cast<unsigned>(integer(table["source"], 65'535)),
        false,
        table["flowing"].boolean(),
        point(table["flow"]),
        {}
    };

    // Records belonging to one source merge art layers into one physical body.
    for (std::size_t index = 0; index < count; ++index) {
        append_strip(result, image, address + index * stride, table, vertex_layout);
    }

    return result;
}
}  // namespace openrac::lighting::water::source
