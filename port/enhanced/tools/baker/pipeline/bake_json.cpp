// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared baker JSON and source identities, without retaining full binary sources after parsing.
#include "enhanced/tools/baker/pipeline/bake_json.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "assets/disc/checksum.h"
#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/coordinator_protocol.h"

namespace openrac::lighting::bake_json {
namespace {
// Keeps recursive array traversal outside the tagged-value dispatch.
std::string array_json(const json::Value& value) {
    std::string result = "[";

    // Retain every profile array element in source order.
    for (const auto& item : value.items()) {
        // JSON separators belong between elements only.
        if (result.size() > 1) {
            result += ',';
        }

        result += json_text(item);
    }

    return result + ']';
}

// Keeps object traversal stable by the parsed document's sorted member names.
std::string object_json(const json::Value& value) {
    std::string result = "{";

    // Sorted members give stable profile serialization.
    for (const auto& [key, item] : value.members()) {
        // JSON separators belong between members only.
        if (result.size() > 1) {
            result += ',';
        }

        result += coordinator_string(key) + ':' + json_text(item);
    }

    return result + '}';
}
}  // namespace

std::string json_text(const json::Value& value) {
    // Preserve each parsed JSON type in the report.
    switch (value.type()) {
        case json::Value::Type::Null:
            return "null";
        case json::Value::Type::Bool:
            return value.boolean() ? "true" : "false";
        case json::Value::Type::Number: {
            std::ostringstream out;
            out << std::setprecision(std::numeric_limits<double>::max_digits10) << value.number();
            return out.str();
        }
        case json::Value::Type::String:
            return coordinator_string(value.string());
        case json::Value::Type::Array:
            return array_json(value);
        case json::Value::Type::Object:
            return object_json(value);
    }

    throw std::runtime_error("invalid bake JSON value");
}

json::Value document(const std::filesystem::path& path) {
    const auto bytes = source_bytes(path);
    // The JSON parser borrows this bounded byte buffer only until parsing completes.
    const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    json::Value value;
    std::string error;

    // Unreadable or malformed documents cannot seed a valid plan.
    if (!json::parse(text, value, error) || !value.is_object()) {
        throw std::runtime_error("invalid bake JSON input: " + error);
    }

    return value;
}

std::vector<std::uint8_t> source_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    constexpr std::streamoff kMaximumSource = 64ll * 1024 * 1024;

    // Bound binary input allocation before reading gameplay records.
    if (!input || input.tellg() < 0 || input.tellg() > kMaximumSource) {
        throw std::runtime_error("missing or oversized bake source: " + path.string());
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(input.tellg()));
    input.seekg(0);

    // Short reads must not acquire a valid cache identity.
    if (!input.read(
            reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        )) {
        throw std::runtime_error("cannot read bake source: " + path.string());
    }

    return bytes;
}

void hash_file(assets::disc::Sha256& hash, const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::array<char, 65'536> buffer{};

    // A missing source cannot share an identity with an empty file.
    if (!input) {
        throw std::runtime_error(
            "cannot read bake source: " + path.string() + "; re-export the level if missing"
        );
    }

    // Source size does not increase temporary hashing storage.
    while (input.read(buffer.data(), buffer.size()) || input.gcount() > 0) {
        hash.update(std::string_view(buffer.data(), static_cast<std::size_t>(input.gcount())));
    }

    // Interrupted reads cannot authorise cache reuse or a complete source report.
    if (!input.eof()) {
        throw std::runtime_error("incomplete bake source read: " + path.string());
    }
}

std::string file_hash(const std::filesystem::path& path) {
    assets::disc::Sha256 hash;
    hash_file(hash, path);
    return assets::disc::to_hex(hash.finish());
}

std::string source_hash(std::span<const std::uint8_t> bytes) {
    return assets::disc::to_hex(assets::disc::sha256(bytes));
}

Result<void> verify_file_checksum(
    const std::filesystem::path& path, std::span<const std::uint8_t> expected
) {
    // I/O failures must prevent publication of a completed bake.
    try {
        const profiling::Zone timing("Verify bake file checksum");
        // Size mismatches need no full-file hashing and also catch truncation or trailing bytes.
        if (std::filesystem::file_size(path) != expected.size()) {
            return unexpected(Error{"bake read-back size mismatch"});
        }
        // Streaming the saved file keeps scratch storage fixed at the existing hash-reader block size.
        if (file_hash(path) != source_hash(expected)) {
            return unexpected(Error{"bake read-back checksum mismatch"});
        }
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}

Object optional_object(const std::filesystem::path& path) {
    // Missing authored sidecars differ from malformed present definitions.
    if (!std::filesystem::exists(path)) {
        return {};
    }

    return document(path).members();
}

json::Value object_value(const Object& fields) {
    std::string text = "{";

    // Sorted keys keep generated output independent of merge insertion order.
    for (const auto& [key, value] : fields) {
        // JSON separators belong between members only.
        if (text.size() > 1) {
            text += ',';
        }

        text += coordinator_string(key) + ':' + json_text(value);
    }

    text += '}';
    json::Value value;
    std::string error;

    // Construction still passes through the existing bounded-depth JSON validator.
    if (!json::parse(text, value, error)) {
        throw std::runtime_error("cannot assemble bake JSON object: " + error);
    }

    return value;
}

bool equivalent(const json::Value& first, const json::Value& second) {
    // Python's authored-default cleanup equates boolean values with their numeric 0/1 counterparts.
    if ((first.type() == json::Value::Type::Bool && second.is_number())
        || (second.type() == json::Value::Type::Bool && first.is_number())) {
        const double a = first.is_number() ? first.number() : static_cast<int>(first.boolean());
        const double b = second.is_number() ? second.number() : static_cast<int>(second.boolean());
        return a == b;
    }

    return json_text(first) == json_text(second);
}
}  // namespace openrac::lighting::bake_json
