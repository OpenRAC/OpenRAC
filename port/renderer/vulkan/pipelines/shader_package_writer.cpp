// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic archive serialization with shared compressed SPIR-V/reflection chunks.
#include <algorithm>
#include <map>
#include <stdexcept>

#include "renderer/vulkan/pipelines/shader_package.h"
#include "renderer/vulkan/pipelines/shader_package_internal.h"

namespace openrac::vkgl {
namespace {
using namespace package_detail;

// Owns the compressed blocks and index while one archive is assembled.
class PackageBuilder {
public:
    // Appends one source-keyed entry, sharing byte-identical chunks with earlier entries.
    void add(const ShaderPackageEntry& entry) {
        const auto decoded = entry.uncompressed_size
                                 ? decompress_shader_blob(entry.value, entry.uncompressed_size)
                                 : std::vector<char>{};
        const auto value = entry.uncompressed_size ? std::span<const char>(decoded) : entry.value;

        if (value.empty()) {
            throw std::runtime_error("empty shader package entry");
        }

        number(m_index, entry.key.key);
        number(m_index, entry.key.check);
        number(m_index, entry.key.length);
        number(m_index, value.size());
        number(m_index, (value.size() + kChunkBytes - 1) / kChunkBytes);

        for (std::size_t at = 0; at < value.size(); at += kChunkBytes) {
            const auto part = value.subspan(at, std::min(kChunkBytes, value.size() - at));
            number(m_index, chunk(part));
        }
    }

    // Writes the header, entry index and compressed chunks in their insertion order.
    std::vector<char> finish(std::string_view fingerprint, std::size_t count) const {
        std::vector<char> out(kMagic.begin(), kMagic.end());
        number(out, fingerprint.size());
        out.insert(out.end(), fingerprint.begin(), fingerprint.end());
        number(out, count);
        number(out, m_chunks.size());
        out.insert(out.end(), m_index.begin(), m_index.end());

        for (const auto& part : m_chunks) {
            number(out, part.bytes.size());
            number(out, part.size);
            number(out, hash(part.bytes));
            out.insert(out.end(), part.bytes.begin(), part.bytes.end());
        }

        return out;
    }

private:
    // One compressed block and its decoded byte count.
    struct Chunk {
        std::vector<char> bytes;  // Owned LZ4-compressed block.
        std::uint64_t size;       // Size after decompression.
    };

    // Finds a byte-identical candidate, checking payloads rather than accepting hash collisions.
    std::uint32_t chunk(std::span<const char> part) {
        auto& candidates = m_by_hash[hash(part)];

        for (auto candidate : candidates) {
            const auto& previous = m_chunks[candidate];

            if (previous.size != part.size()) {
                continue;
            }

            const auto decoded = decompress_shader_blob(previous.bytes, previous.size);

            if (std::equal(decoded.begin(), decoded.end(), part.begin(), part.end())) {
                return candidate;
            }
        }

        const auto id = static_cast<std::uint32_t>(m_chunks.size());
        candidates.push_back(id);
        m_chunks.push_back({compress_shader_blob(part), part.size()});
        return id;
    }

    std::vector<Chunk> m_chunks;  // Blocks owned until finish returns the archive bytes.
    std::map<std::uint64_t, std::vector<std::uint32_t>> m_by_hash;  // Candidates by raw checksum.
    std::vector<char> m_index;  // Serialized key and chunk references, preceding the block data.
};
}  // namespace

std::vector<char> encode_shader_package(
    std::string_view fingerprint, std::span<const ShaderPackageEntry> entries
) {
    std::vector<ShaderPackageEntry> sorted(entries.begin(), entries.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.key.key < b.key.key;
    });
    PackageBuilder builder;

    for (const auto& entry : sorted) {
        builder.add(entry);
    }

    return builder.finish(fingerprint, sorted.size());
}
}  // namespace openrac::vkgl
