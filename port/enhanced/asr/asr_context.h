// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GPU lifetime and dispatch of Arm Accuracy Super Resolution, the temporal upscaler made for the
// tile-based GPUs of phones: the same inputs and calls as fsr3_detail::SdkContext, so that the
// temporal module (enhanced/fsr3) picks either one behind its frame hooks.
#pragma once
#include <array>
#include <memory>
#include <string>

#include "enhanced/fsr3/sdk_context.h"

namespace openrac::enhanced::asr_detail {
// Whether Arm ASR upscales instead of FSR 3.1: OPENRAC_TEMPORAL_UPSCALER=asr or fsr3 when it is
// set, otherwise on Qualcomm Adreno GPUs only, where it measured faster than FSR 3.1.
[[nodiscard]] bool preferred();
// Whether ASR can upscale from `render_size`: its luminance pyramid (half the render size) needs a
// sixth level, so the larger side must be at least 64 pixels; FSR 3.1 upscales smaller frames.
[[nodiscard]] bool supports(std::array<int, 2> render_size);

// One GPU resource context, used only on the render thread.
class AsrContext {
public:
    AsrContext();
    // Drains queued work before releasing a live context; the graphics context must remain current.
    ~AsrContext();
    AsrContext(const AsrContext&) = delete;
    AsrContext& operator=(const AsrContext&) = delete;

    // Recreates GPU resources when render/display extents change; false includes a diagnostic.
    [[nodiscard]] bool configure(
        std::array<int, 2> render_size, std::array<int, 2> display_size, std::string& error
    );
    // Records colour, depth, motion, reactive mask and output, in that order, as SdkContext does.
    [[nodiscard]] bool dispatch(
        std::array<unsigned, 5> textures, const fsr3_detail::Dispatch& frame, std::string& error
    );
    // Whether its 16-bit shader variants were chosen (the device enabled half arithmetic).
    [[nodiscard]] bool float16_active() const;
    // Retires GPU resources while the graphics context is still current.
    void release();

private:
    struct State;  // The library's context and scratch memory, kept out of this header.
    std::unique_ptr<State> m_state;
};
}  // namespace openrac::enhanced::asr_detail
