// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Material module's scoped rounding for portable metadata parsing, serialization and FP32 conversion.
#pragma once
#include <cfenv>
#include <stdexcept>

namespace openrac::materials {
// Keeps decimal metadata canonical across the baker and games running with toward-zero arithmetic.
class MetadataRounding {
public:
    // Selects nearest rounding for JSON numbers; reports unsupported floating-point environments.
    MetadataRounding() : m_previous(std::fegetround()) {
        if (m_previous == -1 || std::fesetround(FE_TONEAREST) != 0) {
            throw std::runtime_error("cannot select canonical material metadata rounding");
        }
    }

    MetadataRounding(const MetadataRounding&) = delete;
    MetadataRounding& operator=(const MetadataRounding&) = delete;
    MetadataRounding(MetadataRounding&&) = delete;
    MetadataRounding& operator=(MetadataRounding&&) = delete;

    // Restores the calling thread's game arithmetic before leaving the metadata parser.
    ~MetadataRounding() { std::fesetround(m_previous); }

private:
    int m_previous;  // Calling thread's rounding mode, restored on success and failure.
};

}  // namespace openrac::materials
