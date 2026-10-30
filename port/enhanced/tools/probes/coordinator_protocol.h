// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded JSON-lines framing and escaped strings for native/Python bake coordination.
#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <string_view>

#include "common/json.h"

namespace openrac::lighting {
// Quotes UTF-8 text as JSON, escaping all control bytes and preserving ordinary path characters.
std::string coordinator_string(std::string_view text);
// Reads a bounded line, retaining its original text for the final region plan.
bool read_coordinator_line(std::istream& input, std::string& line);
// Reads a complete bounded protocol record; false means clean EOF before a new record.
// Malformed or oversized input throws before it can reach a job or allocate a scene.
bool read_coordinator_message(std::istream& input, json::Value& message);
// Reads an unsigned integer within exact JSON-number range, rejecting fractions and missing fields.
std::uint64_t coordinator_integer(const json::Value& value, std::uint64_t maximum);
}  // namespace openrac::lighting
