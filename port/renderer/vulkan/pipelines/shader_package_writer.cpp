// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic archive serialization with shared compressed SPIR-V/reflection chunks.
#include <algorithm>
#include <climits>
#include <map>
#include <stdexcept>

#include "renderer/vulkan/pipelines/shader_package.h"
#include "renderer/vulkan/pipelines/shader_package_internal.h"

namespace openrac::vkgl {
namespace {
using namespace package_detail;

// Reads a bounded stage word count from the compilation-result codec's little-endian bytes.
std::uint32_t program_word(std::span<const char> bytes) {
    std::uint32_t value = 0;

    // Callers check that the encoded word fits before taking this view.
    for (unsigned n = 0; n < sizeof(value); ++n) {
        value |= std::uint32_t(static_cast<unsigned char>(bytes[n])) << (n * CHAR_BIT);
    }

    return value;
}

// Keeps stage boundaries stable across vertex/fragment pairings without changing encoded bytes.
std::vector<std::span<const char>> program_sections(std::span<const char> value) {
    // SPIR-V headers start with this magic and contain five words before any instructions.
    constexpr std::uint32_t kSpirvMagic = 0x07230203, kSpirvHeaderWords = 5;
    constexpr std::size_t kCodeWordBytes = sizeof(std::uint32_t);
    std::vector<std::span<const char>> sections;
    std::size_t at = 0;

    // Keeping each count with its stage preserves the compilation-result codec during lookup.
    for (std::size_t stage = 0; stage < kShaderStages; ++stage) {
        // Markers and truncated vector headers are opaque entries, with no inferred boundaries.
        if (value.size() - at < kCodeWordBytes) {
            return {value};
        }

        const auto words = program_word(value.subspan(at, kCodeWordBytes));

        // Reject impossible vectors before multiplying the untrusted word count.
        if (words > (value.size() - at - kCodeWordBytes) / kCodeWordBytes) {
            return {value};
        }

        const auto bytes = static_cast<std::size_t>(words) * kCodeWordBytes;

        // Only compilation-result vectors get stage boundaries; arbitrary blobs stay intact.
        if (words
            && (words < kSpirvHeaderWords
                || program_word(value.subspan(at + kCodeWordBytes, kCodeWordBytes))
                       != kSpirvMagic)) {
            return {value};
        }

        sections.push_back(value.subspan(at, kCodeWordBytes + bytes));
        at += kCodeWordBytes + bytes;
    }

    // Reflection is independent of stage lengths and can share its own final blocks.
    if (at < value.size()) {
        sections.push_back(value.subspan(at));
    }

    return sections;
}

// Owns the compressed blocks and index while one archive is assembled.
class PackageBuilder {
public:
    // Appends one source-keyed entry, sharing byte-identical chunks with earlier entries.
    void add(const ShaderPackageEntry& entry) {
        const auto decoded = entry.uncompressed_size
                                 ? decompress_shader_blob(entry.value, entry.uncompressed_size)
                                 : std::vector<char>{};
        const auto value = entry.uncompressed_size ? std::span<const char>(decoded) : entry.value;

        if (value.empty() || value.size() > kMaxShaderBytes) {
            throw std::runtime_error("shader entry exceeds package limit");
        }

        const auto sections = program_sections(value);
        std::size_t parts = 0;

        // Every section starts a fresh block, preserving sharing when another stage changes size.
        for (const auto section : sections) {
            parts += (section.size() + kChunkBytes - 1) / kChunkBytes;
        }

        number(m_index, entry.key.key);
        number(m_index, entry.key.check);
        number(m_index, entry.key.length);
        number(m_index, value.size());
        number(m_index, parts);

        // Concatenating all referenced blocks still reconstructs the original codec verbatim.
        for (const auto section : sections) {
            for (std::size_t at = 0; at < section.size(); at += kChunkBytes) {
                const auto part = section.subspan(at, std::min(kChunkBytes, section.size() - at));
                number(m_index, chunk(part));
            }
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
