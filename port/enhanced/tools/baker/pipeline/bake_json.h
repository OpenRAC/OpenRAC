// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded JSON and source identities shared by native level preparation and planning.
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

#include "assets/disc/checksum.h"
#include "common/json.h"
#include "common/result.h"

namespace openrac::lighting::bake_json {
// Mutable object fields used while preserving local overrides; values own their parsed children.
using Object = std::map<std::string, json::Value, std::less<>>;
// Emits a parsed JSON value with stable member ordering and round-trip numeric precision.
std::string json_text(const json::Value& value);
// Reads a required object, rejecting invalid JSON and sources larger than 64 MiB.
json::Value document(const std::filesystem::path& path);
// Reads an optional object; an absent file has no fields, while malformed present input is rejected.
Object optional_object(const std::filesystem::path& path);
// Builds an immutable JSON object from owned mutable fields.
json::Value object_value(const Object& fields);
// Compares parsed values independent of key ordering or numeric spelling.
bool equivalent(const json::Value& first, const json::Value& second);
// Reads at most 64 MiB of owned source bytes, rejecting short reads.
std::vector<std::uint8_t> source_bytes(const std::filesystem::path& path);
// Appends complete source bytes to a digest through bounded storage; throws on incomplete reads.
void hash_file(assets::disc::Sha256& hash, const std::filesystem::path& path);
// Hashes one file without allocating a complete binary source copy; throws on read failures.
std::string file_hash(const std::filesystem::path& path);
// Checks written size and SHA-256 through bounded file storage, without a second full-file buffer.
[[nodiscard]] Result<void> verify_file_checksum(
    const std::filesystem::path& path, std::span<const std::uint8_t> expected
);
// Computes the complete source SHA-256 used by Python cache and diagnostic reports.
std::string source_hash(std::span<const std::uint8_t> bytes);
}  // namespace openrac::lighting::bake_json
