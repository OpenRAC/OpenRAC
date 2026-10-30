// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Coordinator wire framing; scene arrays remain in their existing diagnostic and binary formats.
#include "enhanced/tools/probes/coordinator_protocol.h"

#include <cmath>
#include <stdexcept>

namespace openrac::lighting {
std::string coordinator_string(std::string_view text) {
    std::string result = "\"";
    constexpr char kHex[] = "0123456789abcdef";  // JSON's hexadecimal control-byte spelling.

    for (const unsigned char byte : text) {
        if (byte == '"' || byte == '\\') {
            result += '\\';
            result += static_cast<char>(byte);
        } else if (byte < 32) {
            result += "\\u00";
            result += kHex[byte >> 4];
            result += kHex[byte & 15];
        } else {
            result += static_cast<char>(byte);
        }
    }

    return result + '"';
}

bool read_coordinator_line(std::istream& input, std::string& line) {
    // Region plans may carry thousands of bounds, but never unbounded scene geometry.
    constexpr std::size_t kMaximumRecord = 16ull * 1024u * 1024u;
    line.clear();
    char byte = 0;

    while (input.get(byte) && byte != '\n') {
        if (line.size() == kMaximumRecord) {
            throw std::runtime_error("coordinator record exceeds 16 MiB");
        }

        line += byte;
    }

    if (line.empty() && !input) {
        return false;
    }

    return true;
}

bool read_coordinator_message(std::istream& input, json::Value& message) {
    std::string line;

    if (!read_coordinator_line(input, line)) {
        return false;
    }

    std::string error;

    if (!json::parse(line, message, error) || !message.is_object()) {
        throw std::runtime_error("invalid coordinator record: " + error);
    }

    return true;
}

std::uint64_t coordinator_integer(const json::Value& value, std::uint64_t maximum) {
    const double number = value.number(-1);

    if (!value.is_number() || !std::isfinite(number) || number < 0
        || number > static_cast<double>(maximum) || std::floor(number) != number) {
        throw std::runtime_error("invalid coordinator integer");
    }

    return static_cast<std::uint64_t>(number);
}
}  // namespace openrac::lighting
