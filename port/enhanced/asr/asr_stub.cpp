// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Builds without Arm ASR (OPENRAC_ARM_ASR off) upscale with FSR 3.1 alone.
#include "enhanced/asr/asr_context.h"

namespace openrac::enhanced::asr_detail {
struct AsrContext::State {};

bool preferred() {
    return false;
}

AsrContext::AsrContext() = default;
AsrContext::~AsrContext() = default;

bool AsrContext::configure(std::array<int, 2>, std::array<int, 2>, std::string& error) {
    error = "this build has no Arm ASR";
    return false;
}

bool AsrContext::dispatch(
    std::array<unsigned, 5>, const fsr3_detail::Dispatch&, std::string& error
) {
    error = "this build has no Arm ASR";
    return false;
}

bool AsrContext::float16_active() const {
    return false;
}

void AsrContext::release() {}
}  // namespace openrac::enhanced::asr_detail
