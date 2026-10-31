// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared runtime arithmetic guard for cached and newly written probe verification.
#pragma once
#include <cfenv>
#include <stdexcept>

namespace openrac::lighting {
// Restore the caller's arithmetic mode even if read-back diagnostics reject a volume.
class RuntimeProbeRounding {
public:
    // Saves the complete caller environment before selecting runtime truncation.
    RuntimeProbeRounding() {
        // Verification must reproduce runtime arithmetic without changing the caller environment.
        if (std::fegetenv(&m_previous) != 0 || std::fesetround(FE_TOWARDZERO) != 0) {
            throw std::runtime_error("cannot set runtime probe verification rounding");
        }
    }

    // Restores the saved environment even when verification throws.
    ~RuntimeProbeRounding() { std::fesetenv(&m_previous); }

    RuntimeProbeRounding(const RuntimeProbeRounding&) = delete;
    RuntimeProbeRounding& operator=(const RuntimeProbeRounding&) = delete;
    RuntimeProbeRounding(RuntimeProbeRounding&&) = delete;
    RuntimeProbeRounding& operator=(RuntimeProbeRounding&&) = delete;

private:
    std::fenv_t m_previous{};  // restored before the next grid is traced
};

}  // namespace openrac::lighting
