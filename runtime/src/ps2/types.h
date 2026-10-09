// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ps2 {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

// A field of `count` bits starting at bit `start`.
constexpr u64 bits(u64 value, unsigned start, unsigned count) {
    return (value >> start) & ((u64{1} << count) - 1);
}

// The PlayStation 2 is little-endian and so is every host this is built for;
// memcpy keeps unaligned reads defined.
template <typename T>
inline T load(const void* p) {
    T v;
    std::memcpy(&v, p, sizeof(T));
    return v;
}

template <typename T>
inline void store(void* p, T v) {
    std::memcpy(p, &v, sizeof(T));
}

inline float as_float(u32 v) {
    return load<float>(&v);
}

inline u32 as_u32(float v) {
    return load<u32>(&v);
}

}  // namespace ps2
