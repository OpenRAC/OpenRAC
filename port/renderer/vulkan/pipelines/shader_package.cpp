// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Immutable archive lookup and validation; payload views borrow the supplied package bytes.
#include "renderer/vulkan/pipelines/shader_package.h"

#include <algorithm>
#include <utility>

#include "renderer/vulkan/pipelines/shader_package_internal.h"

namespace openrac::vkgl {
using namespace package_detail;

bool ShaderPackage::load_index(Reader& input, std::uint64_t count, std::uint64_t chunk_count) {
    // An entry needs five words plus at least one chunk index; a chunk needs three words.
    if (!input.valid() || count > input.remaining() / (6 * kWordBytes)
        || chunk_count > input.remaining() / (3 * kWordBytes)) {
        return false;
    }

    for (std::uint64_t n = 0; n < count; ++n) {
        ShaderKey key{input.number(), input.number(), input.number()};
        const std::uint64_t size = input.number();
        const std::uint64_t parts = input.number();

        if (!input.valid() || !size || size > kMaxShaderBytes || !parts
            || parts > input.remaining() / kWordBytes
            || parts != (size + kChunkBytes - 1) / kChunkBytes
            || (!m_entries.empty() && m_entries.back().key.key >= key.key)) {
            return false;
        }

        Entry entry{key, size, {}};

        for (std::uint64_t i = 0; i < parts; ++i) {
            const std::uint64_t id = input.number();

            if (!input.valid() || id >= chunk_count) {
                return false;
            }

            entry.chunks.push_back(static_cast<std::uint32_t>(id));
        }

        m_entries.push_back(std::move(entry));
    }

    return true;
}

bool ShaderPackage::load_chunks(Reader& input, std::uint64_t count) {
    for (std::uint64_t n = 0; n < count; ++n) {
        const std::uint64_t size = input.number();
        const std::uint64_t raw_size = input.number();
        const std::uint64_t checksum = input.number();

        if (!input.valid() || !raw_size || raw_size > kChunkBytes || size > input.remaining()) {
            return false;
        }

        const auto value = input.take(size);

        if (hash(value) != checksum) {
            return false;
        }

        m_chunks.push_back({value, raw_size});
    }

    return true;
}

bool ShaderPackage::load(
    std::span<const char> bytes, std::string_view fingerprint, std::string& error
) {
    m_entries.clear();
    m_chunks.clear();
    const auto fail = [&] {
        m_entries.clear();
        m_chunks.clear();
        error = "incompatible or damaged OpenRAC shader package";
        return false;
    };
    Reader input(bytes);
    const auto magic = input.take(kMagic.size());

    if (!input.valid() || std::string_view(magic.data(), magic.size()) != kMagic) {
        return fail();
    }

    const std::uint64_t length = input.number();
    const auto stored = input.take(length);

    if (!input.valid() || std::string_view(stored.data(), stored.size()) != fingerprint) {
        return fail();
    }

    const std::uint64_t count = input.number();
    const std::uint64_t chunk_count = input.number();

    if (!load_index(input, count, chunk_count) || !load_chunks(input, chunk_count)
        || !input.done()) {
        return fail();
    }

    // Entry sizes must match the chunks that lookup will concatenate.
    for (const auto& entry : m_entries) {
        std::uint64_t size = 0;

        for (auto id : entry.chunks) {
            size += m_chunks[id].size;
        }

        if (size != entry.size) {
            return fail();
        }
    }

    return true;
}

std::vector<char> ShaderPackage::find(ShaderKey key) const {
    const auto found = std::lower_bound(
        m_entries.begin(), m_entries.end(), key.key, [](const auto& entry, std::uint64_t value) {
            return entry.key.key < value;
        }
    );

    if (found == m_entries.end() || found->key != key) {
        return {};
    }

    std::vector<char> out;
    out.reserve(static_cast<std::size_t>(found->size));

    for (auto id : found->chunks) {
        const auto& chunk = m_chunks[id];
        const auto decoded = decompress_shader_blob(chunk.bytes, chunk.size);

        if (decoded.empty()) {
            return {};
        }

        out.insert(out.end(), decoded.begin(), decoded.end());
    }

    return out;
}

}  // namespace openrac::vkgl
