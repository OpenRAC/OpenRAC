// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fingerprinted shader identities and immutable, compressed executable-package lookup.

#pragma once

#include <initializer_list>
#include <span>
#include <string_view>

#include "renderer/vulkan/cache/spirv_cache_file.h"

namespace openrac::vkgl {
namespace package_detail {
class Reader;
}

// A forward hash, reverse collision check and encoded source byte count.
struct ShaderKey {
    std::uint64_t key = 0;     // Sorted archive lookup key.
    std::uint64_t check = 0;   // Independent reverse-source collision check.
    std::uint64_t length = 0;  // Source identity length, including separators.
    bool operator==(const ShaderKey&) const = default;
};

// Final source text and compiler fingerprint determine identity, independently of GPU or cache.
ShaderKey shader_key(
    std::string_view kind,
    std::initializer_list<std::string_view> parts,
    std::string_view fingerprint = {}
);
// Returns the source/configuration identity embedded in this executable's archive.
std::string shader_package_fingerprint();

// A serialization view; its caller owns the payload until encoding completes.
struct ShaderPackageEntry {
    ShaderKey key;                        // Fingerprinted final source identity.
    std::span<const char> value;          // Borrowed raw or compressed program bytes.
    std::uint64_t uncompressed_size = 0;  // Zero accepts an uncompressed input when writing.
};

// Entries borrow the package bytes; a failed load leaves no usable entries.
class ShaderPackage {
public:
    // Validates all archive metadata and checksums; bytes must outlive this package.
    [[nodiscard]] bool load(
        std::span<const char> bytes, std::string_view fingerprint, std::string& error
    );
    // Returns decoded owned bytes, or an empty result for a missing or damaged entry.
    std::vector<char> find(ShaderKey key) const;

private:
    // Reads sorted source keys and validates every referenced chunk index.
    bool load_index(package_detail::Reader& input, std::uint64_t count, std::uint64_t chunk_count);
    // Validates block sizes/checksums before retaining borrowed payload views.
    bool load_chunks(package_detail::Reader& input, std::uint64_t count);

    // One validated key and the blocks that reconstruct its original program.
    struct Entry {
        ShaderKey key;                      // Sorted complete identity.
        std::uint64_t size;                 // Decoded program byte count.
        std::vector<std::uint32_t> chunks;  // Ordered indices into m_chunks.
    };

    // One bounded LZ4 block borrowing the archive owner's immutable payload.
    struct Chunk {
        std::span<const char> bytes;  // Valid until the loaded archive's owner releases it.
        std::uint64_t size;           // Decoded block byte count.
    };

    std::vector<Entry> m_entries;  // Owned validated lookup metadata.
    std::vector<Chunk> m_chunks;   // Owned views borrowing the loaded archive.
};

// Serializes sorted keys and shared blocks; unsupported entry sizes throw to the tool boundary.
std::vector<char> encode_shader_package(
    std::string_view fingerprint, std::span<const ShaderPackageEntry> entries
);
// Compresses one bounded compilation result; invalid sizes or compression failures throw.
std::vector<char> compress_shader_blob(std::span<const char> bytes);
// Returns decoded bytes or an empty result when the payload or expected size is invalid.
std::vector<char> decompress_shader_blob(std::span<const char> bytes, std::uint64_t size);
// Encodes native Vulkan descriptor reflection through the shared compilation-result format.
std::vector<char> encode_native_shader(const NativeComputeShader& shader);
// Validates native descriptors before replacing the caller's program.
[[nodiscard]] bool decode_native_shader(std::span<const char> bytes, NativeComputeShader& shader);

// Generated package: one shared library for every program (libopenrac-shaders), built in on iOS.
std::span<const char> embedded_shader_package();
// Returns the process-lifetime archive; a corrupt executable package throws during initialization.
const ShaderPackage& builtin_shader_package();
}  // namespace openrac::vkgl
