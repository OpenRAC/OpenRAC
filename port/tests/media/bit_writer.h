// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/mpeg2.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// A minimal MSB-first bit writer for building synthetic MPEG-2 streams in
// the media tests.

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace openrac::test {

class BitWriter {
public:
    std::vector<std::uint8_t>& bytes() { return m_bytes; }

    void put(std::uint32_t value, unsigned bits) {
        for (unsigned k = bits; k-- > 0;) {
            m_acc = static_cast<std::uint8_t>(m_acc << 1 | ((value >> k) & 1));
            if (++m_count == 8) {
                m_bytes.push_back(m_acc);
                m_acc = 0;
                m_count = 0;
            }
        }
    }

    // A code as the standard prints it ("0000 0101 10").
    void code(std::string_view bits) {
        for (const char c : bits) {
            if (c == '0' || c == '1') {
                put(c == '1' ? 1 : 0, 1);
            }
        }
    }

    void align() {
        while (m_count != 0) {
            put(0, 1);
        }
    }

    void start(std::uint8_t code) {
        align();
        m_bytes.insert(m_bytes.end(), {0, 0, 1, code});
    }

private:
    std::vector<std::uint8_t> m_bytes;
    std::uint8_t m_acc = 0;
    unsigned m_count = 0;
};

}  // namespace openrac::test
