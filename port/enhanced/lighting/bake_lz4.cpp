// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/lighting/bake_lz4.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <memory>
#include <stdexcept>

#ifdef OPENRAC_BAKE_LZ4
#include <lz4frame.h>
#endif

namespace openrac::lighting {
bool bake_lz4_available() {
#ifdef OPENRAC_BAKE_LZ4
    return true;
#else
    return false;
#endif
}

bool is_lz4_bake_asset(std::span<const std::uint8_t> bytes) {
    constexpr std::array<std::uint8_t, 4> kMagic{0x04, 0x22, 0x4d, 0x18};
    return bytes.size() >= kMagic.size() && std::equal(kMagic.begin(), kMagic.end(), bytes.begin());
}

#ifdef OPENRAC_BAKE_LZ4
namespace {
std::size_t checked(std::size_t result) {
    if (LZ4F_isError(result)) {
        throw std::runtime_error(LZ4F_getErrorName(result));
    }
    return result;
}
}  // namespace

std::vector<std::uint8_t> compress_bake_asset(std::span<const std::uint8_t> bytes) {
    LZ4F_preferences_t options{};
    options.frameInfo.contentSize = bytes.size();
    options.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;
    std::vector<std::uint8_t> frame(checked(LZ4F_compressFrameBound(bytes.size(), &options)));
    const auto size = checked(
        LZ4F_compressFrame(frame.data(), frame.size(), bytes.data(), bytes.size(), &options)
    );
    if (size >= bytes.size()) {
        return {};
    }
    frame.resize(size);
    return frame;
}

std::vector<std::uint8_t> decompress_bake_asset(
    std::span<const std::uint8_t> frame, std::size_t max_bytes
) {
    LZ4F_dctx* context = nullptr;
    checked(LZ4F_createDecompressionContext(&context, LZ4F_VERSION));
    const std::unique_ptr<LZ4F_dctx, decltype(&LZ4F_freeDecompressionContext)> decoder(
        context, &LZ4F_freeDecompressionContext
    );
    LZ4F_frameInfo_t info{};
    std::size_t header_size = frame.size();
    checked(LZ4F_getFrameInfo(decoder.get(), &info, frame.data(), &header_size));
    if (info.frameType != LZ4F_frame || info.dictID || !info.contentSize
        || info.contentSize > max_bytes
        || info.contentChecksumFlag != LZ4F_contentChecksumEnabled) {
        throw std::runtime_error("invalid LZ4 bake asset header");
    }
    frame = frame.subspan(header_size);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(info.contentSize));
    std::span remaining(bytes);
    while (true) {
        auto consumed = frame.size();
        auto written = remaining.size();
        const auto next = checked(LZ4F_decompress(
            decoder.get(), remaining.data(), &written, frame.data(), &consumed, nullptr
        ));
        frame = frame.subspan(consumed);
        remaining = remaining.subspan(written);
        if (next == 0 && remaining.empty() && frame.empty()) {
            return bytes;
        }
        if (next == 0 || frame.empty() || (!consumed && !written)) {
            throw std::runtime_error("invalid LZ4 bake asset size");
        }
    }
}
#else
std::vector<std::uint8_t> compress_bake_asset(std::span<const std::uint8_t>) {
    throw std::runtime_error("LZ4 bake assets require OPENRAC_BAKE_LZ4");
}

std::vector<std::uint8_t> decompress_bake_asset(std::span<const std::uint8_t>, std::size_t) {
    throw std::runtime_error("LZ4 bake assets require OPENRAC_BAKE_LZ4");
}
#endif

std::vector<std::uint8_t> read_bake_asset(
    const std::filesystem::path& path, std::size_t max_bytes
) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    if (size <= 0 || size > static_cast<std::streamoff>(max_bytes)) {
        throw std::runtime_error("invalid bake asset size: " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(
            reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        )) {
        throw std::runtime_error("cannot read bake asset: " + path.string());
    }
    return bytes;
}
}  // namespace openrac::lighting
